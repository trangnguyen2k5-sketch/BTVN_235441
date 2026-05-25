#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080
#define BUF_SIZE 512

typedef struct {
    int my_sock;
    int peer_sock;
} ChatPair;

void *forward_thread(void *params);
void match_clients(int new_sock);

int waiting_socket = -1; 
pthread_mutex_t pair_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    addr.sin_port = htons(PORT);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }
    
    if (listen(listener, 10)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    
    printf("Pair Chat Server đang chạy trên cổng %d...\n", PORT);
    printf("Đang chờ người dùng kết nối để thực hiện ghép cặp...\n");
    
    while (1) {
        int client_sock = accept(listener, NULL, NULL);
        if (client_sock == -1) {
            perror("accept() failed");
            continue;
        }
        match_clients(client_sock);
    }

    close(listener);
    return 0;
}

void match_clients(int new_sock) {
    pthread_mutex_lock(&pair_mutex);

    if (waiting_socket == -1) {
        waiting_socket = new_sock;
        printf("[Hàng đợi] Client fd %d đang đợi bạn chat...\n", new_sock);
    
        char *wait_msg = "Dang tim kiem ban chat... Vui long doi trong giay lat.\n";
        send(new_sock, wait_msg, strlen(wait_msg), 0);
        
        pthread_mutex_unlock(&pair_mutex);
    } 
    else {
        int peer_sock = waiting_socket;
        waiting_socket = -1;
        
        printf("[Ghép cặp] Thành công! Cặp đôi: (fd %d <---> fd %d)\n", peer_sock, new_sock);
        
        char *start_msg = "He thong: Da tim thay ban chat! Phong chat 2 nguoi bat dau. Hay go tin nhan.\n";
        send(peer_sock, start_msg, strlen(start_msg), 0);
        send(new_sock, start_msg, strlen(start_msg), 0);
        
        ChatPair *pair1 = malloc(sizeof(ChatPair));
        pair1->my_sock = peer_sock;
        pair1->peer_sock = new_sock;
        
        ChatPair *pair2 = malloc(sizeof(ChatPair));
        pair2->my_sock = new_sock;
        pair2->peer_sock = peer_sock;
        
        pthread_t t1, t2;
        pthread_create(&t1, NULL, forward_thread, (void *)pair1);
        pthread_create(&t2, NULL, forward_thread, (void *)pair2);
        pthread_detach(t1);
        pthread_detach(t2);
        pthread_mutex_unlock(&pair_mutex);
    }
}

void *forward_thread(void *params) {
    ChatPair *pair = (ChatPair *)params;
    int my_sock = pair->my_sock;
    int peer_sock = pair->peer_sock;
    char buf[BUF_SIZE];
    
    while (1) {
        int len = recv(my_sock, buf, sizeof(buf) - 1, 0);
        
        // Nếu 1 trong 2 client ngắt kết nối (len <= 0)
        if (len <= 0) {
            printf("[Ngắt kết nối] Client fd %d rời phòng.\n", my_sock);
            char *bye_msg = "He thong: Ban chat cua ban da roi phong. Ket noi dong.\n";
            send(peer_sock, bye_msg, strlen(bye_msg), 0);
            
            close(my_sock);
            close(peer_sock);
            break;
        }
        
        buf[len] = '\0';
        char send_buf[BUF_SIZE + 30];
        snprintf(send_buf, sizeof(send_buf), "Friend: %s", buf);
        send(peer_sock, send_buf, strlen(send_buf), 0);
    }
    
    free(pair); 
    pthread_exit(NULL);
}