#include <func.h>
#include "client.h"

#define DEFAULT_PORT 21
int control_sockfd;
int data_sockfd;

int recv_cycle(int newfd, void *p, int len){
    int ret, total = 0;
    char *p_start = (char*)p;
    while(total < len){
        ret = recv(newfd, p_start+total, len-total, 0);
        if(ret == 0){
            return -1;
        }
        total += ret;
    }
    return 0;
}
/*-------------------------------------------------------------------------------------*/
int cmd_pasv(){ //要求服务器进入被动模式,服务器将在数据端口监听
    char buf[512] = {0};
    client_send_cmd("PASV", NULL);
    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0); 
    printf("%s", buf); //打印服务器发来的应答信息
    int res, data_port = -1;
    sscanf(buf, "%d", &res);
    if(res == 227){ //进入passive mode成功, 获取数据端端口
        char *pch, *p_port;
        p_port = buf;
        int i;
        for(i = 0; i < 4; i++){
            pch = strchr(p_port, ',');
            if(pch != NULL){ 
                p_port = pch + 1;
            }
            else{
                printf("服务端227协议有问题?\n");
            }
        }
        int m, n;
        sscanf(p_port, "%d,%d)", &m, &n);
        data_port = m * 256 + n; //得到数据端口
    }
    return data_port;    
}

/* 登录ftp服务器 */
int cmd_open(const char *ip_addr, const char *port){
    char username[64] = {0}, *passwd;
    control_sockfd = client_connect(ip_addr, atoi(port));
    printf("control_sockfd = %d\n", control_sockfd);
    if(control_sockfd == -1){
        return -1;
    }

    int reply_code;
    reply_code = client_recv_reply();
    if(reply_code == -1){
        return -1;
    }

    printf("username:");
    scanf("%s", username);
    client_send_cmd("USER", username);
    reply_code = client_recv_reply();
    if(reply_code == -1){
        return -1;
    }
    //测试区开始
    client_send_cmd("MKD", NULL);

    //测试区结束

    passwd = getpass("password:");
    getchar(); //标准输入还有一个'\n'
    client_send_cmd("PASS", passwd);
    reply_code = client_recv_reply();
    if(reply_code == -1){
        return -1;
    }

    //测试
    client_send_cmd("CWD", "dir/");

    return 0;
}


/* 进入对应目录 */
void cmd_cd(const char *path){
    client_send_cmd("CWD", path);
    client_recv_reply();
}

/* 列出目录内容 */
void cmd_ls(const char *path){
    int data_port = cmd_pasv();
    data_sockfd = client_connect("192.168.3.6", data_port);

    client_send_cmd("LIST", path);

    char buf[1024] = {0};
    recv(data_sockfd, buf, sizeof(buf), 0);
    close(data_sockfd);
    printf("%s", buf);

    client_recv_reply();
}

/* 打印当前工作路径 */
void cmd_pwd(){
    client_send_cmd("PWD", NULL);
    client_recv_reply();
}

/* 上传 */
void cmd_put(const char *filename){
    int fd = open(filename, O_RDONLY, 0664);
    if(fd == -1){
        printf("local: %s:", filename);
        fflush(stdout);
        perror("");
        return;
    }
    struct stat st;
    fstat(fd, &st);
    char *p_file = (char*)mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    ERROR_CHECK(p_file, (char*)-1, "mmap");
    
    int data_port = cmd_pasv();
    data_sockfd = client_connect("192.168.3.6", data_port);

    client_send_cmd("STOR", filename);
    int reply_code = client_recv_reply();
    if(reply_code != 150){
        //文件名非法
        return;
    }
    
    int ret;
    long curr_size = 0;
    while(curr_size < st.st_size){
        ret = send(data_sockfd, p_file, st.st_size-curr_size, 0); 
        ERROR_CHECK(ret, -1, "send");
        curr_size += ret;
        printf("curr_size=%ld\n", curr_size);
    }
    close(data_sockfd);
    client_recv_reply();
}

/* 下载 */
int get_file_size(long *p_filesize){
    char buf[128] = {0};
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s", buf);
    int reply_code;
    sscanf(buf, "%d", &reply_code);
    if(reply_code == 213){
        sscanf(buf, "%d%ld", &reply_code, p_filesize);
    }
    else{
        *p_filesize = -1;
    }
    return reply_code;
}
void cmd_get(const char *filename){
    int fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC, 0664);
    if(fd == -1){
        printf("local: %s:", filename);
        fflush(stdout);
        perror("");
        return;
    }
    int data_port = cmd_pasv();
    data_sockfd = client_connect("192.168.3.6", data_port);

    long filesize;
    int reply_code;
    client_send_cmd("SIZE", filename);
    reply_code = get_file_size(&filesize);    
    if(reply_code != 213){
        //文件不存在
        return;
    }
    //文件存在
    client_send_cmd("RETR", filename);
    client_recv_reply();
    char buf[1024] = {0};
    int ret, curr_size = 0;
    while(curr_size < filesize){
        ret = recv(data_sockfd, buf, sizeof(buf), 0);
        if(ret == -1){
            perror("recv");
            return;
        }
        curr_size += ret;
        write(fd, buf, ret);
    }
    client_recv_reply();
    close(data_sockfd);
    close(fd);
}

/* 删除文件 */
void cmd_rm(const char *filename){
    client_send_cmd("DELE", filename);
    int reply_code = client_recv_reply();
    if(reply_code == 250){
        //删除成功！
    }
    else{
        //判断原因！
    }
}

/* 创建目录 */
void cmd_mkdir(const char *dir){
    client_send_cmd("MKD", dir);
    client_recv_reply();
}

/* 删除目录 */
void cmd_rmdir(const char *dir){
    client_send_cmd("RMD", dir);
    client_recv_reply();
}

/* 帮助信息 */
void cmd_help(){
    printf("usage: ./tiny_ftp [ip_addr [port]]\n");
    printf("----------------------------Commands-----------------------------\n");
    printf("\033[32mcd <dir>\033[0m    --enter  directory.\n");
    printf("\033[32mclose\033[0m       --close the connection with the server.\n");
    printf("\033[32mexit\033[0m        --exit this ftp client.\n");
    printf("\033[32mget <file>\033[0m  --get remote file from ftp server.\n");
    printf("\033[32mhelp\033[0m        --print help information.\n");
    printf("\033[32mls [<dir>]\033[0m  --list the files and directoris in directory.\n");
    printf("\033[32mmkdir <dir>\033[0m --make a new diretory on the ftp server.\n");
    printf("\033[32mopen\033[0m        --open ftp server.\n");
    printf("\033[32mput <file>\033[0m  --send local file to ftp server.\n");
    printf("\033[32mpwd\033[0m         --print the current/work directory of server.\n");
    printf("\033[32mquit\033[0m        --quit this ftp client.\n");
    printf("\033[32mrm <file>\033[0m   --delete the file on the ftp server.\n");
    printf("\033[32mrmdir <dir>\033[0m --delete an empty directory on the ftp server.\n");
    printf("-----------------------------------------------------------------\n");
    printf("tiny_ftp client  v0.01  20190507.\n");
    printf("Copyright (C) 2019 AIWWZ\n");
}

/* 退出 */
void cmd_close(){ //断开与ftp服务器的连接但不关闭客户端
    client_send_cmd("QUIT", NULL);
    client_recv_reply();
    close(control_sockfd);
}
void cmd_exit(){ //退出客户端
    cmd_close();
    exit(0);
}
void cmd_quit(){
    cmd_exit();
}

/* 向服务器发送命令 */
int client_send_cmd(const char *cmd, const char *args){
    char buf[512] = {0};
    if(cmd != NULL){
        strcpy(buf, cmd);
        if(args != NULL){
            strcat(buf, " "); //空格分隔
            strcat(buf, args);
        }
    }
    strcat(buf, "\r\n");
    int ret;
    ret = send(control_sockfd, buf, strlen(buf), 0);
    if(ret == -1){
        perror("send");
        return -1;
    }
    return 0;
}

/* 获取服务器应答 */
int client_recv_reply(){
    char buf[512] = {0};
    int ret;
    ret = recv(control_sockfd, buf, sizeof(buf), 0);
    if(ret == -1){
        perror("recv");
        return -1;
    }
    printf("%s", buf); //打印响应码及详细信息
    fflush(stdout);

    buf[3] = '\0';
    return  atoi(buf); //返回响应码
}

int client_connect(const char *ip_addr, int port){
    //与ftp服务器端口建立连接
    int sockfd;
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_aton(ip_addr, &serv_addr.sin_addr);
    Connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("Connect to %s:%d\n", ip_addr, port);

    return sockfd;
}

/* 切分参数 */
char** split_cmds(char *buf){
    if(buf == NULL){
        return NULL;
    }
    int argnum = 4; //默认只支持3个参数(另外1个为末尾的NULL)
    char **cmds = (char**)calloc(argnum,  sizeof(char*));

    char *pch = buf;
    char *start;
    int curr_argnum = 0; //当前参数个数
    int len;

    while(1){
        while(*pch == ' ' || *pch == '\t' || *pch == '\n'){ //跳过空白符
            pch++;
        }
        if(*pch == '\0'){
            break;
        }

        if(curr_argnum + 1 >= argnum){
            //空间不足,扩大为原来的两倍
            cmds = (char**)realloc(cmds, 2 * argnum * sizeof(char*));
            argnum *= 2;
        }

        len = 0;
        start = pch;
        while(*pch != ' ' && *pch != '\t' && *pch != '\n' && *pch != '\0'){
            len++;
            pch++;
        }
        cmds[curr_argnum] = (char*)calloc(len + 1, sizeof(char));
        strncpy(cmds[curr_argnum], start, len);
        cmds[curr_argnum][len] = '\0'; //末尾加'\0'
        curr_argnum++;
    }
    cmds[curr_argnum] = NULL; //命令参数表以NULL结尾
    return cmds;
}

void free_cmds(char **cmds){
    if(cmds == NULL){
        return;
    }
    int i = 0;
    while(cmds[i] != NULL){
        free(cmds[i++]);
    }
    free(cmds);
}

void print_cmds(char **cmds){
    if(cmds == NULL){
        return;
    }
    char **pch = cmds;
    while(*pch != NULL){
        printf("%s ", *pch);
        pch++;
    }
    printf("\n");
}

int sizeof_cmd(char **cmds){
    if(cmds == NULL){
        return 0;
    }
    int size = 0;
    while(cmds[size] != NULL){
        size++;
    }
    return size;
}

void ftp_client(){
    char buf[512];
    char **cmds = NULL;

    while(1){
        printf("tiny_ftp>");
        fflush(stdout);
        bzero(buf, sizeof(buf));
        free_cmds(cmds);
        fgets(buf, sizeof(buf), stdin);
        cmds = split_cmds(buf);
        if(cmds[0] == NULL){
            continue;
        }
        else if(strcmp(cmds[0], "open") == 0){
            /*if(cmds[1] == NULL){ //未指定ip
              printf("usage: open ip_addr [port]\n");
              continue;
              }
              else if(cmds[2] == NULL){ //未指定port,使用默认端口
              cmd_open(cmds[1], "21");
              }
              else{
              cmd_open(cmds[1], cmds[2]);
              }*/
            cmd_open("192.168.3.6", "21");
        }
        else if(strcmp(cmds[0], "cd") == 0){
            //bug:若cmds[1]==NULL,将崩溃
            if(sizeof_cmd(cmds) == 1){
                cmd_cd("/");
            }
            else{
                cmd_cd(cmds[1]);
            }
        }
        else if(strcmp(cmds[0], "ls") == 0){
            cmd_ls(cmds[1]);
        }
        else if(strcmp(cmds[0], "rm") == 0){
            cmd_rm(cmds[1]);
        }
        else if(strcmp(cmds[0], "mkdir") == 0){
            cmd_mkdir(cmds[1]);
        }
        else if(strcmp(cmds[0], "rmdir") == 0){
            cmd_rmdir(cmds[1]);
        }
        else if(strcmp(cmds[0], "get") == 0){
            cmd_get(cmds[1]);
        }
        else if(strcmp(cmds[0], "put") == 0){
            cmd_put(cmds[1]);
        }
        else if(strcmp(cmds[0], "close") == 0){
            cmd_close();
        }
        else if(strcmp(cmds[0], "pwd") == 0){
            cmd_pwd();
        }
        else if(strcmp(cmds[0], "exit") == 0){
            cmd_exit();
        }
        else if(strcmp(cmds[0], "quit") == 0){
            cmd_quit();
        }
        else if(strcmp(cmds[0], "help") == 0){
            cmd_help();
        }
        else{
            printf("??invalid command\n");
            fflush(stdout);
            continue;
        }
        //print_cmds(cmds);
    }
}

int main(int argc, char* argv[]){
    if(argc == 1){
        ftp_client();
    }
    else{
        //printf("usage: tiny_ftp [hostname [port]]\n");
        printf("usage: ./tiny_ftp\n");
    }

    return 0;
    /*------------------------------------------------------------------------------------*/
    int control_sockfd, data_sockfd;
    control_sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    //与ftp服务器控制端口建立连接
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(21);
    inet_aton("192.168.3.6", &serv_addr.sin_addr);
    Connect(control_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    printf("Connect success!\n");
    char buf[1000] = {0};
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf); //打印服务器发来的应答信息

    //发送用户名 
    sprintf(buf, "USER %s\r\n", argv[1]);
    send(control_sockfd, buf, strlen(buf), 0);
    memset(buf, 0, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0); 
    printf("%s\n", buf); //打印服务器发来的应答信息

    //发送密码
    sprintf(buf, "PASS %s\r\n", argv[2]);
    send(control_sockfd, buf, strlen(buf), 0);
    memset(buf, 0, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0); 
    printf("%s\n", buf); //打印服务器发来的应答信息

    //要求服务器进入被动模式,服务器将在数据端口监听
    sprintf(buf, "PASV\r\n");
    send(control_sockfd, buf, strlen(buf), 0);
    memset(buf, 0, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0); 
    printf("%s\n", buf); //打印服务器发来的应答信息
    int res; //相应码
    sscanf(buf, "%d", &res);
    int i;
    int data_port;
    if(res == 227){ //进入passive mode成功, 获取数据端端口
        char *pch, *p_port;
        p_port = buf;
        for(i = 0; i < 4; i++){
            pch = strchr(p_port, ',');
            if(pch != NULL){ 
                p_port = pch + 1;
            }
            else{
                printf("服务端227协议有问题?\n");
            }
        }
        int m, n;
        sscanf(p_port, "%d,%d)", &m, &n);
        data_port = m * 256 + n; //得到数据端口
    }

    //与ftp服务器数据端口建立连接
    data_sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(data_port); //数据端
    inet_aton("192.168.3.6", &serv_addr.sin_addr);
    Connect(data_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    //改变目录
    sprintf(buf, "CWD /dir1/\r\n");
    send(control_sockfd, buf, strlen(buf), 0);
    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf); //打印服务器发来的应答信息

    //获取当前目录
    sprintf(buf, "PWD\r\n");
    send(control_sockfd, buf, strlen(buf), 0);
    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf); //打印服务器发来的应答信息

    //获取文件size
    sprintf(buf, "SIZE /dir2/1.txt\r\n");
    send(control_sockfd, buf, strlen(buf), 0);
    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf); //打印服务器发来的应答信息
    long filesize;
    sscanf(buf, "%d%ld", &res, &filesize);

    //下载文件--发送命令
    sprintf(buf, "RETR /dir2/1.txt\r\n");
    send(control_sockfd, buf, strlen(buf), 0);
    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf); //打印服务器发来的应答信息
    //下载文件--下载数据
    int fd1 = open("1.txt", O_CREAT|O_WRONLY, 0662);
    recv_cycle(data_sockfd, buf, filesize);
    write(fd1, buf, filesize);
    close(fd1);
    printf("ok!\n");
    close(data_sockfd);

    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), MSG_DONTWAIT); //有粘包问题
    printf("%s\n", buf); //打印服务器发来的应答信息


    //要求服务器进入被动模式,服务器将在数据端口监听
    sprintf(buf, "PASV\r\n");
    send(control_sockfd, buf, strlen(buf), 0);
    memset(buf, 0, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0); 
    printf("%s\n", buf); //打印服务器发来的应答信息
    sscanf(buf, "%d", &res);
    if(res == 227){ //进入passive mode成功, 获取数据端端口
        char *pch, *p_port;
        p_port = buf;
        for(i = 0; i < 4; i++){
            pch = strchr(p_port, ',');
            if(pch != NULL){ 
                p_port = pch + 1;
            }
            else{
                printf("服务端227协议有问题?\n");
            }
        }
        int m, n;
        sscanf(p_port, "%d,%d)", &m, &n);
        data_port = m * 256 + n; //得到数据端口
    }

    //与ftp服务器数据端口建立连接
    data_sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(data_port); //数据端
    inet_aton("192.168.3.6", &serv_addr.sin_addr);
    Connect(data_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    //获取当前目录
    sprintf(buf, "PWD\r\n");
    send(control_sockfd, buf, strlen(buf), 0);
    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf); //打印服务器发来的应答信息

    //发送文件
    sprintf(buf, "STOR /file.txt\r\n");
    send(control_sockfd, buf, strlen(buf), 0);
    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf); //打印服务器发来的应答信息
    fd1 = open("1.txt", O_CREAT|O_RDONLY, 0662);
    bzero(buf, sizeof(buf));
    read(fd1, buf, sizeof(buf));
    send(data_sockfd, buf, 6, 0);
    close(data_sockfd);
    printf("close data_sockfd\n");
    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf); //打印服务器发来的应答信息
    close(fd1);
    printf("send file ok!\n");

    //准备结束
    sprintf(buf, "QUIT\r\n");
    send(control_sockfd, buf, strlen(buf), 0);
    bzero(buf, sizeof(buf));
    recv(control_sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf); //打印服务器发来的应答信息
    close(control_sockfd);

    printf("over!\n");

    return 0;
    //--------------------------------------------------------------------
    int data_len;
    recv_cycle(control_sockfd, &data_len, 4);    //读出文件名长度
    recv_cycle(control_sockfd, buf, data_len);   //读出文件名
    int fd = open(buf, O_CREAT|O_WRONLY, 0666);
    ERROR_CHECK(fd, -1, "open");

    off_t file_size = 0, curr_size = 0, old_size = 0, slice_size = 0;
    recv_cycle(control_sockfd, &file_size, sizeof(off_t)); //读出文件实际大小
    slice_size = file_size/10000; //每读取0.01%则刷新

    int ret;
    while(1)
    {
        ret = recv_cycle(control_sockfd, &data_len, 4);
        if(ret == -1){
            break;
        }

        if(data_len > 0)
        {
            recv_cycle(control_sockfd, buf, data_len);
            write(fd, buf, data_len);
            curr_size += data_len; //当前已读size
            if(curr_size - old_size >= slice_size){
                printf("\rdownload %5.2f%%", (float)curr_size/file_size*100);
                fflush(stdout); 
                old_size = curr_size;
            }
        }else{
            printf("\n");
            break;
        }
    }

    close(fd);
    close(control_sockfd);
    return 0;
}
