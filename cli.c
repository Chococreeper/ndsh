#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ndserver.h"
#include "transhandle.h"
#include "cmdhandle.h"
#include "typehandle.h"

int input(int fd);
int sockinput(int fd);

int handle_err(transHeader_t *header, uint8_t *data, thr_dat_t *info);

int main(int argc, char *argv[])
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in caddr = {
        .sin_addr = inet_addr(argv[1]),
        .sin_port = htons(atoi(argv[2])),
        .sin_family = AF_INET};

    int ret = connect(fd, (void *)&caddr, sizeof(caddr));
    if (ret)
    {
        perror("connect:");
        return -1;
    }

    fd_set fds, fdsb;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    FD_SET(fd, &fds);
    fdsb = fds;

    while (1)
    {
        int ret = select(fd + 1, &fds, NULL, NULL, NULL);
        if (ret <= 0)
        {
            printf("disconnect!");
            return 0;
        }

        if (FD_ISSET(STDIN_FILENO, &fds))
            input(fd);

        if (FD_ISSET(fd, &fds))
            sockinput(fd);

        fds = fdsb;
    }

    return 0;
}

int sockinput(int fd)
{
    transHeader_t header;
    thr_dat_t info;

    uint8_t *data = NULL;
    uint64_t ndata = 0;

    info.fd = fd;
    recv_cli_header(&header, &info);
    if (!header_types_is_ok(&header))
        return -1;
    recv_cli_data(&data, &ndata, &header, &info, DATA_HEAP);
    switch (header.types & 0xffff)
    {
    case TYPE_CMD:
        break;
    case TYPE_MSG:
        handle_message(&header, data, &info);
        break;
    case TYPE_ERR:
        handle_err(&header, data, &info);
        break;
    case TYPE_FILE:
        break;
    default:
        break;
    }
    if (data != NULL)
        free(data);
}

int handle_err(transHeader_t *header, uint8_t *data, thr_dat_t *info)
{
    printf("%s\n", data + 8);
    switch (*(uint64_t *)data)
    {
    case DISCONNECT:
        exit(0);
        break;

    case FILE_GET_OK:
        strcpy(info->pwd, ".");
        cmd_upload(info, data, CLIENT);
        break;
    case FILE_UP_OK:
        strcpy(info->pwd, ".");
        cmd_getfile(info, data, CLIENT);
    default:
        break;
    }
}

int input(int fd)
{
    char cmd[64];
    scanf("%s", cmd);
    getchar();

    transHeader_t header;
    thr_dat_t info;
    info.fd = fd;

    if (!strncmp(cmd, "ls", 2))
    {

        char data[8] = "GETLIST";
        header.types = (HEADER_CODE << 32) | TYPE_CMD;
        header.datalen = sizeof(data);

        send_data(&header, data, &info);
    }

    if (!strncmp(cmd, "pwd", 3))
    {

        char data[8] = "PRIWKDIR";
        header.types = (HEADER_CODE << 32) | TYPE_CMD;
        header.datalen = sizeof(data);

        send_data(&header, data, &info);
    }

    if (!strncmp(cmd, "exit", 4))
    {

        char data[8] = "EXIT";
        header.types = (HEADER_CODE << 32) | TYPE_CMD;
        header.datalen = sizeof(data);

        send_data(&header, data, &info);
    }

    if (!strncmp(cmd, "cd", 2))
    {
        char data[256] = "CHNGEDIR";
        fgets(data + 8, sizeof(data) - 8, stdin);
        (data + 8)[strlen(data + 8) - 1] = '\0';

        header.types = (HEADER_CODE << 32) | TYPE_CMD;
        header.datalen = 8 + strlen(data + 8) + 1;

        send_data(&header, data, &info);
    }

    if (!strncmp(cmd, "rm", 2))
    {
        char data[256] = "REMOVE";
        fgets(data + 8, sizeof(data) - 8, stdin);
        (data + 8)[strlen(data + 8) - 1] = '\0';

        header.types = (HEADER_CODE << 32) | TYPE_CMD;
        header.datalen = 8 + strlen(data + 8) + 1;

        send_data(&header, data, &info);
    }

    if (!strncmp(cmd, "msg", 3))
    {
        char data[256];
        fgets(data, sizeof(data), stdin);
        data[strlen(data) - 1] = '\0';

        send_msg(data, &info);
    }

    if (!strncmp(cmd, "get", 3) || !strncmp(cmd, "pull", 4))
    {
        char data[256] = "GETFILE";
        fgets(data + 8, sizeof(data) - 8, stdin);
        (data + 8)[strlen(data + 8) - 1] = '\0';

        header.types = (HEADER_CODE << 32) | TYPE_CMD;
        header.datalen = 8 + strlen(data + 8) + 1;
        send_data(&header, data, &info);
    }

    if (!strncmp(cmd, "push", 4))
    {
        char data[256] = "UPLOAD";
        scanf("%s", data + 8);
        getchar();

        header.types = (HEADER_CODE << 32) | TYPE_CMD;
        header.datalen = 8 + strlen(data + 8) + 1;
        send_data(&header, data, &info);
    }

    return 0;
}