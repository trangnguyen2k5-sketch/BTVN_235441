#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Sử dụng: %s <IP Server> <Cổng>\n", argv[0]);
        return 1;
    }
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Kết nối thất bại");
        return 1;
    }
    printf("Đã kết nối. Nhập chuỗi để gửi (Ctrl+C để thoát):\n");
    char buffer[BUFFER_SIZE];
    while (1) {
        printf("> ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) > 0) {
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }
    close(client_sock);
    return 0;
}
