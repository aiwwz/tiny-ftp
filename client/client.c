#include <func.h>

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

int main(int argc, char* argv[]){
    ARGS_CHECK(argc, 3);

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
