#include "../include/tran_file.h"

void tran_file(int newfd){
	train_t train;
	
    //发送文件名
    train.data_len = strlen(FILENAME);
	strcpy(train.buf, FILENAME);
    int ret; 
    ret = send(newfd, &train, train.data_len+4, 0);
    if(ret == -1){
        printf("send失败！\n");      
        return;
    }
    printf("发送文件名成功!\n");

    int fd = Open(FILENAME, O_RDONLY);
   
    //发送文件大小
    struct stat st;
    fstat(fd, &st);
    ret = send(newfd, &st.st_size, sizeof(off_t), 0);
    if(ret == -1) return;
    printf("发送文件大小成功!\n");

    //发送文件内容
    while((train.data_len = Read(fd, train.buf, sizeof(train.buf)))){
        ret = send(newfd, &train, 4+train.data_len, 0);
        if(ret == -1) return;
    }
    printf("发送文件内容成功!\n");

    //发送结束标志
    send(newfd, &train, 4, 0); //此时train.date_len为0
    printf("发送结束标志成功!\n");
}
