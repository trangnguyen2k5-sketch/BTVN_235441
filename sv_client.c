#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

struct sinhvien {
    char mssv[16];
    char hoten[64];
    char ngaysinh[16];
    float dtb;
};
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP_address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }
    printf("Connected successfully!\n");
    while (1) {
        struct sinhvien sv;
        memset(&sv, 0, sizeof(sv));
        printf("\nNhap thong tin sinh vien\n");
        printf("MSSV (Nhap '0' de thoat): ");
        scanf("%15s", sv.mssv);
        if (strcmp(sv.mssv, "0") == 0) {
            printf("Dang ngat ket noi...\n");
            break;
        }
        while(getchar() != '\n'); 
        printf("Ho ten: ");
        fgets(sv.hoten, sizeof(sv.hoten), stdin);
        sv.hoten[strcspn(sv.hoten, "\n")] = 0; 
        printf("Ngay sinh (YYYY-MM-DD): ");
        scanf("%15s", sv.ngaysinh);
        printf("Diem trung binh: ");
        scanf("%f", &sv.dtb);
        if (send(client_sock, &sv, sizeof(sv), 0) < 0) {
            perror("send() failed");
            break; 
        } else {
            printf(">> Da gui thong tin thanh cong!\n");
        }
    }
    close(client_sock);
    return 0;
}