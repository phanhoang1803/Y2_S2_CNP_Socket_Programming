// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
// #include <unistd.h>
#include <vector>
#include <iostream>
#include <string>
#include <windows.h>

#include "..\database.h"
#include "..\utils.h"
#include "..\UI.h"
#include "..\handle.h"

#include <sstream>

using namespace std;
using namespace handle;
using namespace utils;
using namespace UI;

// string rcv_data;
char sndBuf[100] = "Accept connection.";

int main(int argc, char *argv[])
{
    int error = -1;
    // Initialize Winsock library.
    initWinsockLib();

    // Initializing socket operator.
    SOCKET listenfd = -1;
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0)
        esc("listen", listenfd);

    struct sockaddr_in server_addr;
    // Server inf
    server_addr.sin_family = AF_INET;                        // Set address family
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Convert IP string to integer then set IP addr for server that we want to connect
    server_addr.sin_port = htons(PORT);                      // Set port number, convert, host to network short

    error = bind(listenfd, (sockaddr *)&server_addr, sizeof(server_addr));
    if (error < 0)
        esc("bind", listenfd);

    error = listen(listenfd, 3);
    if (error < 0)
        esc("listen", listenfd);

    SOCKET acceptfd = -1;
    // If the connection doesn't succeed, return errno (-1)
    int connectCount = 0;
    while (1)
    {
        cout << ++connectCount << ". Waiting for connection...";
        acceptfd = accept(listenfd, NULL, NULL);
        cout << " Connecting client... ";
        if (acceptfd == INVALID_SOCKET)
            esc("accept", listenfd, acceptfd);

        // Send list of features.
        stringstream builder;
        builder << "Successful connection.\n";
        string snd = builder.str();
        error = send(acceptfd, snd.c_str(), snd.size(), 0);
        if (error == SOCKET_ERROR)
            esc("send", listenfd, acceptfd);

        cout << "Responsed to client and closed connection.\n";

        while (1)
        {
            char rcvBuf[100] = {
                0,
            };
            // Receive request from client.

            error = recv(acceptfd, rcvBuf, 100, 0);
            if (error == -1)
                esc("recv", listenfd, acceptfd);
            if (error == 0)
            {
                cout << "Client has disconnected.\n";
                break; // Client disconnected
            }

            vector<string> tokens = split(string(rcvBuf), "-");

            // Control app toggle
            if (tokens[0] == "1")
            {

                int sign = 0;
                // Start or finish app.
                cout << "Tokens 2: " << tokens[2];
                string excutableFile = appsMap[tokens[2]];
                if (tokens[1] == "1")
                {
                    cout << string("start " + excutableFile) << endl;
                    sign = system(string("start " + excutableFile).c_str());
                }
                else
                {
                    cout << string("taskkill /F /im " + excutableFile) << endl;
                    sign = system(string("taskkill /F /im " + excutableFile).c_str());
                }

                // Send if success
                if (sign == 0)
                    error = send(acceptfd, "SUCCESS", 10, 0);
                else
                    error = send(acceptfd, "FAIL", 10, 0);
                if (error == SOCKET_ERROR)
                    esc("send", listenfd, acceptfd);
            }
            // Control process toggle
            else if (tokens[0] == "2")
            {
                // Sth
            }
            // Take a screenshot right now.
            else if (tokens[0] == "3")
            {
                int t = sendImage(captureAndSave(), acceptfd);
                if (!t)
                    cerr << "Error: send image error.\n";
            }
            // Catch key press.
            else if (tokens[0] == "4")
                Server_CatchKeyPresses(acceptfd);
            // Browse the directory tree.
            else if (tokens[0] == "5")
            {
                // Sth
            }
            else
            {
                error = send(acceptfd, "FAIL", snd.size(), 0);
                if (error == SOCKET_ERROR)
                    esc("send", listenfd, acceptfd);
            }
        }
    }

    return 0;
}
