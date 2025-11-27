#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>


void show_history() {
    int fd = open("chat_history.txt", O_RDONLY);
    if (fd == -1) {
        std::cout << "История в данный момент пока пуста.\n";
        return;
    }

    flock(fd, LOCK_SH);
    char buffer[512];
    std::cout << "\n История переписки:\n";
    while (ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1)) {
        if (bytes_read <= 0) break;
        buffer[bytes_read] = '\0';
        std::cout << buffer;
    }
    std::cout << "\n";
    flock(fd, LOCK_UN);
    close(fd);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Ошибка: не удалось создать сокет" << std::endl;
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Не удалось подключиться к серверу, возможно он не запущен" << std::endl;
        close(sockfd);
        return 1;
    }

    std::cout << "Подключение к серверу. Введите сообщение (Ctrl+D или /exit для выхода):\n";

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/exit") break;

        std::string message = line + "\n";
        write(sockfd, message.c_str(), message.size());

        char buffer[512];
        size_t total = 0;
        while (total < sizeof(buffer) - 1) {
            ssize_t bytes_read = read(sockfd, buffer + total, sizeof(buffer) - 1 - total);
            if (bytes_read <= 0) {
                std::cerr << "Соединение с сервером разорвано.\n";
                close(sockfd);
                return 1;
            }

            for (ssize_t i = 0; i < bytes_read; ++i) {
                if (buffer[total + i] == '\n') {
                    buffer[total + i] = '\0';
                    total += i;
                    goto reply_ready;
                }
            }
            total += bytes_read;
        }
        buffer[total] = '\0';

    reply_ready:
        std::cout << "<-- Сервер: " << buffer << "\n";

        usleep(10000);
    }

    std::cout << "\n Происходит отключение...\n";
    close(sockfd);

    show_history();
    return 0;   
}