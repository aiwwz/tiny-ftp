#include "../include/head.h"

void send_fd(int sfd,int fd, int exit_flag){
	struct msghdr msg;
	memset(&msg,0,sizeof(msg));
	struct iovec iov[2];
	char buf[6] = "hello";
	iov[0].iov_base=&exit_flag;
	iov[0].iov_len=4;
	iov[1].iov_base=buf;
	iov[1].iov_len=5;
	msg.msg_iov=iov;
	msg.msg_iovlen=2;
	struct cmsghdr *cmsg;
	int len=CMSG_LEN(sizeof(int));
	cmsg=(struct cmsghdr *)calloc(1,len);
	cmsg->cmsg_len=len;
	cmsg->cmsg_level=SOL_SOCKET;
	cmsg->cmsg_type=SCM_RIGHTS;
	*(int*)CMSG_DATA(cmsg)=fd;
	msg.msg_control=cmsg;
	msg.msg_controllen=len;
	Sendmsg(sfd,&msg,0);
}

void recv_fd(int sfd,int *fd, int *exit_flag){
	struct msghdr msg;
	memset(&msg,0,sizeof(msg));
	struct iovec iov[2];
	char buf[6];
	iov[0].iov_base=exit_flag;
	iov[0].iov_len=4;
	iov[1].iov_base=buf;
	iov[1].iov_len=5;
	msg.msg_iov=iov;
	msg.msg_iovlen=2;
	struct cmsghdr *cmsg;
	int len=CMSG_LEN(sizeof(int));
	cmsg=(struct cmsghdr *)calloc(1,len);
	cmsg->cmsg_len=len;
	cmsg->cmsg_level=SOL_SOCKET;
	cmsg->cmsg_type=SCM_RIGHTS;
	msg.msg_control=cmsg;
	msg.msg_controllen=len;
	Recvmsg(sfd,&msg,0);
	*fd=*(int*)CMSG_DATA(cmsg);
}
