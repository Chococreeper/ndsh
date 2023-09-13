#define _GNU_SOURCE
#include "cmdhandle.h"
#include "transhandle.h"

#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef NOT_PRINT_MSG
#define PRINT_MSG(...) printf(__VA_ARGS__)
#else
#define PRINT_MSG(msg)
#endif

static long *filelist[1024] = {NULL};

char file_type(unsigned char type)
{
    switch (type)
    {
    case DT_DIR:
        return 'd';
    case DT_REG:
        return '-';
    default:
        return 'u';
    }
}

int cmd_getlist(thr_dat_t *info)
{
    DIR *curdir = opendir(info->pwd);
    struct dirent *file;
    char buf[256];

    if (curdir == NULL)
    {
        sprintf(buf, "opendir:%s", strerror(errno));
        send_msg(buf, info);
        return -1;
    }

    sprintf(buf, "%s:", info->pwd);
    send_msg(buf, info);

    while (1)
    {
        file = readdir(curdir);
        if (NULL == file)
            break;
        if (!strncmp(file->d_name, "..", 1) || !strncmp(file->d_name, "..", 2))
            continue;
        sprintf(buf, ">>>%ld %c %s", telldir(curdir), file_type(file->d_type), file->d_name);
        send_msg(buf, info);
    }
    closedir(curdir);
    return 0;
}

void rm_relative_path(char *path)
{
    char *substr, *substr_f;
    while (substr = strstr(path, ".."))
    {
        substr_f = substr - 2;
        while ('/' != *(substr_f))
        {
            substr_f--;
            if (substr_f < path)
            {
                strcpy(path, ".");
                return;
            }
        }
        memmove(substr_f, substr + 2, strlen(substr + 2) + 1);
    }
    if (*(path += (strlen(path) - 1)) == '/')
        *path = '\0';
}

int cmd_changedir(thr_dat_t *info, uint8_t *data)
{
    char modPath[256];

    // data前8位为命令字符串，向后偏移8字节获得路径
    data += 8;
    if ('~' == *data) // home dir
    {
        modPath[0] = '.';
        strcpy(modPath + 1, data + 1);
    }
    else
    {
        strcpy(modPath, info->pwd);
        strcat(modPath, "/");
        strcat(modPath, data);
        rm_relative_path(modPath);
    }

    struct stat filestat;

    if (-1 == stat(modPath, &filestat) || !S_ISDIR(filestat.st_mode))
    { // failure
        send_msg("Error Path!", info);
        return -1;
    }
    else
    { // successful
        strcpy(info->pwd, modPath);
        sprintf(modPath, "working dir change to:\n\t%s", info->pwd);
        send_msg(modPath, info);
    }
}

int cmd_pwd(thr_dat_t *info)
{
    char buf[256];

    sprintf(buf, "Current directory: %s", info->pwd);

    send_msg(buf, info);
    return 0;
}

int cmd_getfile(thr_dat_t *info, uint8_t *data, int flag)
{
    char filePath[256];
    data += 8;
    if ('~' == *data) // home dir
    {
        filePath[0] = '.';
        strcpy(filePath + 1, data + 1);
    }
    else
    {
        strcpy(filePath, info->pwd);
        strcat(filePath, "/");
        strcat(filePath, data);
        rm_relative_path(filePath);
    }
    struct stat filestat;
    int ret = stat(filePath, &filestat);
    if (ret == -1 || !S_ISREG(filestat.st_mode))
    {
        send_err_code(FILE_NOT_EXIST, strerror(errno), info);
        return -1;
    }
    send_err_code(FILE_GET_OK, basename(filePath), info);

    if (flag && recv_errcode(info) != FILE_UP_OK)
        return -1;

    int fd = open(filePath, O_RDONLY);
    transHeader_t header = {.types = (HEADER_CODE << 32) | TYPE_FILE};
    fileData_t *filedata = malloc(sizeof(fileData_t));
    filedata->flag = FILE_MORE;

    while (1)
    {
        header.datalen = read(fd, filedata->data, sizeof(filedata->data));
        if (header.datalen < sizeof(filedata->data))
        {
            filedata->flag = FILE_END;
            header.datalen += 8;
            send_data(&header, filedata, info);
            break;
        }
        header.datalen += 8;
        send_data(&header, filedata, info);
    }
    free(filedata);
    close(fd);
    return 0;
}

// upload 命令数据包低八位为命令
// 其后所有数据为路径
// 传输文件时低8位是传输标识
// 其后所有数据为文件内容
int cmd_upload(thr_dat_t *info, uint8_t *data, int flag)
{
    char filePath[256];
    data += 8;
    if ('~' == *data) // home dir
    {
        filePath[0] = '.';
        strcpy(filePath + 1, data + 1);
    }
    else
    {
        strcpy(filePath, info->pwd);
        strcat(filePath, "/");
        strcat(filePath, data);
        rm_relative_path(filePath);
    }
    int fd = open(filePath, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (-1 == fd)
    {
        send_err_code(FILE_EXIST, strerror(errno), info);
        return -1;
    }
    send_err_code(FILE_UP_OK, basename(filePath), info);

    if (flag && recv_errcode(info) != FILE_GET_OK)
        return -1;

    transHeader_t header;
    fileData_t *filedata = malloc(sizeof(fileData_t));
    uint64_t ndata = sizeof(filedata->data);

    while (1)
    {
        if (0 != recv_cli_header(&header, info))
            return -1;
        if (0 != recv_cli_data((void *)&filedata, &ndata, &header, info, DATA_STACK))
            return -1;
        write(fd, filedata->data, header.datalen - 8);
        if (FILE_END == filedata->flag)
            break;
    }
    free(filedata);
    close(fd);
    return 0;
}

// remove 命令数据包前8位为命令
// 其后数据为文件路径
int cmd_remove(thr_dat_t *info, uint8_t *data)
{
    char filePath[256];
    data += 8;
    if ('~' == *data) // home dir
    {
        filePath[0] = '.';
        strcpy(filePath + 1, data + 1);
    }
    else
    {
        strcpy(filePath, info->pwd);
        strcat(filePath, "/");
        strcat(filePath, data);
        rm_relative_path(filePath);
    }

    int ret = unlink(filePath);
    if (ret != 0)
    {
        send_err_code(FILE_NOT_EXIST, strerror(errno), info);
        return -1;
    }
    return 0;
}

int cmd_exit(thr_dat_t *info, uint8_t *data)
{
    send_err_code(DISCONNECT, "disconnected!", info);
    free(info);
    close(info->fd);
    free(data);
    pthread_exit(NULL);
}