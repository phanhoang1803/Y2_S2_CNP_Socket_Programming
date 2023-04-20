#pragma once

#include <windows.h>
#include <psapi.h>
#include <string>
#include <vector>

#include "utils.h"

// std::string ListProcesses()
// {
//     std::string command = "tasklist /fo csv /nh";
//     std::string result = "";

//     FILE* pipe = _popen(command.c_str(), "r");
//     if (!pipe)
//         return;

//     char buffer[128];
//     while (fgets(buffer, sizeof(buffer), pipe))
//         result += buffer;

//     _pclose(pipe);

//     // Get the result from csv format
//     int start = 0;
//     while (start < result.size())
//     {
//         int end = result.find("\n", start);
//         std::string line = result.substr(start, end - start);
//         int pos = line.find(",");
//         if (pos != std::string::npos)
//         {
//             std::string processName = line.substr(1, pos - 2);
//             std::cout << "Process name: " << processName << std::endl;
//         }
//         start = end + 1;
//     }
// }

namespace prcCtr {
    std::string ListProcess() {
        std::string command("tasklist");
        std::string result = "";

        FILE* pipe = _popen(command.c_str(), "r");
        if (!pipe)
            return "";

        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe))
            result += buffer;

        _pclose(pipe);

        return result;
    }

    bool TerminateProcess_(DWORD pid) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess == NULL)
            return false;

        BOOL result = TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);

        return (result == TRUE);
    }
}