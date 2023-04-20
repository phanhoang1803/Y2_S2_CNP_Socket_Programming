#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>
// #include <unistd.h>
#include <cstring>
#include <string.h>
#include <map>
#include <sstream>
#include "..\database.h"
#include "..\utils.h"
#include "..\UI.h"
#include "..\handle.h"

using namespace std;
using namespace utils;
using namespace UI;
using namespace handle;

int main(int argc, char *argv[])
{
    // string rcv_data;
    char rcvBuf[2000] = {
        0,
    };

    int error = -1;
    // Initialize Winsock library.
    initWinsockLib();

    // Initializing socket operator.
    SOCKET speakfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (speakfd < 0)
        esc("socket", speakfd);

// 
    // Server inf that we want to connect.
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;                     // Set address family
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Convert IP string to integer then set IP addr for server that we want to connect
    server_addr.sin_port = htons(PORT);                   // Set port number, convert, host to network short

    error = connect(speakfd, (sockaddr *)&server_addr, sizeof(server_addr));

    // If the connection doesn't succeed, return errno (-1)
    if (error < 0)
        esc("connect", speakfd);

    // Receive Server's confirmation
    error = recv(speakfd, rcvBuf, 2000, 0);
    if (error < 0)
        esc("recv", speakfd);
    cout << "Server: " << rcvBuf << endl;

    
    menu(speakfd);


    closesocket(speakfd);
    WSACleanup();

    system("pause");

    return 0;
}
