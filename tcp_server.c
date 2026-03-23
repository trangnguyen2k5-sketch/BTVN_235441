#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <port> <greeting_file> <log_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    char *file_chao = argv[2];
    char *file_luu = argv[3];
    //đọc file greeting
    FILE *f_hello = fopen(file_chao, "r");
    if (f_hello == NULL) {
        perror("fopen() failed");
        exit(EXIT_FAILURE);
    }
    char hello[BUFFER_SIZE] = {0};
    fread(hello, 1, BUFFER_SIZE - 1, f_hello);
    fclose(f_hello);
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        close(listener);
        exit(EXIT_FAILURE);
    }
    if (listen(listener, 5) < 0) {
        perror("listen() failed");
        close(listener);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", port);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept() failed");
            continue;
        }
        printf("Client connected from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        if (send(client_sock, hello, strlen(hello), 0) < 0) {
            perror("send() failed");
            close(client_sock);
            continue;
        }
        //mở file log
        FILE *f_log = fopen(file_luu, "a");
        if (f_log == NULL) {
            perror("fopen() failed");
            close(client_sock);
            continue;
        }
        char buffer[BUFFER_SIZE];
        int bytes_received;
        //nhận dữ liệu
        while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            printf("Received %d bytes: %s", bytes_received, buffer);
            fputs(buffer, f_log);
            fflush(f_log);
        }
        if (bytes_received < 0) {
            perror("recv() failed");
        }
        printf("Client disconnected.\n");
        fclose(f_log);
        close(client_sock);
    }
    close(listener);
    return 0;
}