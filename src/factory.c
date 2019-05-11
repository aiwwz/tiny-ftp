#include "../include/factory.h"
#define LOGGED_ON  1
#define LOGGED_OUT 0

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
        while(*pch == ' ' || *pch == '\t' || *pch == '\r' || *pch == '\n'){ //跳过空白符
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
        while(*pch != ' ' && *pch != '\t' && *pch != '\r' && *pch != '\n' && *pch != '\0'){
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

void free_path(char **dirs){
    free_cmds(dirs);
}

void send_welcome_message(int sockfd){
    char buf[1024] = {0};
    sprintf(buf, "220-tiny_ftp server version 0.01 beta\r\n");
    printf("\033[32m%s\033[0m", buf);
    send(sockfd, buf, strlen(buf), 0);
    sprintf(buf, "220 written by WZAIW(237689330@qq.com)\r\n");
    printf("\033[32m%s\033[0m", buf);
    send(sockfd, buf, strlen(buf), 0);
}

int server_recv_cmd(elem_t *task, char ***cmds){ //注意这里的cmds是三级指针
    free_cmds(*cmds); //每次使用之前必须释放上一次申请的空间

    char buf[1024] = {0};
    int ret = recv(task->newfd, buf, sizeof(buf), 0);
    ERROR_CHECK(ret, -1, "recv");
    if(ret == 0){ //对方已断开
        return 0;
    }

    *cmds = split_cmds(buf);
    printf("\033[34m%s\033[0m", buf);
    return ret;
}

void server_send_reply(elem_t *task, int reply_code, const char *info){
    char buf[1024] = {0};
    sprintf(buf, "%d %s\r\n", reply_code, info);
    int ret = send(task->newfd, buf, strlen(buf), 0);
    ERROR_CHECK(ret, -1, "send");

    printf("\033[32m%s\033[0m", buf);
}

void print_cmds(char **cmds){
    if(cmds == NULL){
        return;
    }
    char **pch = cmds;
    printf("print_cmds:");
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

int sizeof_dir(char **dirs){
    return sizeof_cmd(dirs);
}

/* 为PASV模式提供新端口 */
short get_data_port(){
    //待实现:由配置文件指定可用端口范围
    return 5000;
}

/************************************** 
 *   功能: 无大小写差别比较两字符串 
 * 返回值: -1 -- str1 > str2
 *          0 -- str1 == str2 
 *          1 -- str1 < str2
 **************************************/
int ftp_strcmp(const char *str1, const char *str2){
    assert((str1 != NULL) && (str2 != NULL));
    while(*str1 && *str2 && (toupper(*str1) == toupper(*str2))){
        str1++;
        str2++;
    }
    return (*str1-*str2);
}

/* 将字符串str中所有的字符ch1替换为ch2 */ 
char *str_replace(char *str, char ch1, char ch2){
    assert(str != NULL);
    char *tmp = str;
    while(*str != '\0'){
        if(*str == ch1){
            *str = ch2;
        }
        str++;
    }
    return tmp;
}


/* 切分路径为目录名 */
char** split_path(const char *buf){
    if(buf == NULL){
        return NULL;
    }

    int dirnum = 6; //初始支持5级目录(另外1个为末尾的NULL)
    char **dirs = (char**)calloc(dirnum,  sizeof(char*));

    const char *pch = buf;
    const char *start;
    int curr_dirnum = 0; //当前目录级数
    int len;

    while(1){
        //跳过空格和'/'
        while(*pch == '/' || *pch == ' ' || *pch == '\t' || *pch == '\r' || *pch == '\n'){
            pch++;
        }
        if(*pch == '\0'){
            break;
        }

        if(curr_dirnum + 1 >= dirnum){
            //空间不足,扩大为原来的两倍
            dirs = (char**)realloc(dirs, 2 * dirnum * sizeof(char*));
            dirnum *= 2;
        }

        len = 0;
        start = pch;
        while(*pch != '/' && *pch != ' ' && *pch != '\t' && *pch != '\r' && *pch != '\n' && *pch != '\0'){
            len++;
            pch++;
        }
        dirs[curr_dirnum] = (char*)calloc(len + 1, sizeof(char));
        strncpy(dirs[curr_dirnum], start, len);
        dirs[curr_dirnum][len] = '\0'; //末尾加'\0'
        curr_dirnum++;
    }
    dirs[curr_dirnum] = NULL; //命令参数表以NULL结尾
    return dirs;
}

/* 查看目录dirs是否在code为dir_code的目录下 */
int handle_cwd(int dir_code, char **dirs){
    if(dirs == NULL){
        return -1;
    }
    file_info_t file;
    int ret;
    if(dir_code != 0){ //若不是根目录, 则需检查该目录是否存在
        ret = db_get_file(dir_code, &file);
        if(ret == -1 || !S_ISDIR(file.st_mode)){ //目录不存在
            return -1;
        }
    }
    int tmp_code = dir_code;
    int ret_code;
    char **p_dir = dirs;
    int flag = 1; //标志:是否搜索成功
    while(*p_dir != NULL){
        ret_code = db_find_dir(tmp_code, *p_dir); //在数据库中搜索
        if(ret_code != -1){ //该目录存在
            tmp_code = ret_code; //进入该目录
        }
        else{
            flag = 0; //没找到该目录
            break;
        }
        p_dir++;
    }
    if(flag == 1){
        return tmp_code;
    }
    else{
        return -1;
    }
}

/*************************************************
 *   功能: 从code为dir_code的目录开始, 创建目录dirs 
 *     例: dirs: dir1/dir2/movie
 *         要求dir1/dir2已存在,最后创建movie目录 
 * 返回值: 0 -- 创建成功
 *        -1 -- 中间目录不存在
 *        -2 -- 目标目录已存在
 *        -3 -- 非法的目录名
 *************************************************/
int handle_mkd(const char *username, int dir_code, char **dirs){
    int ret = 0;
    int code = dir_code;
    char **p_dir = dirs;
    if(*p_dir == NULL){ //非法的目录名
        return -3;
    }
    //判断中间目录是否都存在
    while(*(p_dir+1) != NULL){
        ret = db_find_dir(code, *p_dir);
        if(ret == -1){ //中间目录不存在
            return -1;
        }
        code = ret; //进入下一级目录继续查找
        p_dir++;
    }
    printf("code:%d, name:%s\n", code, *p_dir);
    ret = db_create_dir(username, code, *p_dir); //在最后一级中间目录中创建目标目录
    if(ret == 0){ //创建成功
        return 0;
    }
    else{
        return -2; //目标目录已存在
    }
}

/***************************************************
 *   功能: 从code为dir_code的目录开始, 删除空目录dirs 
 *     例: dirs: dir1/dir2/movie
 *         要求dir1/dir2/movie存在,且movie为空目录,
 *         最后删除movie目录 
 * 返回值: 0 -- 删除成功
 *        -1 -- 目录不存在
 *        -2 -- 目标目录非空
 *        -3 -- 非法的目录名
 ****************************************************/
int handle_rmd(const char *username, int dir_code, char **dirs){
    int ret = 0;
    int code = dir_code;
    char **p_dir = dirs;
    if(*p_dir == NULL){ //非法的目录名
        return -3;
    }
    //判断所有目录是否都存在
    while(*p_dir != NULL){
        ret = db_find_dir(code, *p_dir);
        if(ret == -1){ //目录不存在
            return -1;
        }
        code = ret; //进入下一级目录继续查找
        p_dir++;
    }
    //判断目标目录是否为空
    ret = db_dir_empty(username, code);
    if(ret == 1){ //目录为空可以删除
        db_remove_dir(username, code);
        return 0;
    }
    else{ //目录非空, 不能删除
        return -2;
    }
}


/*****************************************************
 *   功能: 从code为dir_code的目录开始, 删除普通文件dirs 
 *     例: dirs: dir1/dir2/file
 *         要求dir1/dir2/file存在,且file为普通文件
 *         最后删除file文件 
 * 返回值: 0 -- 删除成功
 *        -1 -- 删除失败:文件不存在
 *******************************************************/
int handle_dele(const char *username, int dir_code, char **dirs){
    int ret = 0;
    int code = dir_code;
    char **p_dir = dirs;
    file_info_t file;
    if(*p_dir == NULL){ //非法的路径
        return -1;
    }
    //判断所有中间目录是否都存在
    while(*(p_dir+1) != NULL){
        ret = db_find_dir(code, *p_dir);
        if(ret == -1){ //目录不存在
            return -1;
        }
        code = ret; //进入下一级目录继续查找
        p_dir++;
    }

    ret = db_find_file(username, code, *p_dir);
    if(ret == -1){ //文件不存在
        return -1;
    }
    else{ //文件存在
        db_get_file(ret, &file);
        if(S_ISREG(file.st_mode)){ //是普通文件, 删除之
            printf("st_mode:%d\n", file.st_mode);
            db_remove_file(username, ret);
            return 0;
        }
        else{
            return -1; //目录文件
        }
    }
}


void tran_list_info(int sockfd, p_file_info list_info){
    if(list_info == NULL){
        close(sockfd);
        return;
    }
    p_file_info p_info = list_info;
    char buf[256] = {0};
    const char *dir = "drwxr-xr-x";
    const char *reg = "-rw-r--r--";

    while(p_info->code != 0){
        sprintf(buf, "%s 1 ftp ftp %12ld May 05 13:14 %s\r\n", S_ISDIR(p_info->st_mode)?dir:reg, p_info->st_size, p_info->filename);
        Send(sockfd, buf, strlen(buf), 0);

        p_info++;
    }

    free(list_info);
    close(sockfd);
}


/********************************************************
 *   功能: 从code为dir_code的目录开始, 打印目录dirs的信息 
 *     例: dirs: dir1/dir2/file, 要求dir1/dir2/file存在
 *         若file是目录     -- 打印该目录下的所有文件的信息
 *         若file是普通文件 -- 打印该文件的信息
 * 返回值: 0 -- 打印成功
 *        -1 -- 失败:目录不存在
 *********************************************************/
int handle_list(int data_sockfd, const char *username, int dir_code, char **dirs){
    int ret = 0;
    int code = dir_code;
    char **p_dir = dirs;
    file_info_t *list_info = NULL;
    if(dirs == NULL){ //打印当前目录
        db_list_info(username, dir_code, &list_info);
    }   
    else{ //打印指定目录
        while(*(p_dir+1) != NULL){ //判断所有中间目录是否都存在
            ret = db_find_dir(code, *p_dir);
            if(ret == -1){ //目录不存在
                close(data_sockfd);
                return -1;
            }
            code = ret; //进入下一级目录继续查找
            p_dir++;
        }
        //从倒数第二级目录获取目标文件信息
        printf("code:%d, dir:%s\n", code, *p_dir);
        code = db_find_file(username, code, *p_dir);
        if(code == -1){ //不存在
            close(data_sockfd);
            return -1;
        }
        else{
            db_list_info(username, code, &list_info);
        }
    }
    tran_list_info(data_sockfd, list_info);
    return 0;
}


void ftp_server(elem_t *task){
    //测试区开始
    /*while(1){
      int i;
      int r;
      file_info_t *list_info = NULL;
      scanf("%d", &i);
      r = db_list_info("wzj", i, &list_info);
      printf("ret:%d\n", r);
      if(r == 0){
      tran_list_info(0, list_info);
      }
      }*/
    //测试区结束
    char info[512] = {0}; //存放响应信息
    char pwd[1024] = "/"; //工作目录
    int  pwd_code = 0;    //工作目录的code
    user_info_t user;     //当前用户信息
    file_info_t file;     //存储文件信息

    printf("Connected, sending welcome message...\n");
    send_welcome_message(task->newfd); //发送欢迎信息

    char **cmds = NULL;      //客户端发来的参数列表
    char **dirs = NULL;      //保存分隔后的目录名
    int logged = LOGGED_OUT; //登录状态
    int listen_sockfd, data_sockfd = -1;    //PASV模式的数据端口
    int ret = 0;
    while(1){
        ret = server_recv_cmd(task, &cmds);    
        if(ret == 0){
            //客户端已断开连接
            printf("Disconnected.\n");
            break;
        }
        if(sizeof_cmd(cmds) == 0){ //空命令,检查以防段错误
            continue;
        }
        if(logged == LOGGED_ON){
            if(ftp_strcmp(cmds[0], "USER") == 0){ //切换用户
                strcpy(user.username, cmds[1]); //获取用户名
                sprintf(info, "Password required for %s", user.username);
                server_send_reply(task, 331, info); //请求客户端发送密码
                server_recv_cmd(task, &cmds);
                if(strcmp(cmds[0], "PASS") == 0){
                    if(sizeof_cmd(cmds) < 2){ //null passwd
                        server_send_reply(task, 530, "Login or password incorrect!");
                        break;
                    }
                    if(db_get_user(&user) == -1){ //用户不存在
                        server_send_reply(task, 530, "Login or password incorrect!");
                        logged = LOGGED_ON;
                    }
                    else{ //用户存在,验证密码
                        printf("username:%s, salt:%s, passwd:%s\n", user.username, user.salt, user.passwd);
                        if(ftp_strcmp(cmds[1], user.passwd) == 0){
                            //登录成功！
                            server_send_reply(task, 230, "Logged on");
                            logged = LOGGED_ON;
                        }
                        else{
                            //密码错误,登录失败
                            server_send_reply(task, 530, "Login or password incorrect!");
                            logged = LOGGED_OUT;
                        }

                    }
                    continue;
                }
            }
            else if(ftp_strcmp(cmds[0], "CWD") == 0){
                if(sizeof_cmd(cmds) == 1){ //cd未带参数
                    sprintf(info, "Broken client detected, missing argument to CWD. \"%s\" is current directory.", pwd);
                    server_send_reply(task, 150, info);
                }
                else{ //参数合法
                    dirs = split_path(cmds[1]);

                    if(cmds[1][0] == '/'){ //绝对路径
                        ret = handle_cwd(ROOT_DIR_CODE, dirs); //从根目录开始
                    }
                    else{ //相对路径
                        if(strcmp(dirs[0], ".") == 0){
                            ret = handle_cwd(pwd_code, dirs+1);
                        }
                        else if(strcmp(dirs[0], "..") == 0){
                            if(pwd_code == 0){ //根目录没有上一级目录
                                ret = handle_cwd(pwd_code, dirs+1);
                            }
                            else{ //非根目录有父目录
                                bzero(&file, sizeof(file));
                                if(db_get_file(pwd_code, &file) == 0){ //存在
                                    ret = handle_cwd(file.precode, dirs+1); //从父目录开始找
                                }
                                else{
                                    ret  = -1;
                                }
                            }
                        }
                        else{ //普通相对路径
                            ret = handle_cwd(pwd_code, dirs);
                        }
                    }
                    if(ret != -1){ //找到该目录
                        pwd_code = ret;             //改变工作目录code
                        printf("pwd_code:%d\n", pwd_code);
                        db_get_pwd(pwd_code, pwd); //改变工作目录
                        sprintf(info, "CWD successful. \"%s\" is current directory.", pwd);
                        server_send_reply(task, 250, info);
                    }
                    else{ //未找到该目录
                        sprintf(info, "CWD failed. \"%s\": directory not found.", cmds[1]);
                        server_send_reply(task, 550, info);
                    }
                    free_path(dirs); //不要忘记释放内存!!!
                }
            }
            else if(ftp_strcmp(cmds[0], "LIST") == 0){
                if(data_sockfd == -1){ //尚未建立数据连接
                    server_send_reply(task, 503, "Bad sequence of commands.");
                    continue;
                }
                else{
                    sprintf(info, "150 Opening data channel for directory listing of \"%s\"", sizeof_cmd(cmds)>1?cmds[1]:pwd);
                    server_send_reply(task, 150, info);
                }

                if(sizeof_cmd(cmds) == 1){ //ls当前目录
                    printf("当前路径!\n");
                    ret = handle_list(data_sockfd, user.username, pwd_code, NULL);
                }
                else{
                    dirs = split_path(cmds[1]);
                    if(cmds[1][0] == '/'){ //绝对路径
                        printf("绝对路径!\n");
                        ret = handle_list(data_sockfd, user.username, ROOT_DIR_CODE, dirs);
                    }
                    else{ //相对路径
                        if(strcmp(dirs[0], ".") == 0){
                            printf("相对路径:'.'\n");
                            ret = handle_list(data_sockfd, user.username, pwd_code, dirs+1);
                        }
                        else if(strcmp(dirs[0], "..") == 0){
                            printf("相对路径:'..'\n");
                            if(pwd_code == 0){ //根目录没有上一级目录
                                ret = handle_list(data_sockfd, user.username, pwd_code, dirs+1);
                            }
                            else{ //从上一级目录开始
                                db_get_file(pwd_code, &file);
                                ret = handle_list(data_sockfd, user.username, file.precode, dirs+1);
                            }
                        }
                        else{ //普通相对路径
                            printf("普通相对路径\n");
                            ret = handle_list(data_sockfd, user.username, pwd_code, dirs);
                        }
                    }
                    free_path(dirs); //不要忘记释放内存!!!
                }

                if(ret == 0){ //打印成功
                    if(cmds[1] == NULL){ //打印的是当前路径
                        sprintf(info, "Successfully transferred \"%s\"", pwd);
                    }
                    else{ //打印的是指定路径
                        sprintf(info, "Successfully transferred \"%s\"", cmds[1]);
                    }
                    server_send_reply(task, 226, info);
                }else{ //打印失败
                    server_send_reply(task, 550, "Directory not found.");
                }
                close(data_sockfd);
                close(listen_sockfd);
                data_sockfd = -1;
                printf("close sock!\n");
            }
            else if(ftp_strcmp(cmds[0], "RETR") == 0){
                if(sizeof_cmd(cmds) == 1){ //get未带参数
                    server_send_reply(task, 550, "Syntax error.");
                    continue;
                }

                if(data_sockfd == -1){ //尚未建立数据连接
                    server_send_reply(task, 503, "Bad sequence of commands.");
                    continue;
                }
                else{
                    sprintf(info, "150 Opening data channel for file download from server of \"%s\"", sizeof_cmd(cmds)>1?cmds[1]:pwd);
                    server_send_reply(task, 150, info);
                }

                dirs = split_path(cmds[1]);
                if(cmds[1][0] == '/'){ //绝对路径
                    printf("绝对路径!\n");
                    ret = handle_retr(data_sockfd, user.username, ROOT_DIR_CODE, dirs);
                }
                else{ //相对路径
                    if(strcmp(dirs[0], ".") == 0){
                        printf("相对路径:'.'\n");
                        ret = handle_retr(data_sockfd, user.username, pwd_code, dirs+1);
                    }
                    else if(strcmp(dirs[0], "..") == 0){
                        printf("相对路径:'..'\n");
                        if(pwd_code == 0){ //根目录没有上一级目录
                            ret = handle_retr(data_sockfd, user.username, pwd_code, dirs+1);
                        }
                        else{ //从上一级目录开始
                            db_get_file(pwd_code, &file);
                            ret = handle_retr(data_sockfd, user.username, file.precode, dirs+1);
                        }
                    }
                    else{ //普通相对路径
                        printf("普通相对路径\n");
                        ret = handle_retr(data_sockfd, user.username, pwd_code, dirs);
                    }
                }
                free_path(dirs); //不要忘记释放内存!!!

                if(ret == 0){ //打印成功
                    if(cmds[1] == NULL){ //打印的是当前路径
                        sprintf(info, "Successfully transferred \"%s\"", pwd);
                    }
                    else{ //打印的是指定路径
                        sprintf(info, "Successfully transferred \"%s\"", cmds[1]);
                    }
                    server_send_reply(task, 226, info);
                }else{ //打印失败
                    server_send_reply(task, 550, "Directory not found.");
                }
                close(data_sockfd);
                close(listen_sockfd);
                data_sockfd = -1;
                printf("close sock!\n");
            }
            else if(ftp_strcmp(cmds[0], "STOR") == 0){

            }
            else if(ftp_strcmp(cmds[0], "DELE") == 0){ //删除普通文件
                if(sizeof_cmd(cmds) < 2){ //rm没带参数
                    server_send_reply(task, 550, "Syntax error.");
                }
                else{
                    dirs = split_path(cmds[1]);
                    if(cmds[1][0] == '/'){ //绝对路径
                        printf("绝对路径!\n");
                        ret = handle_dele(user.username, ROOT_DIR_CODE, dirs);
                    }
                    else{ //相对路径
                        if(strcmp(dirs[0], ".") == 0){
                            printf("相对路径:'.'\n");
                            ret = handle_dele(user.username, pwd_code, dirs+1);
                        }
                        else if(strcmp(dirs[0], "..") == 0){
                            printf("相对路径:'..'\n");
                            if(pwd_code == 0){ //根目录没有上一级目录
                                ret = handle_dele(user.username, pwd_code, dirs+1);
                            }
                            else{ //从上一级目录开始
                                db_get_file(pwd_code, &file);
                                ret = handle_dele(user.username, file.precode, dirs+1);
                            }
                        }
                        else{ //普通相对路径
                            printf("普通相对路径\n");
                            ret = handle_dele(user.username, pwd_code, dirs);
                        }
                    }
                    free_path(dirs); //不要忘记释放内存!!!
                }
                if(ret == 0){ //删除成功
                    server_send_reply(task, 250, "File deleted successfully!");
                }
                else if(ret == -1){ //文件不存在
                    server_send_reply(task, 550, "File not found!");
                }
            }
            else if(ftp_strcmp(cmds[0], "MKD") == 0){
                if(sizeof_cmd(cmds) < 2){ //mkdir没带参数
                    server_send_reply(task, 550, "Syntax error.");
                }
                else{
                    dirs = split_path(cmds[1]);
                    if(cmds[1][0] == '/'){ //绝对路径
                        printf("绝对路径!\n");
                        ret = handle_mkd(user.username, ROOT_DIR_CODE, dirs);
                    }
                    else{ //相对路径
                        if(strcmp(dirs[0], ".") == 0){
                            printf("相对路径:'.'\n");
                            ret = handle_mkd(user.username, pwd_code, dirs+1);
                        }
                        else if(strcmp(dirs[0], "..") == 0){
                            printf("相对路径:'..'\n");
                            if(pwd_code == 0){ //根目录没有上一级目录
                                ret = handle_mkd(user.username, pwd_code, dirs+1);
                            }
                            else{ //从上一级目录开始
                                db_get_file(pwd_code, &file);
                                ret = handle_mkd(user.username, file.precode, dirs+1);
                            }
                        }
                        else{ //普通相对路径
                            printf("普通相对路径\n");
                            ret = handle_mkd(user.username, pwd_code, dirs);
                        }
                    }
                    free_path(dirs); //不要忘记释放内存!!!
                }
                if(ret == 0){ //创建目录成功
                    sprintf(info, "\"%s/\" created successfully!", cmds[1]);
                    server_send_reply(task, 257, info);
                }
                else if(ret == -1){ //中间目录不存在
                    sprintf(info, "\"%s/\": No such file or directory!", cmds[1]);
                    server_send_reply(task, 550, info);
                }
                else if(ret == -2){ //目标目录已存在
                    sprintf(info, "\"%s/\": Directory already exists!", cmds[1]);
                    server_send_reply(task, 551, info);
                }
            }
            else if(ftp_strcmp(cmds[0], "RMD") == 0){ //删除目录文件
                if(sizeof_cmd(cmds) < 2){ //rmdir没带参数
                    server_send_reply(task, 550, "Syntax error.");
                }
                else{
                    dirs = split_path(cmds[1]);
                    if(cmds[1][0] == '/'){ //绝对路径
                        printf("绝对路径!\n");
                        ret = handle_rmd(user.username, ROOT_DIR_CODE, dirs);
                    }
                    else{ //相对路径
                        if(strcmp(dirs[0], ".") == 0){
                            printf("相对路径:'.'\n");
                            ret = handle_rmd(user.username, pwd_code, dirs+1);
                        }
                        else if(strcmp(dirs[0], "..") == 0){
                            printf("相对路径:'..'\n");
                            if(pwd_code == 0){ //根目录没有上一级目录
                                ret = handle_rmd(user.username, pwd_code, dirs+1);
                            }
                            else{ //从上一级目录开始
                                db_get_file(pwd_code, &file);
                                ret = handle_rmd(user.username, file.precode, dirs+1);
                            }
                        }
                        else{ //普通相对路径
                            printf("普通相对路径\n");
                            ret = handle_rmd(user.username, pwd_code, dirs);
                        }
                    }
                    free_path(dirs); //不要忘记释放内存!!!
                }
                if(ret == 0){ //删除空目录成功
                    server_send_reply(task, 250, "Directory deleted successfully!");
                }
                else if(ret == -1){ //目录不存在
                    server_send_reply(task, 550, "Directory not found.");
                }
                else if(ret == -2){ //目录非空
                    server_send_reply(task, 550, "Directory not empty.");
                }
            }
            else if(ftp_strcmp(cmds[0], "QUIT") == 0){
                server_send_reply(task, 221, "Goodbye!");
                break;
            }
            else if(ftp_strcmp(cmds[0], "PWD") == 0){
                sprintf(info, "\"%s\" is current directory.", pwd);
                server_send_reply(task, 257, info);
            }
            else if(ftp_strcmp(cmds[0], "TYPE") == 0){
                //该指令待完善,当前只是对客户端的欺骗
                if(sizeof_cmd(cmds) < 2){
                    server_send_reply(task, 503, "Syntax error, nead a parameter:A/B");
                    break;
                }
                else if(ftp_strcmp(cmds[1], "A") == 0){ //ASCII模式传输
                    //trans_mode = ASCII_MODE;
                    server_send_reply(task, 200, "Type set to A.");
                }
                else if(ftp_strcmp(cmds[1], "B") == 0){ //Binary模式传输
                    //trans_mode = BINARY_MODE;
                    server_send_reply(task, 200, "Type set to B.");
                }
                else{ //不支持的传输类型
                    server_send_reply(task, 504, "Command not implemented for that parameter");
                }
            }
            else if(ftp_strcmp(cmds[0], "PASV") == 0){
                //被动模式,打开新端口进行数据传输
                int data_port = get_data_port();
                listen_sockfd = Tcp_init(task->sin_addr, data_port); 
                sprintf(info, "Entering Passive Mode (%s,%d,%d)", str_replace(inet_ntoa(task->sin_addr), '.', ','), data_port/256, data_port%256);
                server_send_reply(task, 227, info);
                data_sockfd = Accept(listen_sockfd, NULL, NULL);
                printf("use sock!\n");
            }
            else if(ftp_strcmp(cmds[0], "SYST") == 0){
                server_send_reply(task, 215, "Linux ubuntu.");
            }
            else{
                //错误或尚不支持的指令
                server_send_reply(task, 500, "Syntax error, command unrecognized.");
            }
        }
        else{ //尚未登录
            if(ftp_strcmp(cmds[0], "USER") == 0){ //未登录状态只能请求该命令
                strcpy(user.username, cmds[1]); //获取用户名
                sprintf(info, "Password required for %s", user.username);
                server_send_reply(task, 331, info); //请求客户端发送密码
                server_recv_cmd(task, &cmds);
                if(strcmp(cmds[0], "PASS") == 0){
                    if(sizeof_cmd(cmds) < 2){ //null passwd
                        server_send_reply(task, 530, "Login or password incorrect!");
                        break;
                    }
                    if(db_get_user(&user) == -1){ //用户不存在
                        server_send_reply(task, 530, "Login or password incorrect!");
                        logged = LOGGED_ON;
                    }
                    else{ //用户存在,验证密码
                        printf("username:%s, salt:%s, passwd:%s\n", user.username, user.salt, user.passwd);
                        if(ftp_strcmp(cmds[1], user.passwd) == 0){
                            //登录成功！
                            server_send_reply(task, 230, "Logged on");
                            logged = LOGGED_ON;
                        }
                        else{
                            //密码错误,登录失败
                            server_send_reply(task, 530, "Login or password incorrect!");
                            logged = LOGGED_OUT;
                        }

                    }
                    continue;
                }
            }
            //先登录才能继续进行
            server_send_reply(task, 530, "Please log in with USER and PASS first.");        
        }
    }
    if(data_sockfd != -1){
        close(data_sockfd);
    }
}

void* handle_client_command(void *p){
    //判断队列是否为空,为空就等待,否则就工作
    p_factory pfac = (p_factory)p;
    p_queue que = pfac->task_queue;
    elem_t new_task;    
    while(1){
        pthread_mutex_lock(&que->mutex);
        if(is_empty(que)){
            //队列为空,等待任务到来
            printf("队列为空,先等等!我是线程:%lu\n", pthread_self()%100);
            pthread_cond_wait(&pfac->cond, &que->mutex);
        }
        printf("队列非空!我要干活啦！我是线程:%lu\n", pthread_self()%100);
        dequeue(que, &new_task); //获取任务
        pthread_mutex_unlock(&que->mutex);
        printf("获取任务成功，fd=%d 开始干！我是线程:%lu\n", new_task.newfd, pthread_self()%100);
        //tran_file(new_task.newfd); //发送文件
        ftp_server(&new_task);
        close(new_task.newfd);
    }

    return NULL;
}

void init_factory(p_factory pfac, p_config pconf){
    pfac->p_threads = (pthread_t*)calloc(pconf->thread_num, sizeof(pthread_t));
    if(pfac->p_threads == NULL){
        FatalError("Out of space!");
    }
    pfac->thread_num = pconf->thread_num;;
    pthread_cond_init(&pfac->cond, NULL);
    pfac->task_queue = init_queue(pconf->queue_capacity);
    pfac->start_flag = 0;
}

void start_factory(p_factory pfac){
    int i;
    if(pfac->start_flag == 0){
        for(i = 0; i < pfac->thread_num; i++){
            pthread_create(pfac->p_threads+i, NULL, handle_client_command, pfac);
        } 
    }
}
