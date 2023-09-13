#include "ndserver.h"
#include "cmdhandle.h"
#include "transhandle.h"

#include <stdio.h>
#include <string.h>

int which_cmd(char *cmd)
{
    if (!strncmp(cmd, "GETLIST", 7))
        return CMD_GETLIST;
    if (!strncmp(cmd, "GETFILE", 7))
        return CMD_GETFILE;
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
        cmd_getlist(info);
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
        printf("Remove Command!\n");
        cmd_remove(info, data);
        break;
    case CMD_NUM_REMOVE:
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
    // get list
    // change dir
    // get file
    // printf working dir
    // exit
}

void handle_message(transHeader_t *header, uint8_t *data, thr_dat_t *info)
{
    // PRINT_MSG("%d: message handle\n", info->fd);
    printf("%s\n", data);
}

void handle_respond(transHeader_t *header, uint8_t *data, thr_dat_t *info)
{
    PRINT_MSG("%d: respond handle\n", info->fd);
}

void handle_unknow(transHeader_t *header, uint8_t *data, thr_dat_t *info)
{
}
