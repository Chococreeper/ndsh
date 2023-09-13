#ifndef __CMDHANDLE_H__
#define __CMDHANDLE_H__

#include "ndserver.h"

#define SERVER 1
#define CLIENT 0

int cmd_getlist(thr_dat_t *info, int flag);
int cmd_pwd(thr_dat_t *info);
int cmd_changedir(thr_dat_t *info, uint8_t *data);

// flag 用于判断是否在服务器使用或客户端使用
// 本质上是是否在函数内部接收FILE_GET_OK或FILE_UP_OK错误码
int cmd_getfile(thr_dat_t *info, uint8_t *data, int flag);
int cmd_upload(thr_dat_t *info, uint8_t *data, int flag);

int cmd_num_getfile(thr_dat_t *info, uint8_t *data);
int cmd_num_changedir(thr_dat_t *info, uint8_t *data);
int cmd_num_remove(thr_dat_t *info, uint8_t *data);

int cmd_remove(thr_dat_t *info, uint8_t *data);
int cmd_exit(thr_dat_t *info, uint8_t *data);

#endif