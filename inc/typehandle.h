#ifndef __TYPEHANDLE_H__
#define __TYPEHANDLE_H__

#include "ndserver.h"

void handle_command(transHeader_t *header, uint8_t *data, thr_dat_t *info);
void handle_message(transHeader_t *header, uint8_t *data, thr_dat_t *info);
void handle_respond(transHeader_t *header, uint8_t *data, thr_dat_t *info);
void handle_unknow(transHeader_t *header, uint8_t *data, thr_dat_t *info);

// 检测接收的header是否符合格式
int header_types_is_ok(transHeader_t *header);

// 读取cmd中的命令，返回对应命令码
int which_cmd(char *cmd);

#endif