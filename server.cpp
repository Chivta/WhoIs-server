#include <thread>
#include <iostream>
#include "TCPSocket.h"
#include <chrono>
#include <iomanip>

void handle_connection(TCPSocket sock){
    auto data = sock.recv(4096); 
    sock.send(data);  
}

int main(){

    TCPSocket sock = TCPSocket{};
    sock.close();

    sock.create();
    sock.bind("127.0.0.1",43);
    sock.listen(); 
    while (true) {
        auto connection = sock.accept();

        if (connection.isValid()) {
            std::thread t(handle_connection, std::move(connection));
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);

            std::clog << "[" << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << "] " << "new connection: " << sock.userIP() << '\n';
            t.detach();
        } else {
            std::cerr << "Connection socket is invalid\n";
        }
    }
    return 0;
}

