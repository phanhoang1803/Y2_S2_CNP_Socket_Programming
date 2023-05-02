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
#include "utils.h"
#include "handle.h"

using namespace std;
// using namespace utils;

#define CHECK_SIZE 10

namespace UI
{
	// UI
	void printFeatureList(vector<string> list)
	{
		int i = 1;
		for (auto feature : list)
			cout << i++ << ". " << feature << endl;
	}

	template <class T1, class T2>
	void printMap(map<T1, T2> listMap)
	{
		int i = 1;
		for (auto it : listMap)
			cout << i++ << ". " << it.first << endl;
	}

	bool controlApp(SOCKET fd, bool start, string appName)
	{
		string s = string("1-") + (start ? "1-" : "0-") + appName;
		cout << "command: " << s << endl;
		char *tmp = strdup(s.c_str());
		int error = send(fd, tmp, strlen(tmp), 0);
		free(tmp);
		if (error == SOCKET_ERROR)
		{
			cerr << "Error: send error\n";
			return false;
		}

		char response[CHECK_SIZE];
		error = recv(fd, response, CHECK_SIZE, 0);
		return !(error == SOCKET_ERROR && strcmp(response, "FAIL") == 0);
	}

	void controlAppMenu(SOCKET fd)
	{
		cout << "\t\t\t-----------List Of Applications-----------\n";

		UI::printMap(appsMap);
		int choice = -1;
		int start = true;

		while (1)
		{
			do
			{
				cout << "\nPlease enter a number of the app in the list you want to start/finish (or enter 0 to quit): ";
				cin >> choice;
				if (choice == 0)
					return;
				choice--;

			} while (choice < 0 || choice >= appsMap.size());
			auto it = appsMap.begin();
			std::advance(it, choice);
			fflush(stdin);
			string appName = it->first;
			cout << "Do you want to start (other 0) or finish (enter 0) " << appName << "? ";
			fflush(stdin);
			cin >> start;
			if (!controlApp(fd, start, appName)) 
				cerr << "Fail to controling app.\n";
		}
	}

	bool listProcess(SOCKET fd) {
		int error = send(fd, "2-", 2, 0);
		char response[64000];
		error = recv(fd, response, 64000, 0);
		if (error == SOCKET_ERROR) return false;
		std::cout << response;
		return true;
	}

	void processControlMenu(SOCKET fd) {
		cout << "\t\t\t-----------List Of Processess-----------\n";
		if (listProcess(fd)) {
			int pid;
			std::cout << "Enter process's ID of which you want to terminate: ";
			std::cin >> pid;
			int error = send(fd, (char*)&pid, sizeof(pid), 0);

			char response[CHECK_SIZE];
			error = recv(fd, response, CHECK_SIZE, 0);
			if (error == SOCKET_ERROR)
				std::cout << "Fail to terminate process!" << std::endl;
			else 
				std::cout << "Process was terminated!" << std::endl;
		}
	}

	void menu(SOCKET fd)
	{
		while (1)
		{
			// system("cls");
			cout << "\t\t\t----------- Remote Computer Control Program -----------\n";
			cout << "Here is a list of features that we currently offer:\n";
			UI::printFeatureList(featureList);
			int choice = -1;
			cout << "\nEnter your choice: ";
			cin >> choice;

			switch (choice)
			{
			case 1:
				UI::controlAppMenu(fd);
				break;
			case 2:
				UI::processControlMenu(fd);
				break;
			case 3:
				handle::captureScreen(fd);
				break;
			case 4:
				handle::Client_CatchKeyPresses(fd);
				break;
			case 5:
				handle::Client_5(fd);
				break;
			default:
				cout << "Exiting the programme...\n";
				return;
			}
		}
	}
}
