#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    printf("Connecting to %s:%d...\n", ip, port);
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }
    printf("Connected successfully!\n");
    char buffer[1024];
    while (1) {
        printf("Enter message to send (Type EXIT to quit): ");
        fgets(buffer, sizeof(buffer), stdin);
        if (send(client_sock, buffer, strlen(buffer), 0) < 0) {
            perror("send() failed");
            break;
        }
        if (strncmp(buffer, "EXIT", 4) == 0) {
            printf("Disconnecting...\n");
            break;
        }
    }
    close(client_sock);
    printf("Connection closed.\n");
    return 0;
}