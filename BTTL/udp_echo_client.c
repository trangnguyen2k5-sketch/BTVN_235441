#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Sử dụng: %s <IP Server> <Port>\n", argv[0]);
        return 1;
    }

    // 1. Tạo socket UDP
    int client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    char buffer[BUFFER_SIZE];
    printf("Nhập tin nhắn (Ctrl+C để thoát):\n");

    while (1) {
        printf("> ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
        buffer[strcspn(buffer, "\n")] = 0; // Xóa ký tự xuống dòng

        // 2. Gửi dữ liệu cho Server dùng sendto
        sendto(client_sock, buffer, strlen(buffer), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));

        // 3. Nhận phản hồi (Echo) từ Server dùng recvfrom
        struct sockaddr_in from_addr;
        socklen_t addr_len = sizeof(from_addr);
        int bytes_received = recvfrom(client_sock, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&from_addr, &addr_len);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Server echo: %s\n", buffer);
        }
    }

    close(client_sock);
    return 0;
}
