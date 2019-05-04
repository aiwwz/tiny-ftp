#include "../include/head.h"

void child_init(process_t *pChilds, int childNum){	
	int i, fds[2];
	pid_t pid;
	for(i = 0; i < childNum; i++){
		Socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);
		if((pid = fork()) == 0){
			//child process
			close(fds[0]);
			child_handle(fds[1]);	
		}
		close(fds[1]);
		pChilds[i].pid = pid;
		pChilds[i].fd = fds[0];
		pChilds[i].busy = 0;
	}
}

int child_handle(int fd){
    int newfd, exit_flag;
    while(1){
        recv_fd(fd, &newfd, &exit_flag);
        if(exit_flag){
	        tran_file(newfd);
            close(newfd);
        }
        else{
            exit(0);
        }
        Write(fd, "N", 1); //通知父进程我非忙碌了
	}
	return 0;
}
