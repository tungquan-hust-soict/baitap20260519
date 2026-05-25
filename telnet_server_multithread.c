#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

int checkLogin(char *input)
{
    FILE *f = fopen("a.txt", "r");
    if (!f)
    {
        printf("Khong mo duoc file a.txt\n");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        line[strcspn(line, "\r\n")] = 0;

        if (strcmp(input, line) == 0)
        {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void *thread_proc(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    int state = 0;
    char buf[2048];

    char *ask = "Yeu cau dang nhap (Nhap 'user pass'):\n> ";
    send(client_fd, ask, strlen(ask), 0);

    while (1)
    {
        int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0)
        {
            printf("Client %d disconnected\n", client_fd);
            close(client_fd);
            break;
        }

        buf[ret] = 0;
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0)
        {
            send(client_fd, "> ", 2, 0);
            continue;
        }

        if (state == 0)
        {
            if (checkLogin(buf))
            {
                state = 1;
                char *msg_ok = "Dang nhap thanh cong! Hay go lenh:\n> ";
                send(client_fd, msg_ok, strlen(msg_ok), 0);
                printf("Client %d da dang nhap.\n", client_fd);
            }
            else
            {
                char *msg_fail = "Sai tai khoan. Hay thu lai:\n> ";
                send(client_fd, msg_fail, strlen(msg_fail), 0);
            }
        }
        else
        {
            char cmd[2500];
            char out_file[64];
            snprintf(out_file, sizeof(out_file), "out_%d.txt", client_fd);
            snprintf(cmd, sizeof(cmd), "%s > %s", buf, out_file);

            system(cmd);

            FILE *f = fopen(out_file, "r");
            if (f)
            {
                char file_buf[2048];
                int bytes_read = fread(file_buf, 1, sizeof(file_buf) - 1, f);
                if (bytes_read > 0)
                {
                    file_buf[bytes_read] = 0;
                    send(client_fd, file_buf, bytes_read, 0);
                }
                else
                {
                    char *mes = "Lenh da chay (khong co output).\n";
                    send(client_fd, mes, strlen(mes), 0);
                }
                fclose(f);
            }
            else
            {
                char *mes = "Loi khong the doc ket qua.\n";
                send(client_fd, mes, strlen(mes), 0);
            }
            remove(out_file);
            send(client_fd, "> ", 2, 0);
        }
    }
    return NULL;
}

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed\n");
        exit(1);
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed\n");
        exit(1);
    }

    if (listen(listener, 10) == -1)
    {
        perror("listen() failed\n");
        exit(1);
    }

    printf("Telnet Server is listening on port 8080\n");

    while (1)
    {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd == -1)
            continue;

        int *arg = malloc(sizeof(int));
        *arg = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, thread_proc, arg);
        pthread_detach(tid);
    }

    close(listener);
    return 0;
}