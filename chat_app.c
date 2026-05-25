#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 1024

int partner[MAX_CLIENTS];
int waiting_client = -1;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_handler(void *arg)
{
    int client_fd = *(int *)arg;
    free(arg);

    pthread_mutex_lock(&queue_mutex);
    if (waiting_client == -1)
    {
        waiting_client = client_fd;
        pthread_mutex_unlock(&queue_mutex);

        char *wait_msg = "Waiting for a partner to join...\r\n";
        send(client_fd, wait_msg, strlen(wait_msg), 0);
    }
    else
    {
        int my_partner = waiting_client;
        partner[client_fd] = my_partner;
        partner[my_partner] = client_fd;
        waiting_client = -1;
        pthread_mutex_unlock(&queue_mutex);

        char *matched_msg = "You are paired! Start chatting:\r\n";
        send(client_fd, matched_msg, strlen(matched_msg), 0);
        send(my_partner, matched_msg, strlen(matched_msg), 0);
    }

    char buf[2048];
    char send_buf[2052];
    char recv_buf[2052];

    while (1)
    {
        int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0)
        {
            break;
        }
        buf[ret] = '\0';

        if (partner[client_fd] != -1)
        {
            snprintf(send_buf, sizeof(send_buf), ">> %s", buf);
            send(client_fd, send_buf, strlen(send_buf), 0);

            snprintf(recv_buf, sizeof(recv_buf), "<< %s", buf);
            send(partner[client_fd], recv_buf, strlen(recv_buf), 0);
        }
    }

    pthread_mutex_lock(&queue_mutex);
    if (waiting_client == client_fd)
    {
        waiting_client = -1;
    }

    int my_partner = partner[client_fd];
    if (my_partner != -1)
    {
        partner[my_partner] = -1;
        char *quit_msg = "Your partner has disconnected. Press Enter to exit...\r\n";
        send(my_partner, quit_msg, strlen(quit_msg), 0);
        shutdown(my_partner, SHUT_RDWR);
    }
    partner[client_fd] = -1;
    pthread_mutex_unlock(&queue_mutex);

    close(client_fd);

    return NULL;
}

int main()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        partner[i] = -1;
    }

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9090);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);

    while (1)
    {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd == -1)
            continue;

        int *arg = malloc(sizeof(int));
        *arg = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, arg);
        pthread_detach(tid);
    }

    close(listener);
    return 0;
}