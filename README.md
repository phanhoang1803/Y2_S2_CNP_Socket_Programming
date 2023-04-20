# -Y2-S2-CNP-Socket-Programming
- Requirements:
  + Check your g++ version (You can check by typing [g++ --version] on your cmd), 
    Currently, I'm using VSCode with [g++ (MinGW-W64 x86_64-ucrt-posix-seh, built by Brecht Sanders) 12.2.0]
  + With the screenshot function, you need to change the path to save the screenshot to match your desktop.
  + With IP Adress, you can use '127.0.0.1' or your customized IP address for both client and server to testing on 1 computer.
  + ...

- To run this programme, move to paticular client/server directory: 
    +           g++ -o [.exe] [.cpp] -lws2_32 -lgdi32 -lgdiplus           // Compile
    +           [.exe]                                                    // Run prog
