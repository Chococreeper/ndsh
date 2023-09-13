#ifndef __NDSERVER_H__
#define __NDSERVER_H__

// 是否输出错误信息(服务端)
#define PRINT_ERR 1

#define ERR(errcode)   \
    if (-1 == errcode) \
    {                  \
        return -1;     \
    }

#if PRINT_ERR
#define PERR(fx) perror(fx);
#endif

// 标准输出 开关
#ifndef NOT_PRINT_MSG
#define PRINT_MSG(...) printf(__VA_ARGS__)
#else
#define PRINT_MSG(msg)
#endif

#define ERRP(errcode, fx) \
    if (-1 == errcode)    \
    {                     \
        PERR(fx);         \
        return -1;        \
    }

#include <sys/types.h>

#define N_CONNECT 1024

#define DAT_BUF_SIZE 4096

// 服务器默认监听地址和端口
#define DEF_IP "0.0.0.0"
#define DEF_PORT 8866

// transHeader_t 接收后，接收数据包的超时时间
#define TIMEOUT_SEC 3

typedef unsigned short port_t;

// 传输数据包类型
#define TYPE_CMD 0x0001
#define TYPE_MSG 0x0002
#define TYPE_RES 0x0004
#define TYPE_FILE 0x0008
#define TYPE_ERR 0x0010
#define TYPE_LOGIN 0x0020

#include <netinet/in.h>

// TYPE_CMD数据包类型下，各命令的枚举变量
enum cmd
{
    CMD_UNKNOW,
    CMD_GETLIST = 10,
    CMD_GETHLIST,
    CMD_GETFILE,
    CMD_NUM_GETFILE,
    CMD_UPLOAD,
    CMD_CHANGEDIR,
    CMD_NUM_CHANGEDIR,
    CMD_PWD,
    CMD_REMOVE,
    CMD_NUM_REMOVE,
    CMD_EXIT
};

// 客户端的相关信息
typedef struct
{
    // 传输文件描述符
    int fd;

    // 客户端相关信息
    struct sockaddr_in cliaddr;

    // 客户端当前路径
    char pwd[256];
} thr_dat_t;

// 每个数据传输头，必须先发送该结构体才能发送数据
typedef struct transPkg
{
    // transHeader_t 高32位固定格式
#define HEADER_CODE 0x80FCUL

    // 包类型
    __uint64_t types;

    // 数据包长度
    __uint64_t datalen;

    // 校验码
    __uint64_t varify_code;
} transHeader_t;

// 文件传输Data格式
typedef struct
{
#define FILE_MORE 0x00000010
#define FILE_END 0x0000001F
    uint64_t flag;
    uint8_t data[DAT_BUF_SIZE];
} fileData_t;

// send_err_code函数错误码
#define FILE_GET_OK 0x0F
#define FILE_UP_OK 0x08
#define FILE_NOT_EXIST 0x00000001
#define FILE_EXIST 0x0000000F
#define NUM_ERROR 0x0000F100
#define DISCONNECT 0xFFFFFFFF

int sock_init(char *ip, __uint16_t port);

int netdisk_main(char *path, char *ip, port_t port);

#endif