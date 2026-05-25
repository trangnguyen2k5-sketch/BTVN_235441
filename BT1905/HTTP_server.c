#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_SIZE 2048

void *http_client_thread(void *params);

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }
    if (listen(listener, 20)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    
    printf("HTTP Server đa luồng đang chạy tại http://localhost:8080 ...\n");
    
    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) {
            perror("accept() failed");
            continue;
        }
        printf("\n[Server] Có kết nối mới, Socket fd: %d\n", client);
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, http_client_thread, (void *)(long)client) == 0) {
            pthread_detach(thread_id);
        } else {
            perror("pthread_create() failed");
            close(client);
        }
    }

    close(listener);
    return 0;
}

void *http_client_thread(void *params) {
    int client = (int)(long)params;
    char buf[BUF_SIZE];
    int ret = recv(client, buf, sizeof(buf) - 1, 0);
    if (ret > 0) {
        buf[ret] = '\0';
        printf("--- HTTP REQUEST FROM CLIENT (fd %d) ---\n", client);
        printf("%s", buf);
        printf("----------------------------------------\n");
        char *msg = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=UTF-8\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "<html>"
                    "<head><title>HTTP Multithread Server</title></head>"
                    "<body>"
                    "<h1>Xin chao cac ban!</h1>"
                    "</body>"
                    "</html>";
                    
        send(client, msg, strlen(msg), 0);
    } else if (ret == -1) {
        perror("recv() failed");
    }
    close(client);
    printf("[Server] Đã xử lý xong và đóng kết nối với Socket fd: %d\n", client);
    
    pthread_exit(NULL);
}