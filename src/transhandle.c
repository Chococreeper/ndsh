#include "ndserver.h"
#include "transhandle.h"

#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

int recv_cli_header(transHeader_t *headerp, thr_dat_t *info)
{
    int ret;
    do
    {
        ret = recv(info->fd, headerp, sizeof(transHeader_t), MSG_WAITALL);
        if (0 == ret)
            return ERR_DISCONNECT;
    } while (ret < sizeof(transHeader_t));
    return 0;
}

int recv_cli_data(uint8_t **data, uint64_t *ndata, transHeader_t *header, thr_dat_t *info, int flag)
{
    if (flag && (header->datalen > *ndata))
    {
        if (*data != NULL)
            free(*data);
        *data = malloc(header->datalen);
        *ndata = header->datalen;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(info->fd, &fds);

    int ret, timeoutC = 0;

    struct timeval tout = {0};
    tout.tv_sec = TIMEOUT_SEC;
TOUT:
    /*超时三次返回错误码*/
    if (3 == timeoutC)
    {
        return ERR_TIMEOUT;
    }
    ret = select(info->fd + 1, &fds, NULL, NULL, &tout);
    if (!ret)
    {
        PRINT_MSG("Error: %s:%d recive data time out!\n", inet_ntoa(info->cliaddr.sin_addr), ntohs(info->cliaddr.sin_port));
        timeoutC++;
        goto TOUT;
    }
    if (FD_ISSET(info->fd, &fds))
    {
        ret = recv(info->fd, *data, header->datalen, MSG_WAITALL);
        if (ret <= 0)
        {
            return ERR_DISCONNECT;
        }
    }
    return 0;
}

int send_data(transHeader_t *headerp, void *data, thr_dat_t *info)
{
    send(info->fd, headerp, sizeof(transHeader_t), MSG_MORE);
    send(info->fd, data, headerp->datalen, 0);

    return 0;
}

int send_msg(char *msg, thr_dat_t *info)
{
    transHeader_t header;
    header.types = (HEADER_CODE << 32) | TYPE_MSG;
    header.datalen = strlen(msg) + 1;

    send_data(&header, msg, info);

    // send(info->fd, &header, sizeof(header), 0);
    // send(info->fd, msg, header.datalen, 0);

    return 0;
}

// 错误码数据包为8字节错误码，其后为错误信息
int send_err_code(uint64_t errcode, char *msg, thr_dat_t *info)
{
    transHeader_t header;
    header.types = (HEADER_CODE << 32) | TYPE_ERR;
    header.datalen = sizeof(errcode) + strlen(msg) + 1;

    send(info->fd, &header, sizeof(header), 0);
    send(info->fd, &errcode, sizeof(errcode), 0);
    send(info->fd, msg, header.datalen - sizeof(errcode), 0);

    return 0;
}

int header_types_is_ok(transHeader_t *header)
{
    if ((header->types >> 32) == HEADER_CODE)
        return 1;
    else
        return 0;
}

uint64_t recv_errcode(thr_dat_t *info)
{
    transHeader_t header;
    recv_cli_header(&header, info);

    uint8_t *data = NULL;
    uint64_t ndata = 0;

    recv_cli_data(&data, &ndata, &header, info, DATA_HEAP);
    if (TYPE_ERR == (header.types & 0xFFFF))
    {
        uint64_t errcode = *(uint64_t *)data;
        PRINT_MSG("%s\n", data + 8);
        if (data != NULL)
            free(data);
        return errcode;
    }
    if (data != NULL)
        free(data);
    return 0;
}
