#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVER_HOST "lebavui.io.vn"
#define SERVER_PORT 21
#define MSSV "20235441"
#define NGAY_SINH "18"
#define BUFFER_SIZE 4096
//Nhận phản hồi từ FTP Server
void receive_response(int sock, char *response) {
    memset(response, 0, BUFFER_SIZE);
    int n = recv(sock, response, BUFFER_SIZE - 1, 0);
    if(n > 0) {
        response[n] = '\0';
        printf("%s", response);
    }
}
//Gửi lệnh FTP
void send_command(int sock,
                  const char *command,
                  char *response)
{
    printf(">> %s", command);
    send(sock, command, strlen(command), 0);
    receive_response(sock, response);
}
//Tạo kết nối TCP
int create_connection(const char *ip, int port) {
    int sock;
    struct sockaddr_in server;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        perror("socket");
        exit(1);
    }
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);
    if(connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        exit(1);
    }
    return sock;
}
//Mở kênh dữ liệu ở chế độ PASV
int open_data_connection(int control_sock)
{
    char response[BUFFER_SIZE];
    send_command(control_sock, "PASV\r\n", response);
    int h1,h2,h3,h4,p1,p2;
    char *begin = strchr(response,'(');
    if(begin == NULL) {
        return -1;
    }
    sscanf(begin + 1, "%d,%d,%d,%d,%d,%d", &h1,&h2,&h3,&h4,&p1,&p2);
    char ip[64];
    sprintf(ip, "%d.%d.%d.%d", h1,h2,h3,h4);
    int port = p1 * 256 + p2;
    return create_connection(ip, port);
}
//Tạo mật khẩu 4 số cuối MSSV + ngày sinh
void create_password(char *password) {
    sprintf(password,
            "%s%s",
            &MSSV[strlen(MSSV)-4],
            NGAY_SINH);
}
//Đảo ngược chuỗi
void reverse_string(char *str) {
    int left = 0;
    int right = strlen(str) - 1;
    while(left < right) {
        char temp = str[left];
        str[left] = str[right];
        str[right] = temp;
        left++;
        right--;
    }
}

int main() {
    struct hostent *host;
    //Phân giải tên miền
    host = gethostbyname(SERVER_HOST);
    if(host == NULL) {
        herror("gethostbyname");
        return 1;
    }
    char server_ip[64];
    inet_ntop(AF_INET,
              host->h_addr_list[0],
              server_ip,
              sizeof(server_ip));
    //Kết nối FTP Server
    int control_sock = create_connection(server_ip, SERVER_PORT);
    char response[BUFFER_SIZE];
    receive_response(control_sock, response);
    //Tạo username và password
    char username[64];
    char password[32];
    sprintf(username, "user_%s", MSSV);
    create_password(password);
    char cmd[256];
    //User
    sprintf(cmd, "USER %s\r\n", username);
    send_command(control_sock, cmd, response);
    //Pass
    sprintf(cmd, "PASS %s\r\n", password);
    send_command(control_sock, cmd, response);
    //Lấy danh sách file
    int data_sock = open_data_connection(control_sock);
    send_command(control_sock, "LIST\r\n", response);
    char file_list[20000];
    int total = 0;
    int n;
    while((n = recv(data_sock, 
                    file_list + total, 
                    sizeof(file_list)-total-1, 
                    0)) > 0) {
        total += n;
    }
    file_list[total] = '\0';
    close(data_sock);
    receive_response(control_sock, response);
    printf("\n===== FILE LIST =====\n");
    printf("%s\n", file_list);
    //Tìm File question
    char question_file[256];
    char *ptr = strstr(file_list, "question_");
    if(ptr == NULL) {
        printf("Khong tim thay file question!\n");
        close(control_sock);
        return 1;
    }
    sscanf(ptr, "%s", question_file);
    printf("\nQuestion file: %s\n", question_file);
    //Download file question
    data_sock = open_data_connection(control_sock);
    sprintf(cmd, "RETR %s\r\n", question_file);
    send_command(control_sock, cmd, response);
    char content[2048];
    total = 0;
    while((n = recv(data_sock,
                    content + total,
                    sizeof(content)-total-1,
                    0)) > 0) {
        total += n;
    }
    content[total] = '\0';
    close(data_sock);
    receive_response(control_sock, response);
    //Lưu file question
    FILE *fq = fopen(question_file,"w");
    if(fq != NULL) {
        fputs(content, fq);
        fclose(fq);
    }
    printf("\nOriginal Content:\n%s\n", content);
    //Xóa ký tự xuống dòng cuối
    while(total > 0 && (content[total-1]=='\n' || content[total-1]=='\r')) {
        content[--total] = '\0';
    }
    //Đảo chuỗi
    reverse_string(content);
    //Tạo file Answer
    char answer_file[256];
    sprintf(answer_file, "answer%s", strchr(question_file,'_'));
    //Lưu file answer
    FILE *fa = fopen(answer_file,"w");
    if(fa != NULL) {
        fputs(content, fa);
        fclose(fa);
    }
    printf("\nAnswer File: %s\n", answer_file);
    printf("Reversed Content:\n%s\n", content);
    //Upload file Answer
    data_sock = open_data_connection(control_sock);
    sprintf(cmd, "STOR %s\r\n", answer_file);
    send_command(control_sock, cmd, response);
    send(data_sock, content, strlen(content), 0);
    shutdown(data_sock, SHUT_WR);
    close(data_sock);
    receive_response(control_sock, response);
    //Thoát FTP
    send_command(control_sock, "QUIT\r\n", response);
    close(control_sock);
    printf("\nUpload completed successfully.\n");
    return 0;
}

