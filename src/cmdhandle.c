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

// 文件夹seek
#define LIST_LEN 1024

// 文件序号表
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

// flag 为是否显示隐藏文件，即.开头的文件
// 0 为显示
// 1 为不显示
int cmd_getlist(thr_dat_t *info, int flag)
{
    DIR *curdir = opendir(info->pwd);
    struct dirent *file;
    char buf[512];

    if (curdir == NULL)
    {
        sprintf(buf, "opendir:%s", strerror(errno));
        send_msg(buf, info);
        goto ERR;
    }

    sprintf(buf, "-- %s :", info->pwd);
    send_msg(buf, info);
    if (filelist[info->fd] == NULL)
        filelist[info->fd] = malloc(LIST_LEN * sizeof(long));

    uint64_t count = 1;

    while (1)
    {
        if (count < LIST_LEN)
            filelist[info->fd][count - 1] = telldir(curdir);
        file = readdir(curdir);
        if (NULL == file)
            break;
        if (!strcmp(file->d_name, "..") || !strcmp(file->d_name, ".") || (flag && !strncmp(file->d_name, ".", 1)))
            continue;
        uint8_t *color = "0";
        switch (file->d_type)
        {
        case DT_DIR:
            color = "34";
            break;
        case DT_REG:
            color = "0";
        default:
            break;
        }
        sprintf(buf, "\033[33m>>>\033[0m%d\t%c \033[%sm%s\033[0m", count, file_type(file->d_type), color, file->d_name);

        count++;
        send_msg(buf, info);
    }
    closedir(curdir);
    return 0;
ERR:
    closedir(curdir);
    return -1;
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
    while (substr = strstr(path, "/./"))
    {
        substr_f = substr + 2;
        memmove(substr, substr_f, strlen(substr_f) + 1);
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
        sprintf(modPath, "working dir change to:\n\t\033[34m%s\033[0m", info->pwd);
        send_msg(modPath, info);
    }

    DIR *curdir = opendir(info->pwd);
    struct dirent *file;
    uint64_t count = 1;
    while (1)
    {
        if (count < LIST_LEN)
            filelist[info->fd][count - 1] = telldir(curdir);
        if (NULL == (file = readdir(curdir)))
            break;
        if (!strncmp(file->d_name, "..", 1) || !strncmp(file->d_name, "..", 2))
            continue;
        count++;
    }
    closedir(curdir);
    return 0;
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
        goto ERR;
    }
    send_err_code(FILE_GET_OK, basename(filePath), info);

    if (flag && (recv_errcode(info) != FILE_UP_OK))
        goto ERR;

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
    send_msg("\033[32;1mComplete\033[0m", info);
    free(filedata);
    close(fd);
    return 0;
ERR_TRANS:
    free(filedata);
ERR:
    close(fd);
    return -1;
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
        goto ERR;
    }
    send_err_code(FILE_UP_OK, basename(filePath), info);

    if (flag && (recv_errcode(info) != FILE_GET_OK))
        goto ERR;

    transHeader_t header;
    fileData_t *filedata = malloc(sizeof(fileData_t));
    uint64_t ndata = sizeof(filedata->data);

    while (1)
    {
        if (0 != recv_cli_header(&header, info))
            goto ERR_TRANS;
        if (0 != recv_cli_data((void *)&filedata, &ndata, &header, info, DATA_STACK))
            goto ERR_TRANS;
        write(fd, filedata->data, header.datalen - 8);
        if (FILE_END == filedata->flag)
            break;
    }
    send_msg("\033[32;1mComplete\033[0m", info);

    free(filedata);
    close(fd);
    return 0;
ERR_TRANS:
    free(filedata);
ERR:
    close(fd);
    return -1;
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
    send_msg("\033[32;1mremoved\033[0m", info);
    return 0;
}

int cmd_num_getfile(thr_dat_t *info, uint8_t *data)
{
    if (0 == *(uint64_t *)(data + 8))
    {
        send_err_code(NUM_ERROR, "Error Number!", info);
        return -1;
    }
    DIR *curdir = opendir(info->pwd);
    char filePath[256];
    seekdir(curdir, filelist[info->fd][*(uint64_t *)(data + 8) - 1]);
    struct dirent *dirinfo = readdir(curdir);

    strcpy(filePath, info->pwd);
    strcat(filePath, "/");
    strcat(filePath, dirinfo->d_name);

    PRINT_MSG("%d: get%s\n", info->fd, filePath);
    struct stat filestat;
    int ret = stat(filePath, &filestat);
    if (ret == -1 || !S_ISREG(filestat.st_mode))
    {
        send_err_code(FILE_NOT_EXIST, strerror(errno), info);
        goto ERR;
    }
    send_err_code(FILE_GET_OK, basename(filePath), info);

    if (SERVER && (recv_errcode(info) != FILE_UP_OK))
        goto ERR;

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

    send_msg("\033[32;1mComplete\033[0m", info);
    free(filedata);
    closedir(curdir);
    close(fd);
    return 0;
ERR:
    printf("Error exit\n");
    closedir(curdir);
    return -1;
}

int cmd_num_remove(thr_dat_t *info, uint8_t *data)
{

    if (0 == *(uint64_t *)(data + 8))
    {
        send_err_code(NUM_ERROR, "Error Number!", info);
        return -1;
    }

    DIR *curdir = opendir(info->pwd);
    char filePath[256];
    seekdir(curdir, filelist[info->fd][*(uint64_t *)(data + 8) - 1]);
    struct dirent *dirinfo = readdir(curdir);

    strcpy(filePath, info->pwd);
    strcat(filePath, "/");
    strcat(filePath, dirinfo->d_name);
    rm_relative_path(filePath);

    int ret = unlink(filePath);
    if (ret != 0)
    {
        send_err_code(FILE_NOT_EXIST, strerror(errno), info);
        goto ERR;
    }
    send_msg("\033[32;1mremoved\033[0m", info);
    closedir(curdir);
    return 0;
ERR:
    closedir(curdir);
    return -1;
}

int cmd_num_changedir(thr_dat_t *info, uint8_t *data)
{
    if (0 == *(uint64_t *)(data + 8))
    {
        send_err_code(NUM_ERROR, "Error Number!", info);
        return -1;
    }

    DIR *curdir = opendir(info->pwd);
    char modPath[256];
    seekdir(curdir, filelist[info->fd][*(uint64_t *)(data + 8) - 1]);
    struct dirent *dirinfo = readdir(curdir);

    strcpy(modPath, info->pwd);
    strcat(modPath, "/");
    strcat(modPath, dirinfo->d_name);
    rm_relative_path(modPath);

    struct stat filestat;

    if (-1 == stat(modPath, &filestat) || !S_ISDIR(filestat.st_mode))
    { // failure
        send_msg("Error Path!", info);
        goto ERR;
    }
    else
    { // successful
        strcpy(info->pwd, modPath);
        sprintf(modPath, "working dir change to:\n\t%s", info->pwd);
        send_msg(modPath, info);
    }

    closedir(curdir);

    curdir = opendir(info->pwd);
    struct dirent *file;
    uint64_t count = 1;
    while (1)
    {
        if (count < LIST_LEN)
            filelist[info->fd][count - 1] = telldir(curdir);
        if (NULL == (file = readdir(curdir)))
            break;
        if (!strncmp(file->d_name, "..", 1) || !strncmp(file->d_name, "..", 2))
            continue;
        count++;
    }
    closedir(curdir);
    return 0;

ERR:
    closedir(curdir);
    return -1;
}

int cmd_exit(thr_dat_t *info, uint8_t *data)
{
    printf("\033[31m%d disconnect!\033[0m\n", info->fd);
    send_err_code(DISCONNECT, "disconnected!", info);
    if (NULL != filelist[info->fd])
    {
        free(filelist[info->fd]);
        filelist[info->fd] = NULL;
    }
    close(info->fd);
    free(info);
    free(data);
    pthread_exit(NULL);
}
