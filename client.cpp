#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    const char* address = "127.0.0.1";
    const int port = 8080;
    const std::string message = "{\"msg\":\"hello\"}";

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in server = {};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, address, &server.sin_addr);

    if (connect(fd, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }

    size_t sent = 0;
    while (sent < message.size()) {
        ssize_t n = send(fd, message.data() + sent, message.size() - sent, 0);
        if (n < 0) {
            perror("send");
            close(fd);
            return 1;
        }
        sent += n;
    }

    shutdown(fd, SHUT_WR);

    std::string reply;
    char buffer[1024];
    while (true) {
        ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
        if (n < 0) {
            perror("recv");
            close(fd);
            return 1;
        }
        if (n == 0) {
            break;
        }
        reply.append(buffer, n);
    }

    std::cout << reply << std::endl;

    close(fd);
    return 0;
}
