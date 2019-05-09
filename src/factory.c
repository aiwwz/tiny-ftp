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

    printf("\033[34m%s\033[0m", buf);
    *cmds = split_cmds(buf);
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

/* 为PASV模式提供新端口 */
short get_data_port(){
    //待实现:由配置文件指定可用端口范围
    return 3000;
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

void ftp_server(elem_t *task){
    char username[128] = {0};
    char info[512] = {0};
    char pwd[1024] = "/";

    printf("Connected, sending welcome message...\n");
    send_welcome_message(task->newfd); //发送欢迎信息

    char **cmds = NULL;      //客户端发来的参数列表
    int logged = LOGGED_OUT; //登录状态
    int data_sockfd = -1;    //PASV模式的数据端口
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
                strcpy(username, cmds[1]); //获取用户名
                sprintf(info, "Password required for %s", username);
                server_send_reply(task, 331, info); //请求客户端发送密码
                server_recv_cmd(task, &cmds);
                if(strcmp(cmds[0], "PASS") == 0){
                    if(sizeof_cmd(cmds) < 2){ //null passwd
                        server_send_reply(task, 530, "Login or password incorrect!");
                        break;
                    }
                   if(ftp_strcmp(cmds[1], "123") == 0){
                        //登录成功！
                        server_send_reply(task, 230, "Logged on");
                        logged = LOGGED_ON;
                    }
                    else{
                        //登录失败
                        server_send_reply(task, 530, "Login or password incorrect!");
                        logged = LOGGED_OUT;
                    }
                    continue;
                }
            }
            else if(ftp_strcmp(cmds[0], "CWD") == 0){
                
            }
            else if(ftp_strcmp(cmds[0], "LIST") == 0){

            }
            else if(ftp_strcmp(cmds[0], "DELE") == 0){

            }
            else if(ftp_strcmp(cmds[0], "MKD") == 0){

            }
            else if(ftp_strcmp(cmds[0], "RMD") == 0){

            }
            else if(ftp_strcmp(cmds[0], "RETR") == 0){

            }
            else if(ftp_strcmp(cmds[0], "QUIT") == 0){

            }
            else if(ftp_strcmp(cmds[0], "PWD") == 0){
                sprintf(info, "\"%s\" is current directory.", pwd);
                server_send_reply(task, 257, info);
            }
            else if(ftp_strcmp(cmds[0], "TYPE") == 0){
                if(sizeof_cmd(cmds) < 2){
                    server_send_reply(task, 503, "Syntax error, nead a parameter:A/B");
                    break;
                }
                else if(ftp_strcmp(cmds[1], "A")){ //ASCII模式传输
                    //trans_mode = ASCII_MODE;
                    server_send_reply(task, 200, "Type set to A.");
                }
                else if(ftp_strcmp(cmds[1], "B")){ //Binary模式传输
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
                data_sockfd = Tcp_init(task->sin_addr, data_port); 
                sprintf(info, "Entering Passive Mode (%s,%d,%d)", str_replace(inet_ntoa(task->sin_addr), '.', ','), data_port/256, data_port%256);
                server_send_reply(task, 227, info);
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
                strcpy(username, cmds[1]); //获取用户名
                sprintf(info, "Password required for %s", username);
                server_send_reply(task, 331, info); //请求客户端发送密码
                server_recv_cmd(task, &cmds);
                if(strcmp(cmds[0], "PASS") == 0){
                    if(sizeof_cmd(cmds) < 2){ //null passwd
                        server_send_reply(task, 530, "Login or password incorrect!");
                        break;
                    }
                   if(ftp_strcmp(cmds[1], "123") == 0){
                        //登录成功！
                        server_send_reply(task, 230, "Logged on");
                        logged = LOGGED_ON;
                    }
                    else{
                        //登录失败
                        server_send_reply(task, 530, "Login or password incorrect!");
                        logged = LOGGED_OUT;
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
