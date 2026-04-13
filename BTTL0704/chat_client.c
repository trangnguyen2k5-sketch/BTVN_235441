#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define BUF_SIZE 512

int main() {
    int client = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(client, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        return 1;
    }
    printf("Connected to server!\n");
    fd_set fdread;
    char buf[BUF_SIZE];
    while (1) {
        FD_ZERO(&fdread);
        FD_SET(STDIN_FILENO, &fdread);
        FD_SET(client, &fdread);
        int maxfd = client;
        int ret = select(maxfd + 1, &fdread, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select error");
            break;
        }
        if (FD_ISSET(STDIN_FILENO, &fdread)) {
            fgets(buf, sizeof(buf), stdin);
            send(client, buf, strlen(buf), 0);
        }
        if (FD_ISSET(client, &fdread)) {
            int r = recv(client, buf, sizeof(buf) - 1, 0);
            if (r <= 0) {
                printf("Disconnected from server\n");
                break;
            }
            buf[r] = 0;
            printf("%s", buf);
        }
    }
    close(client);
    return 0;
}