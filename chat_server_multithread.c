#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

struct ClientInfo
{
    int fd;
    int state;
    char id[64];
};

struct ClientInfo clients[64];
int num_clients = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void removeClient(int fd)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++)
    {
        if (clients[i].fd == fd)
        {
            for (int j = i; j < num_clients - 1; j++)
            {
                clients[j] = clients[j + 1];
            }
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *thread_proc(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    char *ask = "Nhap ten theo cu phap client_id: client_name\n";
    send(client_fd, ask, strlen(ask), 0);

    char buf[2048];
    while (1)
    {
        int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0)
        {
            printf("Client %d disconnected\n", client_fd);
            removeClient(client_fd);
            close(client_fd);
            break;
        }

        buf[ret] = 0;

        pthread_mutex_lock(&clients_mutex);

        int my_idx = -1;
        for (int i = 0; i < num_clients; i++)
        {
            if (clients[i].fd == client_fd)
            {
                my_idx = i;
                break;
            }
        }

        if (my_idx != -1)
        {
            if (clients[my_idx].state == 0)
            {
                char name[64];
                if (sscanf(buf, "%[^:]: %[^\n]", clients[my_idx].id, name) == 2)
                {
                    printf("Dang nhap thanh cong. ID: %s, Ten: %s\n", clients[my_idx].id, name);
                    clients[my_idx].state = 1;
                    char *mes = "Da dang nhap thanh cong.\n";
                    send(client_fd, mes, strlen(mes), 0);
                }
                else
                {
                    char *mes = "Sai cu phap. Dang nhap lai!\n";
                    send(client_fd, mes, strlen(mes), 0);
                }
            }
            else
            {
                char mes[2500];
                snprintf(mes, sizeof(mes), "%s: %s", clients[my_idx].id, buf);
                for (int j = 0; j < num_clients; j++)
                {
                    if (j != my_idx && clients[j].state == 1)
                    {
                        send(clients[j].fd, mes, strlen(mes), 0);
                    }
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);
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

    printf("Server is listening on port 8080\n");

    while (1)
    {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd == -1)
            continue;

        printf("New client connected: %d\n", client_fd);

        pthread_mutex_lock(&clients_mutex);
        clients[num_clients].fd = client_fd;
        clients[num_clients].state = 0;
        num_clients++;
        pthread_mutex_unlock(&clients_mutex);

        int *arg = malloc(sizeof(int));
        *arg = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, thread_proc, arg);
        pthread_detach(tid);
    }

    close(listener);
    return 0;
}