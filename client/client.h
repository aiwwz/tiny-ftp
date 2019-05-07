#ifndef __CLIENT_H__
#define __CLIENT_H__



int cmd_open(const char *ip_addr, const char *port);
void cmd_close();
void cmd_cd(const char *path);
void cmd_ls(const char *path);
void cmd_pwd();
void cmd_put(const char *filename);
void cmd_get(const char *filename);
void cmd_rm(const char *filename);
void cmd_mkdir(const char *dir);
void cmd_rmdir(const char *dir);
void cmd_help();
void cmd_quit();
void cmd_exit();
int client_send_cmd(const char *cmd, const char *args);
int client_recv_reply();
int client_connect(const char *ip_addr, int port);



#endif /*__CLIENT_H__*/
