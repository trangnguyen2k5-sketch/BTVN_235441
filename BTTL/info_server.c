#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int port = 8888;
    // Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // Bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    // Listen
    listen(server_fd, 5);
    printf("Server listening...\n");
    // Accept
    client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    //nhan thu muc
    int dir_len;
    recv(client_sock, &dir_len, sizeof(int), 0);
    char dir_name[256];
    recv(client_sock, dir_name, dir_len, 0);
    dir_name[dir_len] = '\0';
    printf("%s\n", dir_name);
    //nhan so file
    int count;
    recv(client_sock, &count, sizeof(int), 0);
    for (int i = 0; i < count; i++) {
        int name_len;
        recv(client_sock, &name_len, sizeof(int), 0);
        char name[256];
        recv(client_sock, name, name_len, 0);
        name[name_len] = '\0';
        long size;
        recv(client_sock, &size, sizeof(long), 0);
        printf("%s - %ld bytes\n", name, size);
    }
    close(client_sock);
    close(server_fd);
    return 0;
}