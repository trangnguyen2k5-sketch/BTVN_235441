#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void signal_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 10)) {
        perror("listen() failed");
        return 1;
    }

    signal(SIGCHLD, signal_handler);

    printf("HTTP Server is running on port 8080...\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        
        if (client == -1) continue;

        printf("New connection from %s (socket %d)\n", inet_ntoa(client_addr.sin_addr), client);

        if (fork() == 0) {
            close(listener);

            char buf[2048];
            int ret = recv(client, buf, sizeof(buf) - 1, 0);
            if (ret > 0) {
                buf[ret] = 0;
                printf("Request from client %d:\n%s\n", client, buf);

                char *msg = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html; charset=utf-8\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "<html><body>"
                            "<h1>Xin chào</h1>"
                            "<p>Server đa tiến trình đã xử lý yêu cầu của bạn!</p>"
                            "</body></html>";
                
                send(client, msg, strlen(msg), 0);
            }

            close(client);
            printf("Child process handled and closed client %d.\n", client);
            exit(0);
        }

        close(client);
    }

    close(listener);
    return 0;
}