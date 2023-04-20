#pragma once

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
// #include <unistd.h>
#include <iostream>
#include <string>
#include <map>
#include <fstream>

#include "database.h"
#include "UI.h"
#include "utils.h"

#include <winerror.h>
#include <windows.h>
#include <conio.h>
#include <gdiplus.h>

#include <random>
#include <chrono>
#include <filesystem>

using namespace std;
// using namespace utils;
using namespace Gdiplus;

#define SND_BUF 1024
#define PIC_SIZE_LEN 128
#define TIMEOUT 10

SOCKET global_fd;
bool exitRequested = false;

namespace handle
{
	// Handle program.
	void initWinsockLib()
	{
		WSADATA WSAData;
		int error = WSAStartup(MAKEWORD(2, 2), &WSAData);
		if (error < 0)
		{
			cout << "WSAStartup failed with error: " << error << endl;
			exit(1);
		}
	}

	// Exit program.
	void esc(string error_func, SOCKET fd1, SOCKET fd2 = 0)
	{
		cerr << "Error: " << error_func << " error\n";
		closesocket(fd1);
		closesocket(fd2);
		WSACleanup();
		exit(1);
	}

	// Check the connections between client and server.
	bool checkConnect(SOCKET fd)
	{
		int optval;
		socklen_t optlen = sizeof(optval);
		int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&optval, &optlen);
		if (ret == 0 && optval == 0)
			// Socket is open and ready to use
			return true;
		else
			// Socket is closed or in error state
			return false;
	}


	// Random a png file name to store.
	string GeneratePNG_FileName(string saveDir)
	{
		string saveFilename;
		string savePath;
		do
		{
			// Tạo một số ngẫu nhiên từ thời gian hệ thống
			std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());

			// Tạo một tên tập tin ngẫu nhiên
			std::uniform_int_distribution<int> dist(1, 1000000);
			saveFilename = "picture" + std::to_string(dist(rng)) + ".png";

			// Kiểm tra xem tập tin đã tồn tại hay chưa
			savePath = saveDir + saveFilename;
		} while (std::filesystem::exists(savePath));
		return savePath;
	}

	//
	int GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
	{
		UINT num = 0;
		UINT size = 0;

		ImageCodecInfo *pImage = NULL;

		GetImageEncodersSize(&num, &size);
		if (size == 0)
			return -1;

		pImage = (ImageCodecInfo *)(malloc(size));
		if (pImage == NULL)
			return -1;

		GetImageEncoders(num, size, pImage);

		for (UINT j = 0; j < num; ++j)
		{
			if (wcscmp(pImage[j].MimeType, format) == 0)
			{
				*pClsid = pImage[j].Clsid;
				free(pImage);
				return j;
			}
		}

		free(pImage);
		return -1;
	}

	// Capture screen and save as png type.
	string captureAndSave()
	{
		// It just capture full screen with the desktop scaling 100%.
		// Need to optimize by getting desktop scaling.
		int width = GetSystemMetrics(SM_CXSCREEN);
		int height = GetSystemMetrics(SM_CYSCREEN);

		// std::cout << "Width: " << width << std::endl;
		// std::cout << "Height: " << height << std::endl;

		HDC hdcScreen = GetDC(NULL);
		HDC hdcMemDC = CreateCompatibleDC(hdcScreen);
		HBITMAP hbmScreen = NULL;

		GdiplusStartupInput gdip;
		ULONG_PTR gdipToken;
		GdiplusStartup(&gdipToken, &gdip, NULL);

		// hbmScreen = CreateCompatibleBitmap(hdcScreen, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		hbmScreen = CreateCompatibleBitmap(hdcScreen, width, height);
		// hbmScreen = CreateCompatibleBitmap(hdcScreen, 700, 700);

		SelectObject(hdcMemDC, hbmScreen);

		BitBlt(hdcMemDC, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);
		// BitBlt(hdcMemDC, 0, 0, 700, 700, hdcScreen, 100, 200, SRCCOPY);

		CLSID encoderID;

		GetEncoderClsid(L"image/png", &encoderID); // image/jpeg

		Bitmap *bmp = new Bitmap(hbmScreen, (HPALETTE)0);
		bmp->Save(L"screen.png", &encoderID, NULL);

		GdiplusShutdown(gdipToken);

		DeleteObject(hbmScreen);
		DeleteObject(hdcMemDC);
		ReleaseDC(NULL, hdcScreen);

		return "screen.png";
	}

	// Send image from server to client.
	bool sendImage(string picPath, SOCKET fd)
	{
		char *fileName = strdup(picPath.c_str());
		FILE *fr = fopen(fileName, "rb");
		free(fileName);
		if (!fr)
		{
			cerr << "Error: Cannot open file. Failed with sending image.\n";
			return false;
		}

		// Get picture size
		int pic_size = 0;
		fseek(fr, 0, SEEK_END);
		pic_size = ftell(fr);
		fseek(fr, 0, SEEK_SET);

		// Send picture size
		char file_size[PIC_SIZE_LEN];
		sprintf(file_size, "%d", pic_size);
		int error = send(fd, file_size, strlen(file_size), 0);
		if (error < 0)
		{
			cerr << "Error: Cannot send picture size.\n";
			return false;
		}

		// Send pictures by splitting them into small files
		char sendBuf[SND_BUF];
		int sentSize = 0;
		while (pic_size > 0)
		{
			if (pic_size >= SND_BUF)
			{
				fread(sendBuf, SND_BUF, 1, fr);
				send(fd, sendBuf, SND_BUF, 0);
			}
			else
			{
				fread(sendBuf, pic_size, 1, fr);
				send(fd, sendBuf, pic_size, 0);
			}
			pic_size -= SND_BUF;
		}

		fclose(fr);
		return true;
	}

	// Receive image from server and open it.
	bool receiveImage(string savePath, SOCKET fd)
	{
		// Receive file size.
		char file_size[PIC_SIZE_LEN];
		int error = 0;
		error = recv(fd, file_size, PIC_SIZE_LEN, 0);
		if (error < 0)
		{
			cerr << "Error: receive image's file size error.\n";
			return false;
		}
		int pic_size = atoi(file_size);

		// Save picture to client computer with savePath.
		FILE *fw = fopen(savePath.c_str(), "wb");

		if (!fw)
		{
			cerr << "Error: Cannot open saved screen file.\n";
			return false;
		}

		// Save pic.
		char readBuf[SND_BUF];
		while (pic_size > 0)
		{
			if (pic_size >= SND_BUF)
			{
				recv(fd, readBuf, SND_BUF, 0);
				fwrite(readBuf, SND_BUF, 1, fw);
			}
			else
			{
				recv(fd, readBuf, pic_size, 0);
				fwrite(readBuf, pic_size, 1, fw);
			}
			pic_size -= SND_BUF;
		}

		fclose(fw);
		return true;
	}

	// Capture Screen: rcv img from server, storing and opening img.
	void captureScreen(SOCKET fd)
	{
		string s = string("3");
		char *tmp = strdup(s.c_str());

		int error = send(fd, tmp, strlen(tmp), 0);
		free(tmp);
		if (error == SOCKET_ERROR)
			esc("send", fd);

		string saveDir = "F:\\Year2_Term2\\ComputerNetwork\\Project3_SocketProgramming\\client\\";
		string savePath = GeneratePNG_FileName(saveDir);
		if (!receiveImage(savePath, fd))
		{
			cerr << "Error: receive image error.\n";
			esc("send", fd);
		};

		if (system(("start " + savePath).c_str()) != 0) // If failed
		{
			cerr << "Error: open image error.\n";
			esc("send", fd);
		}
	}

	// Catch key presses and send to client
	LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION)
		{
			KBDLLHOOKSTRUCT *pKeyboard = (KBDLLHOOKSTRUCT *)lParam;
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
			{
				DWORD dwMsg = 1;
				dwMsg += pKeyboard->scanCode << 16;
				dwMsg += pKeyboard->flags << 24;
				char ch = MapVirtualKey(pKeyboard->vkCode, MAPVK_VK_TO_CHAR);
				if (isalpha(ch))
					ch = (GetKeyState(VK_CAPITAL) || GetKeyState(VK_SHIFT) & 0x8000) ? toupper(ch) : tolower(ch);

				int iResult = send(global_fd, &ch, 1, 0); // Send the key to the client
				if (iResult == SOCKET_ERROR)
					esc("send", global_fd);
			}
		}
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	};

	//
	void Server_CatchKeyPresses(SOCKET fd)
	{
		global_fd = fd;
		HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
		MSG msg;
		DWORD result;

		while (!exitRequested)
		{
			result = MsgWaitForMultipleObjectsEx(0, NULL, TIMEOUT * 1000, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

			if (result == WAIT_OBJECT_0) // New message has arrived
			{
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					// TranslateMessage(&msg);
					// DispatchMessage(&msg);
				}
			}
			else if (result == WAIT_TIMEOUT) // Timeout occurred
			{
				cout << "come break\n";
				break;
			}
		}

		UnhookWindowsHookEx(hHook);
		cout << "Unhooked" << endl;
	}

	// Receive key presses from server and print to the console.
	void Client_CatchKeyPresses(SOCKET fd)
	{
		string s = string("4");
		char *tmp = strdup(s.c_str());

		int error = send(fd, tmp, strlen(tmp), 0);
		free(tmp);

		cout << "Caught key presses\n";
		while (1)
		{
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			struct timeval timeout;
			timeout.tv_sec = TIMEOUT;
			timeout.tv_usec = 0;

			int res = select(fd + 1, &fds, NULL, NULL, &timeout);
			if (res == -1)
			{
				cerr << "\nError in select(): " << strerror(errno) << endl;
				system("pause");
				break;
			}
			else if (res == 0)
			{
				cout << "\nNo key press received in the last " << TIMEOUT << " seconds. Stopping." << endl;
				system("pause");
				break;
			}
			else
			{
				char ch;
				int error = recv(fd, &ch, 1, 0);
				if (error < 0)
					esc("recv", fd);

				if (ch == '\r')
					cout << endl;
				else if (ch == '\t')
					cout << "\t";
				else if (ch == '\b')
				{
					cout << '\b'; // print backspace
					cout << ' ';  // print space char to deleting char on console
					cout << '\b'; // move pointer to the previous position
				}
				else
					printf("%c", ch);
			}
		}
	}
}