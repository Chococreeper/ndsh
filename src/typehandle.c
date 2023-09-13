#include "ndserver.h"
#include "cmdhandle.h"
#include "transhandle.h"

#include <stdio.h>
#include <string.h>

// 上传与下载许可
static int upload_permit = 0;
static int remove_permit = 0;

int which_cmd(char *cmd)
{
    if (!strncmp(cmd, "GETLIST", 7))
        return CMD_GETLIST;
    if (!strncmp(cmd, "GETFILE", 7))
        return CMD_GETFILE;
    if (!strncmp(cmd, "GETHLIST", 8))
        return CMD_GETHLIST;
    if (!strncmp(cmd, "NGETFILE", 8))
        return CMD_NUM_GETFILE;
    if (!strncmp(cmd, "CHNGEDIR", 8))
        return CMD_CHANGEDIR;
    if (!strncmp(cmd, "NCHNGDIR", 8))
        return CMD_NUM_CHANGEDIR;
    if (!strncmp(cmd, "UPLOAD", 6))
        return CMD_UPLOAD;
    if (!strncmp(cmd, "PRIWKDIR", 8))
        return CMD_PWD;
    if (!strncmp(cmd, "REMOVE", 6))
        return CMD_REMOVE;
    if (!strncmp(cmd, "NREMOVE", 7))
        return CMD_NUM_REMOVE;
    if (!strncmp(cmd, "EXIT", 4))
        return CMD_EXIT;
    return CMD_UNKNOW;
}

void handle_command(transHeader_t *header, uint8_t *data, thr_dat_t *info)
{
    PRINT_MSG("%d: command handle:", info->fd);
    switch (which_cmd(data))
    {
    case CMD_GETLIST:
        printf("Get list Command!\n");
        cmd_getlist(info, 1);
        break;
    case CMD_GETHLIST:
        printf("Get hidden File Command!\n");
        cmd_getlist(info, 0);
        break;
    case CMD_GETFILE:
        printf("Get File Command!\n");
        cmd_getfile(info, data, SERVER);
        break;
    case CMD_NUM_GETFILE:
        printf("Get File by number Command!\n");
        cmd_num_getfile(info, data);
        break;
    case CMD_UPLOAD:
        if (!upload_permit)
        {
            send_msg("\033[31;1mServer disable upload!\033[0m", info);
            break;
        }
        printf("Upload File Command!\n");
        cmd_upload(info, data, SERVER);
        break;
    case CMD_CHANGEDIR:
        printf("Change Dir Command!\n");
        cmd_changedir(info, data);
        break;
    case CMD_NUM_CHANGEDIR:
        printf("Change Dir by num Command!\n");
        cmd_num_changedir(info, data);
        break;
    case CMD_PWD:
        printf("Pwd Command!\n");
        cmd_pwd(info);
        break;
    case CMD_REMOVE:
        if (!remove_permit)
        {
            send_msg("\033[31;1mServer disable remove!\033[0m", info);
            return;
        }
        printf("Remove Command!\n");
        cmd_remove(info, data);
        break;
    case CMD_NUM_REMOVE:
        if (!remove_permit)
        {
            send_msg("\033[31;1mServer disable remove!\033[0m", info);
            return;
        }
        printf("Remove by num Command");
        cmd_num_remove(info, data);
        break;
    case CMD_EXIT:
        printf("Exit Command!\n");
        cmd_exit(info, data);
        break;
    default:
        printf("Error Command!\n");
        break;
    }
}

void handle_message(transHeader_t *header, uint8_t *data, thr_dat_t *info)
{
    // PRINT_MSG("%d: message handle\n", info->fd);
    printf("%s\n", data);
}

int handle_login(transHeader_t *header, uint8_t *data, thr_dat_t *info, char *passwd)
{
    if (!strcmp(data, passwd))
        return 1;
    return 0;
}

void handle_respond(transHeader_t *header, uint8_t *data, thr_dat_t *info)
{
    PRINT_MSG("%d: respond handle\n", info->fd);
}

void handle_unknow(transHeader_t *header, uint8_t *data, thr_dat_t *info)
{
}

int cmd_off_upload()
{
    upload_permit = 0;
    printf("upload disable!\n");
    return 0;
}

int cmd_on_upload()
{
    upload_permit = 1;
    printf("upload enable!\n");
    return 0;
}

int cmd_off_remove()
{
    remove_permit = 0;
    printf("remove disable!\n");
    return 0;
}

int cmd_on_remove()
{
    remove_permit = 1;
    printf("remove enable!\n");
    return 0;
}