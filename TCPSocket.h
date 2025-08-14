#include <string>
#include <optional>

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

using clientAddr_t = std::optional<sockaddr_in>;

class TCPSocket {
private:
    SocketHandle sock;
    clientAddr_t clientAddr;
public:
    TCPSocket();
    explicit TCPSocket(SocketHandle existingSock);      // default
    TCPSocket(TCPSocket&& other) noexcept;              // move
    TCPSocket& operator=(TCPSocket&& other) noexcept;   // move asigment
    TCPSocket(const TCPSocket&) = delete;               // copy 
    TCPSocket& operator=(const TCPSocket&) = delete;    // copy asignent
    ~TCPSocket();   

    void create();
    void bind(const std::string &ip, uint16_t port);
    void listen(int backlog);
    TCPSocket accept();
    void connect(const std::string &ip, uint16_t port);
    ssize_t send(const std::string &data);
    std::string recv(size_t size);
    bool isValid() const;
    SocketHandle getHandle() const;
    void close();
    std::string userIP() const{
        if (clientAddr.has_value()){
            return inet_ntoa(clientAddr.value().sin_addr);
        }else{
            return {};
        }
    }
};


TCPSocket::TCPSocket() : sock(INVALID_SOCKET), clientAddr(std::nullopt) {}

TCPSocket::TCPSocket(SocketHandle existingSock) : sock(existingSock) {}

TCPSocket::TCPSocket(TCPSocket&& other) noexcept
    : sock(other.sock) {
    other.sock = INVALID_SOCKET;
}
TCPSocket::~TCPSocket() {
    close();
}
TCPSocket& TCPSocket::operator=(TCPSocket&& other) noexcept {
    if (this != &other) {
        close();             
        sock = other.sock;  
        other.sock = INVALID_SOCKET;
    }
    return *this;
}


void TCPSocket::create() {
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
    #endif
    sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
    throw std::runtime_error("create failed: " + std::string(strerror(errno)));
    }
}

void TCPSocket::bind(const std::string &ip, uint16_t port) {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr)); 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip.empty() ? INADDR_ANY : inet_addr(ip.c_str());
    clientAddr = addr;
    if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_CODE) {
        throw std::runtime_error("bind failed: "+ std::string(strerror(errno)));
    }
}

void TCPSocket::listen(int backlog = 5) {
    if (::listen(sock, backlog) == SOCKET_ERROR_CODE) {
        throw std::runtime_error("listen failed: "+ std::string(strerror(errno)));
    }
}

TCPSocket TCPSocket::accept() {
    sockaddr_in clientAddr;
    #ifdef _WIN32
    int len = sizeof(clientAddr);
    #else
    socklen_t len = sizeof(clientAddr);
    #endif
    SocketHandle clientSock = ::accept(sock, (sockaddr*)&clientAddr, &len);
    if (clientSock == INVALID_SOCKET) {
        throw std::runtime_error("accept failed: "+ std::string(strerror(errno)));
    }

    return TCPSocket(clientSock); // return a new socket object
}


void TCPSocket::connect(const std::string &ip, uint16_t port) {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (::connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_CODE) {
        throw std::runtime_error("connect failed: "+ std::string(strerror(errno)));
    }
}


ssize_t TCPSocket::send(const std::string &data) {
    ssize_t bytesSent = ::send(sock, data.c_str(), data.size(), 0);
    if (bytesSent == SOCKET_ERROR_CODE) {
        throw std::runtime_error("send failed: "+ std::string(strerror(errno)));
    }

    return bytesSent;
}


std::string TCPSocket::recv(size_t size) {
    std::vector<char> buffer(size);
    ssize_t bytesReceived = ::recv(sock, buffer.data(), buffer.size(), 0);
    
    if (bytesReceived > 0) {
        return std::string(buffer.data(), bytesReceived);
    } 
    else if (bytesReceived == 0) {
        return {};
    } 
    else {
        throw std::runtime_error(std::string("recv failed: ") + std::strerror(errno));
    }
}


void TCPSocket::close() {
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

bool TCPSocket::isValid() const {
    return sock != INVALID_SOCKET;
}

SocketHandle TCPSocket::getHandle() const {
    return sock;
}
