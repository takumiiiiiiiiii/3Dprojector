#include "TcpServer.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#include <netinet/tcp.h>
TcpServer::TcpServer()
{
    serverSocket = -1;
    clientSocket = -1;
}

TcpServer::~TcpServer()
{
    Stop();
}

bool TcpServer::Start(int port)
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket < 0)
    {
        std::cerr << "[ERROR] socket() failed : "
                  << strerror(errno) << std::endl;
        return false;
    }

    std::cout << "[INFO] socket() OK" << std::endl;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if(bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "[ERROR] bind() failed : "
                  << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }

    std::cout << "[INFO] bind() OK" << std::endl;

    if(listen(serverSocket, 1) < 0)
    {
        std::cerr << "[ERROR] listen() failed : "
                  << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }

    std::cout << "[INFO] listen() OK" << std::endl;
    std::cout << "[INFO] Waiting for client..." << std::endl;

    sockaddr_in clientAddr{};
    socklen_t len = sizeof(clientAddr);

    clientSocket = accept(serverSocket,
                          (sockaddr*)&clientAddr,
                          &len);

    if(clientSocket < 0)
    {
        std::cerr << "[ERROR] accept() failed : "
                  << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }

    int flag = 1;
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY,
                   (char*)&flag, sizeof(flag)) < 0)
    {
        std::cerr << "[WARN] setsockopt(TCP_NODELAY) failed : "
                  << strerror(errno) << std::endl;
    }


    std::cout << "[INFO] Client Connected" << std::endl;

    return true;
}

int TcpServer::Receive(char* buffer, int size)
{
    return recv(clientSocket, buffer, size, 0);
}

void TcpServer::Stop()
{
    if(clientSocket != -1)
    {
        close(clientSocket);
        clientSocket = -1;
    }

    if(serverSocket != -1)
    {
        close(serverSocket);
        serverSocket = -1;
    }
}