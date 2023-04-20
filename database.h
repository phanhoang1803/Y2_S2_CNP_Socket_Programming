#pragma once

#include <string>
#include <vector>
#include <map>

#define PORT 49720

// 1 start, 0 finish
std::map<std::string, std::string> appsMap ={
    {"Google Chrome", "chrome.exe"},
    {"Microsoft Edge", "msedge.exe"},
    {"Foxit Reader", "FoxitPDFReader.exe"},
    {"Microsoft Word", "winword.exe"},
    {"Microsoft Excel", "excel.exe"},
    {"Microsoft PowerPoint", "powerpnt.exe"},
    {"Notepad", "Notepad.exe"},
    {"Windows Explorer", "explorer.exe"},
    {"VLC Media Player", "vlc.exe"},
    {"Visual Studio Code", "E:/\"Microsoft VS Code\"/Code.exe"}
    // Add extra data
};

std::vector<std::string> featureList ={
    "Control start/finish application.",
    "Control start/finish process.",
    "Take a screenshot right now.",
    "Catch key press.",
    "Browse the directory tree."
};