#include "PlatformSocket.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
// Windows-specific includes isolated here
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Windows platform data structure
struct Win32SocketData {
    SOCKET serverSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    std::string host;
    int port = 0;
    bool serverOpen = false;
    bool clientConnected = false;
    std::string clientInfo;

    static bool wsaInitialized;
    static int wsaRefCount;
};

bool Win32SocketData::wsaInitialized = false;
int Win32SocketData::wsaRefCount = 0;

#else
// Linux includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// Linux platform data structure
struct LinuxSocketData {
    int serverSocket = -1;
    int clientSocket = -1;
    std::string host;
    int port = 0;
    bool serverOpen = false;
    bool clientConnected = false;
    std::string clientInfo;
};
#endif

namespace SiphogLib {

    PlatformSocket::PlatformSocket() {
#ifdef _WIN32
        platformHandle = new Win32SocketData();
#else
        platformHandle = new LinuxSocketData();
#endif
    }

    PlatformSocket::~PlatformSocket() {
        closeServer();
#ifdef _WIN32
        delete static_cast<Win32SocketData*>(platformHandle);
#else
        delete static_cast<LinuxSocketData*>(platformHandle);
#endif
    }

    bool PlatformSocket::initializePlatform() {
#ifdef _WIN32
        Win32SocketData::wsaRefCount++;
        if (!Win32SocketData::wsaInitialized) {
            WSADATA wsaData;
            int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (result != 0) {
                std::cerr << "WSAStartup failed: " << result << std::endl;
                return false;
            }
            Win32SocketData::wsaInitialized = true;
        }
#endif
        return true;
    }

    void PlatformSocket::cleanupPlatform() {
#ifdef _WIN32
        Win32SocketData::wsaRefCount--;
        if (Win32SocketData::wsaRefCount <= 0 && Win32SocketData::wsaInitialized) {
            WSACleanup();
            Win32SocketData::wsaInitialized = false;
            Win32SocketData::wsaRefCount = 0;
        }
#endif
    }

    bool PlatformSocket::createServer(const std::string& host, int port) {
        if (isServerOpen()) {
            return false; // Already open
        }

        if (!initializePlatform()) {
            return false;
        }

#ifdef _WIN32
        Win32SocketData* data = static_cast<Win32SocketData*>(platformHandle);
        data->host = host;
        data->port = port;

        data->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (data->serverSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create server socket" << std::endl;
            return false;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.S_un.S_addr = inet_addr(host.c_str());

        if (bind(data->serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Failed to bind server socket" << std::endl;
            closesocket(data->serverSocket);
            data->serverSocket = INVALID_SOCKET;
            return false;
        }

        if (listen(data->serverSocket, 2) == SOCKET_ERROR) {
            std::cerr << "Failed to listen on server socket" << std::endl;
            closesocket(data->serverSocket);
            data->serverSocket = INVALID_SOCKET;
            return false;
        }

        data->serverOpen = true;

#else
        LinuxSocketData* data = static_cast<LinuxSocketData*>(platformHandle);
        data->host = host;
        data->port = port;

        data->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (data->serverSocket < 0) {
            std::cerr << "Failed to create server socket" << std::endl;
            return false;
        }

        // Allow socket reuse
        int opt = 1;
        setsockopt(data->serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

        if (bind(data->serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Failed to bind server socket" << std::endl;
            close(data->serverSocket);
            data->serverSocket = -1;
            return false;
        }

        if (listen(data->serverSocket, 2) < 0) {
            std::cerr << "Failed to listen on server socket" << std::endl;
            close(data->serverSocket);
            data->serverSocket = -1;
            return false;
        }

        data->serverOpen = true;
#endif

        return true;
    }

    bool PlatformSocket::acceptClient() {
        if (!isServerOpen()) {
            return false;
        }

#ifdef _WIN32
        Win32SocketData* data = static_cast<Win32SocketData*>(platformHandle);

        // Set socket to non-blocking for accept
        u_long mode = 1; // 1 to enable non-blocking socket
        ioctlsocket(data->serverSocket, FIONBIO, &mode);

        struct sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);

        data->clientSocket = accept(data->serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (data->clientSocket == INVALID_SOCKET) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // No connection pending, this is normal
                return false;
            }
            std::cerr << "Accept failed with error: " << error << std::endl;
            return false;
        }

        // Set client socket back to blocking
        mode = 0; // 0 to disable non-blocking socket
        ioctlsocket(data->clientSocket, FIONBIO, &mode);

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        data->clientInfo = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));
        data->clientConnected = true;

        std::cout << "Client connected: " << data->clientInfo << std::endl;

#else
        LinuxSocketData* data = static_cast<LinuxSocketData*>(platformHandle);

        // Set socket to non-blocking
        int flags = fcntl(data->serverSocket, F_GETFL, 0);
        fcntl(data->serverSocket, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        data->clientSocket = accept(data->serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (data->clientSocket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connection pending, this is normal
                return false;
            }
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            return false;
        }

        // Set client socket back to blocking
        flags = fcntl(data->clientSocket, F_GETFL, 0);
        fcntl(data->clientSocket, F_SETFL, flags & ~O_NONBLOCK);

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        data->clientInfo = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));
        data->clientConnected = true;

        std::cout << "Client connected: " << data->clientInfo << std::endl;
#endif

        return true;
    }

    void PlatformSocket::closeClient() {
#ifdef _WIN32
        Win32SocketData* data = static_cast<Win32SocketData*>(platformHandle);
        if (data->clientSocket != INVALID_SOCKET) {
            closesocket(data->clientSocket);
            data->clientSocket = INVALID_SOCKET;
        }
        data->clientConnected = false;
        data->clientInfo.clear();
#else
        LinuxSocketData* data = static_cast<LinuxSocketData*>(platformHandle);
        if (data->clientSocket >= 0) {
            close(data->clientSocket);
            data->clientSocket = -1;
        }
        data->clientConnected = false;
        data->clientInfo.clear();
#endif
    }

    void PlatformSocket::closeServer() {
        closeClient();

#ifdef _WIN32
        Win32SocketData* data = static_cast<Win32SocketData*>(platformHandle);
        if (data->serverSocket != INVALID_SOCKET) {
            closesocket(data->serverSocket);
            data->serverSocket = INVALID_SOCKET;
        }
        data->serverOpen = false;
#else
        LinuxSocketData* data = static_cast<LinuxSocketData*>(platformHandle);
        if (data->serverSocket >= 0) {
            close(data->serverSocket);
            data->serverSocket = -1;
        }
        data->serverOpen = false;
#endif
    }

    bool PlatformSocket::isServerOpen() const {
#ifdef _WIN32
        Win32SocketData* data = static_cast<Win32SocketData*>(platformHandle);
        return data->serverOpen;
#else
        LinuxSocketData* data = static_cast<LinuxSocketData*>(platformHandle);
        return data->serverOpen;
#endif
    }

    bool PlatformSocket::isClientConnected() const {
#ifdef _WIN32
        Win32SocketData* data = static_cast<Win32SocketData*>(platformHandle);
        return data->clientConnected;
#else
        LinuxSocketData* data = static_cast<LinuxSocketData*>(platformHandle);
        return data->clientConnected;
#endif
    }

    int PlatformSocket::sendData(const std::string& data) {
        if (!isClientConnected()) {
            return -1;
        }

#ifdef _WIN32
        Win32SocketData* socketData = static_cast<Win32SocketData*>(platformHandle);
        int bytesSent = send(socketData->clientSocket, data.c_str(), static_cast<int>(data.length()), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Failed to send data to client" << std::endl;
            closeClient();
            return -1;
        }
        return bytesSent;
#else
        LinuxSocketData* socketData = static_cast<LinuxSocketData*>(platformHandle);
        ssize_t bytesSent = send(socketData->clientSocket, data.c_str(), data.length(), 0);
        if (bytesSent < 0) {
            std::cerr << "Failed to send data to client" << std::endl;
            closeClient();
            return -1;
        }
        return static_cast<int>(bytesSent);
#endif
    }

    std::string PlatformSocket::getClientInfo() const {
#ifdef _WIN32
        Win32SocketData* data = static_cast<Win32SocketData*>(platformHandle);
        return data->clientInfo;
#else
        LinuxSocketData* data = static_cast<LinuxSocketData*>(platformHandle);
        return data->clientInfo;
#endif
    }

} // namespace SiphogLib