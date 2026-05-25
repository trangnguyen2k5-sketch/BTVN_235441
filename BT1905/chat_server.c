#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUF_SIZE 512

// Cấu trúc lưu trữ thông tin của mỗi client
typedef struct {
    int socket;
    char id[20];
    char name[50];
    int is_logged_in;
} ClientType;

ClientType clients[MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_handler(void *);
void broadcast_message(const char *msg, int sender_socket);
void remove_client(int socket);
void get_current_time(char *buffer, size_t size);
int is_id_duplicate(const char *id);

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
    
    if (listen(listener, 10)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    
    printf("Chat Server is listening on port 8080...\n");
    
    while (1) {
        int client_sock = accept(listener, NULL, NULL);
        if (client_sock == -1) {
            perror("accept() failed");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        if (num_clients < MAX_CLIENTS) {
            clients[num_clients].socket = client_sock;
            clients[num_clients].is_logged_in = 0;
            memset(clients[num_clients].id, 0, sizeof(clients[num_clients].id));
            memset(clients[num_clients].name, 0, sizeof(clients[num_clients].name));
            
            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, client_handler, (void *)(long)client_sock) == 0) {
                pthread_detach(thread_id);
                num_clients++;
                printf("New connection accepted. Socket fd: %d\n", client_sock);
            } else {
                close(client_sock);
            }
        } else {
            printf("Server full. Rejected connection.\n");
            close(client_sock);
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    close(listener);
    return 0;
}

// Luồng xử lý riêng cho từng client
void *client_handler(void *params) {
    int my_socket = (int)(long)params;
    char buf[BUF_SIZE];
    char client_id[20];   
    char client_name[50]; 
    int logged_in = 0;
    while (!logged_in) {
        char *ask_msg = "Vui long nhap ten theo cu phap 'client_id: client_name'\n";
        send(my_socket, ask_msg, strlen(ask_msg), 0);
        int len = recv(my_socket, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            remove_client(my_socket);
            pthread_exit(NULL);
        }
        buf[len] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;
        if (strlen(buf) == 0) continue;
        int parsed = sscanf(buf, "%19[^:]: %49s", client_id, client_name);

        if (parsed == 2) {
            pthread_mutex_lock(&clients_mutex);
            if (is_id_duplicate(client_id)) {
                pthread_mutex_unlock(&clients_mutex);
                char *dup_msg = "ID nay da co nguoi su dung! Vui long chon ID khac.\n";
                send(my_socket, dup_msg, strlen(dup_msg), 0);
                continue;
            }

            for (int i = 0; i < num_clients; i++) {
                if (clients[i].socket == my_socket) {
                    strcpy(clients[i].id, client_id);
                    strcpy(clients[i].name, client_name);
                    clients[i].is_logged_in = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            
            logged_in = 1;
            char welcome[150];
            sprintf(welcome, "Chao mung %s (ID: %s) den voi phong chat!\n", client_name, client_id);
            send(my_socket, welcome, strlen(welcome), 0);
            
            char join_msg[150];
            sprintf(join_msg, "He thong: %s (ID: %s) da tham gia phong chat.\n", client_name, client_id);
            broadcast_message(join_msg, my_socket);
        } else {
            char *err_msg = "He thong: Sai cu phap! Thu lai.\n";
            send(my_socket, err_msg, strlen(err_msg), 0);
        }
    }

    //Nhận tin nhắn và Broadcast công khai
    while (1) {
        int len = recv(my_socket, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            break; 
        }
        buf[len] = '\0';
        buf[strcspn(buf, "\r\n")] = 0; 

        if (strlen(buf) == 0) continue;

        char time_str[30];
        get_current_time(time_str, sizeof(time_str));

        char formatted_msg[BUF_SIZE + 100];
        snprintf(formatted_msg, sizeof(formatted_msg), "%s %s: %s\n", time_str, client_id, buf);

        printf("%s", formatted_msg);
        broadcast_message(formatted_msg, my_socket);
    }

    // Khi client ngắt kết nối
    char leave_msg[100];
    sprintf(leave_msg, "He thong: Client %s da roi phong chat.\n", client_id);
    broadcast_message(leave_msg, my_socket);
    
    remove_client(my_socket);
    pthread_exit(NULL);
}

int is_id_duplicate(const char *id) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].is_logged_in && strcmp(clients[i].id, id) == 0) {
            return 1;
        }
    }
    return 0;
}

void broadcast_message(const char *msg, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].socket != sender_socket && clients[i].is_logged_in) {
            send(clients[i].socket, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i].socket == socket) {
            close(socket);
            for (int j = i; j < num_clients - 1; j++) {
                clients[j] = clients[j + 1];
            }
            num_clients--;
            printf("Client fd %d removed. Remaining: %d\n", socket, num_clients);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void get_current_time(char *buffer, size_t size) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, size, "%Y/%m/%d %I:%M:%S%p", timeinfo);
}