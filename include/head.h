#ifndef POOL_H
#define POOL_H

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/msg.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define ARGS_CHECK(argc,val) {if(argc!=val) \
	{printf("error args\n"); exit(-1);}}

#define ERROR_CHECK(ret,retval,funcName) {if(ret==retval) \
	{perror(funcName); exit(-1);}}

#define THREAD_ERROR_CHECK(ret, funcName){if(ret!=0) \
	{printf("%s:%s\n", funcName, strerror(ret)); return -1;}}


typedef struct{
	pid_t pid; 	//child pid
	int fd; 	//parent end of the tcp_pipe
	short busy; //is the child busy? 0:not 1:yes
}process_t;


void child_init(process_t*, int);
int child_handle(int);
void send_fd(int sfd, int fd, int exit_flag);
void recv_fd(int sfd, int *fd, int *exit_flag);
void tran_file(int newfd);


/* 套接字接口包装函数 */
int Socket(int domain, int type, int protocol);
void Bind(int sockfd, struct sockaddr *addr, int addrlen);
void Listen(int sockfd, int backlog);
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
void Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int Tcp_init(struct in_addr sin_addr, unsigned short sin_port);
void Socketpair(int, int, int, int*);


/* epoll包装函数 */
int Epoll_create(int size);
void Epoll_ctl(int epfd, int op, int fd, struct epoll_event *events);
int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);


/* I/O包装函数 */
int Open(const char *pathname, int flags);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
ssize_t Send(int sockfd, const void *buf, size_t len, int flags);
ssize_t Recv(int sockfd, void *buf, size_t len, int flags);
void Sendmsg(int sockfd, const struct msghdr *msg, int flags);
ssize_t Recvmsg(int sockfd, struct msghdr *msg, int flags);


#endif /*POOL_H*/
