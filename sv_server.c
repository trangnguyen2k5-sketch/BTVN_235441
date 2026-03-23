#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

struct sinhvien {
    char mssv[16];
    char hoten[64];
    char ngaysinh[16];
    float dtb;
};
void get_current_time(char *buffer, int size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <log_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    char *log_file = argv[2];
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }
    if (listen(listener, 5)) {
        perror("listen() failed");
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
        char *client_ip = inet_ntoa(client_addr.sin_addr);
        printf("\n[+] Client connected from %s\n", client_ip);
        while (1) {
            struct sinhvien sv;
            memset(&sv, 0, sizeof(sv)); 
            int bytes_received = recv(client_sock, &sv, sizeof(sv), 0);
            if (bytes_received <= 0) {
                printf("[-] Client %s disconnected.\n", client_ip);
                break; 
            }
            char time_str[64];
            get_current_time(time_str, sizeof(time_str));
            printf("Received: MSSV: %s | Name: %s | DOB: %s | GPA: %.2f\n", 
                   sv.mssv, sv.hoten, sv.ngaysinh, sv.dtb);
            FILE *f_log = fopen(log_file, "a");
            if (f_log == NULL) {
                perror("fopen() failed");
            } else {
                fprintf(f_log, "%s %s %s %s %s %.2f\n", client_ip, time_str, sv.mssv, sv.hoten, sv.ngaysinh, sv.dtb);
                fclose(f_log);
            }
        }
        close(client_sock); 
    }
    close(listener);
    return 0;
}