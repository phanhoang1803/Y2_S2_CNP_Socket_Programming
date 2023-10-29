# -Y2-S2-CNP-Socket-Programming
- Requirements:
  + Check your g++ version (You can check by typing [g++ --version] on your cmd), 
    Currently. We're using VSCode with [g++ (MinGW-W64 x86_64-ucrt-posix-seh, built by Brecht Sanders) 12.2.0] version.
  + With IP Adress, you can use '127.0.0.1' or your customized IP address for both client and server to testing on 1 computer
    or set another ID that is compatible with your needs.
  + ...

- To run this programme, move to paticular client/server directory:

    +           g++ -o server.exe server.cpp -lws2_32 -lgdi32 -lgdiplus           // Compile Server
    +           g++ -o client.exe client.cpp -lws2_32 -lgdi32 -lgdiplus           // Compile Client
    +           .\server.exe                                                      // Run Server
    +           .\client.exe                                                      // Run Client-
- Demo:
  [Video](https://youtu.be/I3tQlatmGN0?si=MpEtJfgpe5vUzz13)

