#pragma once

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
#include <sstream>
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
namespace fs = std::filesystem;
#define SND_BUF 1024
#define PIC_SIZE_LEN 128
#define TIMEOUT 10
#define EXT_SIZE 10

SOCKET global_fd;
bool exitRequested = false;
// path cua Khanh
string path = "";
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
	string GeneratePNG_FileName(string saveDir, string fileName = "picture", string end = ".png")
	{
		string saveFilename;
		string savePath;
		do
		{
			// Tạo một số ngẫu nhiên từ thời gian hệ thống
			std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());

			// Tạo một tên tập tin ngẫu nhiên
			std::uniform_int_distribution<int> dist(1, 1000000);
			saveFilename = fileName + std::to_string(dist(rng)) + end;

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
		memset(readBuf, '\n', SND_BUF);
		cout << pic_size;

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
				fwrite(readBuf, pic_size, 1, fw);;
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

		string saveDir = "C:\\Users\\LONG KHANH\\Downloads";
		string savePath = GeneratePNG_FileName(saveDir, "png");
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

	// KHANH

	bool IsValidPath(string path)
	{

		if (!std::filesystem::is_directory(path))
		{
			std::cout << "Invalid path\n";
			return false;
		}
		return true;
	}
	string print_directory1(string path)
	{

		std::stringstream ss;

		for (const auto &entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory())
			{
				ss << "[+] " << entry.path().filename() << "\n";
			}
			else if (entry.is_regular_file())
			{
				ss << "[-] " << entry.path().filename() << "\n";
			}
		}

		return ss.str();
	}
	string PrintDrives()
	{

		DWORD drives = GetLogicalDrives();
		string rs;
		for (int i = 0; i < 26; i++)
		{
			if (drives & (1 << i))
			{

				std::string driveName = std::string(1, 'A' + i) + ":";
				string path = driveName;
				rs += (driveName + "\n");
				// for (const auto &entry : fs::directory_iterator(path))
				//     std::cout << entry.path() << std::endl;
			}
		}
		return rs;
	}

	vector<string> spiltInput(char *buffer)
	{
		std::stringstream ss(buffer);
		std::vector<std::string> tokens;
		std::string token;
		while (ss >> token)
		{
			tokens.push_back(token);
		}
		return tokens;
	}

	string seeAvailableFiles(string path)
	{
		string rs = "";
		fs::path directory_path = path;

		for (auto &file : fs::directory_iterator(directory_path))
		{
			if (file.is_regular_file())
			{
				rs += (file.path().filename().stem().string() + file.path().extension().string() + "\n");
			}
		}
		return rs;
	}

	string RenameFile(string path, vector<string> tokens)
	{
		string old_path = path + tokens[1];
		string new_path = path + tokens[2];
		if (rename(old_path.c_str(), new_path.c_str()) != 0)
		{
			perror("Error renaming file");
			return "0";
		}

		cout << "File renamed successfully\n";
		return "1";
	}
	string deleteFile(string filepath)
	{
		if (remove(filepath.c_str()) != 0)
		{
			perror("Error renaming file");
			return "0";
		}

		cout << "File deleted successfully\n";
		return "1";
	}
	string fileExtension(string filePath)
	{
		fs::path file_path(filePath);
		std::string extension = file_path.extension().string();
		return extension;
	}
	string getInput(){
		cout << "1. You must you this command first,  use \"listall\".\n"
		     << "2. Go to a folder, use \"cd [FOLDER NAME DISPLAYED]\".\n"
		     << "4. See available transfer/rename/delete files in this folder, use \"see\"  \n"
		     << "3. End browsing option \"end\": \n>>>";
		cout << "Command code here: \n";
		string input;
		input.clear();

		cin >> ws;
		getline(cin, input);
		return input;
	}
	string getCommand(){
		cout << "1.To transfer  \"a.txt\", use \"transfer a.txt\"" << endl;
		cout << "2.To rename  \"a.txt\", use \"rename a.txt [NEW_NAME].txt\"" << endl;
		cout << "3.To delete  \"a.txt\", use \"delete a.txt \"" << endl;
		cout << "else, use   \"quit\" \n";
		cout << "Command code here: \n";
		string input;
		input.clear();
		cin >> ws;
		getline(cin, input);
		return input;

	}

	string Client_recvFileExtension(SOCKET fd){
		char extension[EXT_SIZE];
		int error = recv(fd, extension, EXT_SIZE, 0);
		std::string extension_string(extension, EXT_SIZE);
		return extension_string;
	}
	void Server_Transfer(SOCKET fd, string path){
		// send file extension
		string extension = fileExtension(path);

		send(fd, extension.c_str(), extension.size(), 0);

		// transfer file
		int t = sendImage(path, fd);
		if (!t)
			cerr << "Error: send file error.\n";
	}
	void Server_ProcessPath(string& path, vector<string> tokens, int &contact){

		// check if user access the drive -> the drive path must include "\\" behind
		//     e.g: C:\\
        //whereas, ordinary folder does not, e.g: C:\\Garena\\FifaOnline4
		path = path + tokens[1];

		// if folder name includes spaces
		if (tokens[2] != "")
		{
			for (int i = 2; i < tokens.size(); i++)
				path = path + " " + tokens[i];
		}

		// if statement for upper metioned purpose
		if (contact == 0)
		{
			path += "\\";
			contact++;
		}
	}
	void Client_5(SOCKET fd)
	{
		string s = string("5");
		char *t = strdup(s.c_str());

		int error = send(fd, t, strlen(t), 0);
		free(t);
		vector<string> tokens;
		while (true)
		{
			char result[SND_BUF];
			memset(result, '\0', SND_BUF);

			// Get User Options
			string input = getInput();

			//process input
			tokens.clear();
			tokens = utils::split(input, " ");

			//send input to server
			send(fd, input.c_str(), input.size(), 0);

			//hanlde input in client

			//end function
			if (tokens[0] == "end")
				break;

			//list computer drives
			if (tokens[0] == "listall")
			{
				cout << "Drives in computer: \n";
			}

			// if user want to access folder
			if (tokens[0] == "cd")
			{
				cout << "[+] -> folder" << endl
					 << "[-] -> file" << endl;
				cout << "------------------------" << endl;
			}

			// see transfer/delete/rename files
			if (tokens[0] == "see")
			{
				cout << "Available transfer/delete/rename files:\n";
				int error = recv(fd, result, SND_BUF, 0);

				if (error < 0)
				{
					cerr << "Error: receive  error.\n";
					memset(result, '\0', SND_BUF);
					return;
				}

				cout << result << endl;

				//-----------------------------------------------

				//get user command for transfer/delete/rename
				string input = getCommand();

				//send input to server
				send(fd, input.c_str(), input.size(), 0);

				// hanlde input in client
				tokens.clear();
				tokens = utils::split(input, " ");
				if (tokens[0] == "quit")
				{

					return;
				}

				if (tokens[0] == "transfer")
				{
					// transfer a.txt

					// file extension
					std::string extension_string = Client_recvFileExtension(fd);

					// save directory
					string saveDir = "client";
					string savePath = GeneratePNG_FileName(saveDir, "transfered", extension_string);

					//receiveFile
					receiveImage(savePath, fd);

				}
				if (tokens[0] == "rename" || tokens[0] == "delete")
				{
					//receive the result of rename/delete task
					int error = recv(fd, result, SND_BUF, 0);

					if (error < 0)
					{
						cerr << "Error: receive  error.\n";
						memset(result, '\0', SND_BUF);
						return;
					}

					//print result
					if (result == "1")
						cout << " Successfully\n";
					else
						cout << " Unsuccessfully\n";
				}

				return;
				// see available files that could be transfered
			}
			//receive the result
			int error = recv(fd, result, SND_BUF, 0);
			if (error < 0)
			{
				cerr << "Error: receive  error.\n";
				memset(result, '\0', SND_BUF);
				return;
			}
			cout << result << endl;

			//if statement for cd purpose. eg: wrong syntax/directory,...
			if (result[0] == '-')
			{
				cout << "Wrong input\n Exitting ...\n";
				return;
			}
		}
	}

	void Sever_5(SOCKET fd)
	{
		int contact = 0;
		while (true)
		{
			char buffer[SND_BUF] = {0};
			int valread = recv(fd, buffer, SND_BUF, 0);
			string draft;
			vector<string> tokens;

			// split input into strings
			tokens = spiltInput(buffer);
			if (tokens[0] == "end")
			{
				path.clear();
				break;
			}

			// first, list drives

			if (tokens[0] == "listall")
			{
				draft = PrintDrives();
			}

			if (tokens[0] == "rename")
			{

				string check = RenameFile(path, tokens);
				send(fd, check.c_str(), check.size(), 0);
				if (check == "1")
					cout << "Renamed successfully\n";
				else
					cout << "Renamed unsuccessfully\n";
				path.clear();
			}

			// if user want to access more
			if (tokens[0] == "cd" || tokens[0] == "transfer" || tokens[0] == "delete")
			{
				//process path for further purposes.
				Server_ProcessPath(path, tokens, contact);

				//browse next folder purpose
				if (tokens[0] == "cd")
				{
					if (!IsValidPath(path))
					{
						draft = "-1";
						int error = send(fd, draft.c_str(), draft.size(), 0);
						path.clear();
						return;
					}

					draft = print_directory1(path);

					if (contact != 1)
						path += "\\";
					else
						contact++;
				}
				else if (tokens[0] == "transfer")
				{
					// transfer file purpose
					Server_Transfer(fd, path);
					path.clear();
					return;
				}
				else if (tokens[0] == "delete")
				{
					//delete file purpose
					string check = deleteFile(path);
					send(fd, check.c_str(), check.size(), 0);
					if (check == "1")
						cout << "Deleted successfully\n";
					else
						cout << "Deleted unsuccessfully\n";
					path.clear();
				}
			}

			if (tokens[0] == "quit")
				break;
			if (tokens[0] == "see")
			{
				// see available file
				draft = seeAvailableFiles(path);
			}

			char *tmp = strdup(draft.c_str());
			int error = send(fd, tmp, strlen(tmp), 0);
			free(tmp);
		}

		path.clear();
	}
}
