#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080

double calculate(double a, double b, char op)
{
    switch(op)
    {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return a / b;
    }
    return 0;
}

void send_response(int client, char *html)
{
    char response[8192];
    sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n"
            "%s",
            strlen(html),
            html);
    send(client, response, strlen(response), 0);
}

char get_operator(char *op_str)
{
    if(strcmp(op_str, "add") == 0)
        return '+';
    if(strcmp(op_str, "sub") == 0)
        return '-';
    if(strcmp(op_str, "mul") == 0)
        return '*';
    if(strcmp(op_str, "div") == 0)
        return '/';
    return '?';
}

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in addr;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("HTTP Calculator Server listening on port %d\n", PORT);
    while(1)
    {
        client_fd = accept(server_fd, NULL, NULL);
        char buffer[8192];
        int len = recv(client_fd, buffer, sizeof(buffer)-1, 0);
        if(len <= 0)
        {
            close(client_fd);
            continue;
        }
        buffer[len] = '\0';
        printf("\n===== REQUEST =====\n");
        printf("%s\n", buffer);
        //Trang chủ
        if(strncmp(buffer, "GET / ", 6) == 0)
        {
            char html[] =
            "<html>"
            "<head><title>HTTP Calculator</title></head>"
            "<body>"

            "<h2>Calculator - GET</h2>"
            "<form method='GET' action='/calc'>"
            "A: <input type='number' step='any' name='a'><br><br>"
            "B: <input type='number' step='any' name='b'><br><br>"
            "<select name='op'>"
            "<option value='add'>+</option>"
            "<option value='sub'>-</option>"
            "<option value='mul'>*</option>"
            "<option value='div'>/</option>"
            "</select><br><br>"
            "<input type='submit' value='Tinh GET'>"
            "</form>"

            "<hr>"

            "<h2>Calculator - POST</h2>"
            "<form method='POST' action='/calc'>"
            "A: <input type='number' step='any' name='a'><br><br>"
            "B: <input type='number' step='any' name='b'><br><br>"
            "<select name='op'>"
            "<option value='add'>+</option>"
            "<option value='sub'>-</option>"
            "<option value='mul'>*</option>"
            "<option value='div'>/</option>"
            "</select><br><br>"
            "<input type='submit' value='Tinh POST'>"
            "</form>"

            "</body>"
            "</html>";

            send_response(client_fd, html);
        }

        //GET
        else if(strncmp(buffer, "GET /calc?", 10) == 0)
        {
            double a, b;
            char op_str[20];
            sscanf(buffer, "GET /calc?a=%lf&b=%lf&op=%19s", &a, &b, op_str);
            char *space = strchr(op_str, ' ');
            if(space)
                *space = '\0';
            char op = get_operator(op_str);
            char html[2048];

            if(op == '/' && b == 0)
            {
                sprintf(html,
                        "<html>"
                        "<body>"
                        "<h1>Loi</h1>"
                        "<p>Khong the chia cho 0</p>"
                        "<a href='/'>Quay lai</a>"
                        "</body>"
                        "</html>");
            }
            else
            {
                double result = calculate(a, b, op);
                sprintf(html,
                        "<html>"
                        "<body>"
                        "<h1>Ket qua GET</h1>"
                        "<p>%.2lf %c %.2lf = %.2lf</p>"
                        "<a href='/'>Quay lai</a>"
                        "</body>"
                        "</html>",
                        a,
                        op,
                        b,
                        result);
            }
            send_response(client_fd, html);
        }

        //POST
        else if(strncmp(buffer, "POST /calc", 10) == 0) {
            char *body = strstr(buffer, "\r\n\r\n");
            if(body) {
                body += 4;
                double a, b;
                char op_str[20];
                sscanf(body, "a=%lf&b=%lf&op=%19s", &a, &b, op_str);
                char op = get_operator(op_str);
                char html[2048];
                if(op == '/' && b == 0) {
                    sprintf(html,
                            "<html>"
                            "<body>"
                            "<h1>Loi</h1>"
                            "<p>Khong the chia cho 0</p>"
                            "<a href='/'>Quay lai</a>"
                            "</body>"
                            "</html>");
                }
                else {
                    double result = calculate(a, b, op);
                    sprintf(html,
                            "<html>"
                            "<body>"
                            "<h1>Ket qua POST</h1>"
                            "<p>%.2lf %c %.2lf = %.2lf</p>"
                            "<a href='/'>Quay lai</a>"
                            "</body>"
                            "</html>",
                            a,
                            op,
                            b,
                            result);
                }
                send_response(client_fd, html);
            }
        }
        close(client_fd);
    }
    close(server_fd);
    return 0;
}