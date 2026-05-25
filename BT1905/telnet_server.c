#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_SIZE 512
#define DB_FILE "users.txt"

void *client_handler(void *);
int check_login(const char *username, const char *password);
void send_file_content(int socket, const char *filename);

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
    
    printf("Telnet Server is listening on port 8080...\n");
    
    while (1) {
        int client_sock = accept(listener, NULL, NULL);
        if (client_sock == -1) {
            perror("accept() failed");
            continue;
        }
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_handler, (void *)(long)client_sock) == 0) {
            pthread_detach(thread_id);
            printf("Client connected on socket fd: %d\n", client_sock);
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
    int logged_in = 0;

    while (!logged_in) {
        char *welcome_msg = "He thong: Vui long dang nhap theo cu phap 'username password'\n";
        send(my_socket, welcome_msg, strlen(welcome_msg), 0);

        int len = recv(my_socket, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            close(my_socket);
            pthread_exit(NULL);
        }
        buf[len] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0) continue;

        char input_user[50] = {0};
        char input_pass[50] = {0};
        int parsed = sscanf(buf, "%49s %49s", input_user, input_pass);

        if (parsed == 2) {
            if (check_login(input_user, input_pass)) {
                logged_in = 1;
                char *success_msg = "Dang nhap thanh cong! Thuc hien lenh:\n";
                send(my_socket, success_msg, strlen(success_msg), 0);
            } else {
                char *fail_msg = "Sai username hoac password! Thu lai.\n";
                send(my_socket, fail_msg, strlen(fail_msg), 0);
            }
        } else {
            char *syntax_msg = "Nhap sai dinh dang! Thong tin phai bao gom user va pass.\n";
            send(my_socket, syntax_msg, strlen(syntax_msg), 0);
        }
    }

    char out_filename[32];
    snprintf(out_filename, sizeof(out_filename), "out_%d.txt", my_socket);

    while (1) {
        char *prompt = "telnet@server$ ";
        send(my_socket, prompt, strlen(prompt), 0);
        int len = recv(my_socket, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            break;
        }
        buf[len] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0) continue;
        if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) {
            char *bye_msg = "Tam biet!\n";
            send(my_socket, bye_msg, strlen(bye_msg), 0);
            break;
        }

        char command[BUF_SIZE + 64];
        snprintf(command, sizeof(command), "%s > %s 2>&1", buf, out_filename);
        printf("Executing command from client %d: %s\n", my_socket, buf);
        system(command);
        send_file_content(my_socket, out_filename);
    }
    unlink(out_filename);
    close(my_socket);
    printf("Client fd %d disconnected.\n", my_socket);
    pthread_exit(NULL);
}

// Hàm kiểm tra thông tin đăng nhập từ file dữ liệu
int check_login(const char *username, const char *password) {
    FILE *f = fopen(DB_FILE, "r");
    if (f == NULL) {
        perror("Could not open database file");
        return 0;
    }

    char file_user[50];
    char file_pass[50];
    int match = 0;

    // Đọc từng dòng của file cho tới hết
    while (fscanf(f, "%49s %49s", file_user, file_pass) == 2) {
        if (strcmp(username, file_user) == 0 && strcmp(password, file_pass) == 0) {
            match = 1; // Khớp tài khoản
            break;
        }
    }

    fclose(f);
    return match;
}

// Hàm bổ trợ đọc nội dung file văn bản và truyền qua socket
void send_file_content(int socket, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        char *err = "He thong: Khong the doc ket qua lenh.\n";
        send(socket, err, strlen(err), 0);
        return;
    }

    char file_buf[BUF_SIZE];
    size_t bytes_read;

    // Đọc theo khối dữ liệu và gửi liên tục đến khi hết file
    while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), f)) > 0) {
        send(socket, file_buf, bytes_read, 0);
    }

    fclose(f);
}