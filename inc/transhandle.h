#ifndef __TRANSHANDLE_H__
#define __TRANSHANDLE_H__

#define ERR_TIMEOUT 100
#define ERR_DISCONNECT 101

#include <ndserver.h>

int send_data(transHeader_t *headerp, void *data, thr_dat_t *info);
int send_msg(char *msg, thr_dat_t *info);
int send_err_code(uint64_t errcode, char *msg, thr_dat_t *info);

int header_types_is_ok(transHeader_t *header);

/// @brief 传输头接收处理函数
/// @param headerp 传输数据头
/// @param info 客户端连接参数
/// @return 成功返回0，失败返回错误码
int recv_cli_header(transHeader_t *headerp, thr_dat_t *info);

/// @brief 数据接收处理函数
/// @param data 接收数据的堆区指针
/// @param ndata data指针指向的空间大小，在函数内做更改，不允许在该函数外更改此变量
/// @param header 传输数据头
/// @param info 客户端连接参数
/// @param flag data是否在堆区
/// @return 成功返回0， 错误返回错误码
int recv_cli_data(uint8_t **data, uint64_t *ndata, transHeader_t *header, thr_dat_t *info, int flag);

uint64_t recv_errcode(thr_dat_t *info);

#define DATA_STACK 0
#define DATA_HEAP 1

#endif