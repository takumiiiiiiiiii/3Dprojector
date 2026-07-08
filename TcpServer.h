#pragma once

#include <string>

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();

    bool Start(int port);
    int Receive(char* buffer, int size);
    void Stop();

private:
    int serverSocket;
    int clientSocket;
};