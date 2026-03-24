#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Sử dụng: %s <Port>\n", argv[0]);
        return 1;
    }

    // 1. Tạo socket UDP (SOCK_DGRAM)
    int server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    // 2. Bind socket với cổng
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    printf("UDP Echo Server đang chạy tại cổng %s...\n", argv[1]);

    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        // 3. Nhận dữ liệu và lưu thông tin người gửi (client_addr)
        int bytes_received = recvfrom(server_sock, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&client_addr, &client_len);
        
        if (bytes_received < 0) break;
        buffer[bytes_received] = '\0';

        printf("Nhận từ %s: %s\n", inet_ntoa(client_addr.sin_addr), buffer);

        // 4. Phản hồi (Echo) lại chính dữ liệu đó cho người gửi
        sendto(server_sock, buffer, bytes_received, 0,
               (struct sockaddr *)&client_addr, client_len);
    }

    close(server_sock);
    return 0;
}
