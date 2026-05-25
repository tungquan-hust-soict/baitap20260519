#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>

void *client_handler(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    char buf[2048];
    int ret;

    char *welcome = "Nhap theo cu phap: GET_TIME [format]\nCac format ho tro: dd/mm/yyyy | dd/mm/yy | mm/dd/yyyy | mm/dd/yy\n> ";
    send(client_fd, welcome, strlen(welcome), 0);

    while (1)
    {
        ret = recv(client_fd, buf, sizeof(buf) - 1, 0);

        if (ret <= 0)
        {
            printf("[Thread %lu] Client disconnected.\n", pthread_self());
            break;
        }

        buf[ret] = 0;
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0)
        {
            send(client_fd, "> ", 2, 0);
            continue;
        }

        if (strncmp(buf, "GET_TIME ", 9) == 0)
        {
            char *format = buf + 9;
            time_t now = time(NULL);
            struct tm t;
            localtime_r(&now, &t);
            char res[64];

            if (strcmp(format, "dd/mm/yyyy") == 0)
            {
                strftime(res, sizeof(res), "%d/%m/%Y\n", &t);
                send(client_fd, res, strlen(res), 0);
            }
            else if (strcmp(format, "dd/mm/yy") == 0)
            {
                strftime(res, sizeof(res), "%d/%m/%y\n", &t);
                send(client_fd, res, strlen(res), 0);
            }
            else if (strcmp(format, "mm/dd/yyyy") == 0)
            {
                strftime(res, sizeof(res), "%m/%d/%Y\n", &t);
                send(client_fd, res, strlen(res), 0);
            }
            else if (strcmp(format, "mm/dd/yy") == 0)
            {
                strftime(res, sizeof(res), "%m/%d/%y\n", &t);
                send(client_fd, res, strlen(res), 0);
            }
            else
            {
                char *err_fmt = "Loi: Format khong duoc ho tro!\n";
                send(client_fd, err_fmt, strlen(err_fmt), 0);
            }
        }
        else
        {
            char *err_cmd = "Loi: Sai cu phap. Hay dung: GET_TIME [format]\n";
            send(client_fd, err_cmd, strlen(err_cmd), 0);
        }

        send(client_fd, "> ", 2, 0);
    }

    close(client_fd);
    return NULL;
}

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed.\n");
        exit(1);
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9090);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed.\n");
        exit(1);
    }

    if (listen(listener, 10) == -1)
    {
        perror("listen() failed.\n");
        exit(1);
    }

    printf("Time Server (Multithreading) is listening on port 9090...\n");

    while (1)
    {
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listener, NULL, NULL);
        if (*client_fd < 0)
        {
            perror("accept() failed.\n");
            free(client_fd);
            continue;
        }

        printf("New client connected. Creating thread...\n");

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, client_fd) != 0)
        {
            perror("pthread_create() failed");
            close(*client_fd);
            free(client_fd);
        }
        else
        {
            pthread_detach(tid);
        }
    }

    close(listener);
    return 0;
}