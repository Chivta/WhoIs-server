#pragma once
#include <string>
#include <iostream>
#include <cstdint>
#include <cstring> // for memset
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET SocketHandle;
const int SOCKET_ERROR_CODE = SOCKET_ERROR;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
typedef int SocketHandle;
const SocketHandle INVALID_SOCKET = -1;
const int SOCKET_ERROR_CODE = -1;
#endif

class TCPSocket {
private:
    SocketHandle sock;

public:
    // Default constructor initializes the socket to invalid
    TCPSocket() : sock(INVALID_SOCKET) {}

    // Wrap an existing socket (used for accepted client connections)
    explicit TCPSocket(SocketHandle existingSock) : sock(existingSock) {}

    TCPSocket(TCPSocket&& other) noexcept
        : sock(other.sock) {
        other.sock = INVALID_SOCKET;
    }

    // Destructor ensures socket is closed
    ~TCPSocket() {
        close();
    }

    TCPSocket& operator=(TCPSocket&& other) noexcept {
        if (this != &other) {
            close();             // close current socket if open
            sock = other.sock;   // take ownership
            other.sock = INVALID_SOCKET;
        }
        return *this;
    }

    // Create a TCP socket
    bool create() {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::cerr << "WSAStartup failed\n";
            return false;
        }
#endif
        sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            std::cerr << "Failed to create socket\n";
            return false;
        }
        return true;
    }

    // Bind the socket to an IP and port
    bool bind(const std::string &ip, uint16_t port) {
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr)); 
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str());

        if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_CODE) {
            std::cerr << "Bind failed\n";
            return false;
        }
        return true;
    }

    // Listen for incoming connections
    bool listen(int backlog = 5) {
        if (::listen(sock, backlog) == SOCKET_ERROR_CODE) {
            std::cerr << "Listen failed\n";
            return false;
        }
        return true;
    }

    // Accept a new client connection
    TCPSocket accept() {
        sockaddr_in clientAddr;
#ifdef _WIN32
        int len = sizeof(clientAddr);
#else
        socklen_t len = sizeof(clientAddr);
#endif
        SocketHandle clientSock = ::accept(sock, (sockaddr*)&clientAddr, &len);
        if (clientSock == INVALID_SOCKET) {
            std::cerr << "Accept failed\n";
        }
        std::cout << "Received connection\n";

        return TCPSocket(clientSock); // return a new socket object
    }

    // Connect to a remote server
    bool connect(const std::string &ip, uint16_t port) {
        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_CODE) {
            std::cerr << "Connect failed\n";
            return false;
        }
        return true;
    }

    // Send data over the socket
    ssize_t send(const std::string &data) {
        ssize_t bytesSent = ::send(sock, data.c_str(), data.size(), 0);
        if (bytesSent == SOCKET_ERROR_CODE) {
            std::cerr << "Send failed\n";
        }
        return bytesSent;
    }

    // Receive up to maxlen bytes from the socket
    std::string recv(size_t size) {
        std::vector<char> buffer(size);
        ssize_t bytesReceived = ::recv(sock, buffer.data(), buffer.size(), 0);
        
        if (bytesReceived > 0) {
            return std::string(buffer.data(), bytesReceived);
        } 
        else if (bytesReceived == 0) {
            std::cout << "Client closed connection\n";
            return {};
        } 
        else {
            perror("recv");
            return {};
        }
}


    // Close the socket
    void close() {
        if (sock != INVALID_SOCKET) {
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            ::close(sock);
#endif
            sock = INVALID_SOCKET;
        }
    }

    // Check if socket is valid
    bool isValid() const {
        return sock != INVALID_SOCKET;
    }

    // Return the raw socket handle
    SocketHandle getHandle() const {
        return sock;
    }
};

void handle_connection(TCPSocket sock){
    while (true) {
        auto data = sock.recv(4096); // blocking call
        if (data.empty()) {          // 0 bytes means client closed or error
            std::cout << "Client disconnected\n";
            break;
        }
        std::cout << "Received: " << data << "\n";
        // Optionally send a response:
        // sock.send("Echo: " + data);
    }
}

int main(){

    TCPSocket sock = TCPSocket{};
    sock.close();

    sock.create();
    sock.bind("127.0.0.1",8080);
    sock.listen(); 
    while (true) {
        auto connection = sock.accept();

        if (connection.isValid()) {
            std::thread t(handle_connection, std::move(connection));
            t.detach();
        } else {
            std::cerr << "Connection socket is invalid\n";
        }
    }

    sock.close();
    return 0;
}

