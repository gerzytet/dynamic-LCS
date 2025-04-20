#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main() {
    const int PORT = 19921;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        char buffer[1024];
        int bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Received: " << buffer << std::endl;
            std::string response = "ACK\n";
            send(client_fd, response.c_str(), response.size(), 0);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
