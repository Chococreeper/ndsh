// Net Disk Server C

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "ndserver.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <libgen.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

#include "cmdhandle.h"
#include "transhandle.h"
#include "typehandle.h"

void *cli_task(thr_dat_t *);
void *term_task(void *);

static sem_t term_task_sem[2];

int netdisk_main(char *path, char *ip, port_t port)
{
    int sockfd = sock_init(ip, port);
    ERRP(sockfd, "sock_init");

    PRINT_MSG("socket init completed\n");
    PRINT_MSG("IP:%s\n", ip);
    PRINT_MSG("PORT:%d\n", port);
    PRINT_MSG("listen...\n");

    int ret;
    fd_set fds, fds_b;
    char buf[256];
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    FD_SET(sockfd, &fds);
    fds_b = fds;

    // 客户端读缓存阻塞
    sem_init(&term_task_sem[0], 0, 1);
    sem_init(&term_task_sem[1], 0, 0);

    pthread_t term_thr;
    pthread_create(&term_thr, NULL, term_task, buf);

    while (1)
    {
        ret = select(sockfd + 1, &fds, NULL, NULL, NULL);
        ERRP(ret, "select");

        if (FD_ISSET(STDIN_FILENO, &fds))
        {
            sem_wait(&term_task_sem[0]);
            fgets(buf, sizeof(buf), stdin);
            sem_post(&term_task_sem[1]);
        }
        if (FD_ISSET(sockfd, &fds))
        {
            thr_dat_t *dat = malloc(sizeof(thr_dat_t));
            strcpy(dat->pwd, path);

            socklen_t clilen = sizeof(thr_dat_t);
            dat->fd = accept(sockfd, &(dat->cliaddr), &clilen);
            if (dat->fd != -1)
            {
                pthread_t tid;
                pthread_create(&tid, NULL, (void *(*)(void *))cli_task, dat);
                pthread_detach(tid);
            }
        }
        fds = fds_b;
    }
}

int sock_init(char *ip, __uint16_t port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ERR(sockfd);

    int ret = 1;

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret));
    ERR(ret);

    struct sockaddr_in sdr =
        {
            .sin_addr.s_addr = inet_addr(ip),
            .sin_family = AF_INET,
            .sin_port = htons(port)};

    ret = bind(sockfd, (void *)&sdr, sizeof(sdr));
    ERR(ret);

    ret = listen(sockfd, N_CONNECT);
    ERR(ret);

    return sockfd;
}

// 客户端处理函数
void *cli_task(thr_dat_t *dat)
{
    transHeader_t header;

    int retv;

    uint8_t *recvData = NULL;
    uint64_t DataMemSize = 0;

    PRINT_MSG("New Connect From:\n");
    PRINT_MSG("\t%s: %d\n", inet_ntoa(dat->cliaddr.sin_addr), ntohs(dat->cliaddr.sin_port));

    while (1)
    {
        recv(dat->fd, &header, sizeof(header), MSG_WAITALL);
        if (!header_types_is_ok(&header))
            break;
        retv = recv_cli_data(&recvData, &DataMemSize, &header, dat, DATA_HEAP);
        switch (retv)
        {
        case ERR_TIMEOUT:
            cmd_exit(dat, recvData);
            return NULL;
        case ERR_DISCONNECT:
            cmd_exit(dat, recvData);
            return NULL;
        default:
            break;
        }
        // 对命令与数据接收后执行
        switch (header.types & 0xffff)
        {
        case TYPE_CMD:
            handle_command(&header, recvData, dat);
            break;
        case TYPE_MSG:
            handle_message(&header, recvData, dat);
            break;
        case TYPE_RES:
            handle_respond(&header, recvData, dat);
            break;
        default:
            break;
        }
        free(recvData);
        recvData = NULL;
        DataMemSize = 0;
    }
    close(dat->fd);
}

// 输入处理函数
void *term_task(void *arg)
{
    while (1)
    {
        sem_wait(&term_task_sem[1]);
        if (!strncasecmp((char *)arg, "quit", 4))
            exit(0);
        sem_post(&term_task_sem[0]);
    }
}