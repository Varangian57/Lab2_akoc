#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/file.h>
#include <cerrno>

void log_message(const char* message) {
    int fd = open("chat_history.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        std::cerr << "Не удалось открыть файл истории: " << strerror(errno) << std::endl;
        return;
    }

    flock(fd, LOCK_EX);
    write(fd, message, strlen(message));
    write(fd, "\n", 1);
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
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Ошибка: не удалось привязать сокет к порту 8080" << std::endl;
        close(sockfd);
        return 1;
    }

    listen(sockfd, 1);
    std::cout << "Сервер запущен и ждёт подключений на порту 8080...\n";

    while (true) {
        int client_fd = accept(sockfd, nullptr, nullptr);
        if (client_fd < 0) {
            std::cerr << "Ошибка при приёме подключения" << std::endl;
            continue;
        }

        std::cout << "Клиент подключился.\n";

        while (true) {
            char buffer[256];
            size_t total = 0;

            while (total < sizeof(buffer) - 1) {
                ssize_t bytes_read = read(client_fd, buffer + total, sizeof(buffer) - 1 - total);
                if (bytes_read <= 0) {
                    std::cout << "Клиент отключился.\n";
                    close(client_fd);
                    goto next_client;
                }

                for (ssize_t i = 0; i < bytes_read; ++i) {
                    if (buffer[total + i] == '\n') {
                        buffer[total + i] = '\0';
                        total += i;
                        goto line_ready;
                    }
                }
                total += bytes_read;
            }
            buffer[total] = '\0';

        line_ready:
            std::string message(buffer);
            std::cout << "Получено сообщение от клиента: '" << message << "'\n";

            std::string response;
            if (message == "ping") {
                response = "pong";
            } else {
                response = message; 
            }

            std::string reply = response + "\n";
            write(client_fd, reply.c_str(), reply.size());

            log_message(("Сервер получил: " + message).c_str());
            log_message(("Сервер отправил: " + response).c_str());
        }

    next_client:;
    }

    close(sockfd);
    return 0;
}

