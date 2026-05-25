#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>

#define PORT 8080
#define BUF_SIZE 1024
#define STORAGE_DIR "./server_storage"

void *client_handler(void *);
int send_file_list(int socket);
void send_file_content(int socket, const char *filename);

int main() {
    mkdir(STORAGE_DIR, 0777);

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
    
    printf("File Server đa luồng đang chạy trên cổng %d...\n", PORT);
    printf("Thư mục lưu trữ định cấu hình: %s\n", STORAGE_DIR);
    
    while (1) {
        int client_sock = accept(listener, NULL, NULL);
        if (client_sock == -1) {
            perror("accept() failed");
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_handler, (void *)(long)client_sock) == 0) {
            pthread_detach(thread_id);
            printf("[Server] Chấp nhận kết nối mới. Socket fd: %d\n", client_sock);
        } else {
            close(client_sock);
        }
    }

    close(listener);
    return 0;
}

void *client_handler(void *params) {
    int my_socket = (int)(long)params;
    char buf[BUF_SIZE];
    int file_count = send_file_list(my_socket);
    if (file_count <= 0) {
        pthread_exit(NULL);
    }
    while (1) {
        int len = recv(my_socket, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            break;
        }
        buf[len] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;
        if (strlen(buf) == 0) continue;
        char filepath[BUF_SIZE + 50];
        snprintf(filepath, sizeof(filepath), "%s/%s", STORAGE_DIR, buf);

        struct stat st;
        if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
            char success_header[64];
            snprintf(success_header, sizeof(success_header), "OK %lld\r\n", st.st_size);
            send(my_socket, success_header, strlen(success_header), 0);
            printf("[Socket %d] Bắt đầu truyền file: %s (%lld bytes)\n", my_socket, buf, st.st_size);
            send_file_content(my_socket, filepath);
            break; 
        } else {
            char *err_msg = "ERROR File not found! Vui long gui lai ten file chinh xac.\r\n";
            send(my_socket, err_msg, strlen(err_msg), 0);
            printf("[Socket %d] Client yêu cầu file không tồn tại: %s\n", my_socket, buf);
        }
    }

    close(my_socket);
    printf("[Socket %d] Đã hoàn thành phiên làm việc và đóng kết nối.\n", my_socket);
    pthread_exit(NULL);
}

int send_file_list(int socket) {
    DIR *dir = opendir(STORAGE_DIR);
    if (dir == NULL) {
        char *err = "ERROR Cannot open storage directory\r\n";
        send(socket, err, strlen(err), 0);
        close(socket);
        return -1;
    }
    struct dirent *entry;
    char list_buf[BUF_SIZE * 4] = {0};
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        strcat(list_buf, entry->d_name);
        strcat(list_buf, "\r\n");
        count++;
    }
    closedir(dir);

    if (count == 0) {
        char *no_files_msg = "ERROR No files to download\r\n";
        send(socket, no_files_msg, strlen(no_files_msg), 0);
        close(socket);
        printf("[Socket %d] Đóng kết nối do hệ thống lưu trữ trống.\n", socket);
        return 0;
    }

    char header[64];
    snprintf(header, sizeof(header), "OK %d\r\n", count);
    send(socket, header, strlen(header), 0);
    strcat(list_buf, "\r\n");
    send(socket, list_buf, strlen(list_buf), 0);

    return count;
}

/**
 * @brief Đọc nội dung file ở chế độ nhị phân và đẩy qua luồng mạng mạng
 */
void send_file_content(int socket, const char *filename) {
    FILE *f = fopen(filename, "rb"); // Đọc ở chế độ nhị phân (Binary Mode)
    if (f == NULL) return;

    char file_buf[BUF_SIZE];
    size_t bytes_read;

    // Đọc liên tục từng khối dữ liệu từ file và đẩy trực tiếp vào socket
    while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f)) > 0) {
        send(socket, file_buf, bytes_read, 0);
    }

    fclose(f);
}