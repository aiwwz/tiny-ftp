#include "../include/head.h"

/*******************
 *  套接字包装函数
 *******************/
int Socket(int domin, int type, int protocol){
	int ret = socket(domin, type, protocol);
	ERROR_CHECK(ret, -1, "socket");
	return ret;
}

void Bind(int sockfd, struct sockaddr *addr, int addrlen){
	int ret = bind(sockfd, addr, addrlen);
	ERROR_CHECK(ret, -1, "bind");
}

void Listen(int sockfd, int backlog){
	int ret = listen(sockfd, backlog);
	ERROR_CHECK(ret, -1, "listen");
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t  *addrlen){
	int ret = accept(sockfd, addr, addrlen);
	ERROR_CHECK(ret, -1, "accept");
	return ret;
}

void Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen){
	int ret = setsockopt(sockfd, level, optname, optval, optlen);
	ERROR_CHECK(ret, -1, "setsockopt");
}

int Tcp_init(struct in_addr sin_addr, unsigned short sin_port){
	int sockfd;
	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(sin_port);
	addr.sin_addr = sin_addr;
	int reuse = 1;
	Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(int));
	Bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
	Listen(sockfd, 1024);

	return sockfd;
}

void Socketpair(int domain, int type, int protocol, int sv[2]){
	int ret = socketpair(domain, type, protocol, sv);
	ERROR_CHECK(ret, -1, "socketpair");	
}


/*******************
 *  epoll包装函数
 *******************/
int Epoll_create(int size){
	int ret = epoll_create(size);
	ERROR_CHECK(ret, -1, "epoll_create");
	return ret;
}

void Epoll_ctl(int epfd, int op, int fd, struct epoll_event *events){
	int ret = epoll_ctl(epfd, op, fd, events);
	ERROR_CHECK(ret, -1, "epoll_ctl");
}

int Epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout){
	int ret = epoll_wait(epfd, events, maxevents, timeout);
	ERROR_CHECK(ret, -1, "epoll_wait");
	return ret;
}


/*******************
 *   I/O包装函数 
 *******************/
int Open(const char *pathname, int flags){
	int ret = open(pathname, flags);
	ERROR_CHECK(ret, -1, "open");
	return ret;	
}

ssize_t Read(int fd, void *buf, size_t count){
	ssize_t ret = read(fd, buf, count);
	ERROR_CHECK(ret, -1, "read");
	return ret;
}

ssize_t Write(int fd, const void *buf, size_t count){
	ssize_t ret = write(fd, buf, count);
	ERROR_CHECK(ret, -1, "write");
	return ret;
}

ssize_t Send(int sockfd, const void *buf, size_t len, int flags){
	ssize_t ret = send(sockfd, buf, len, flags);
	ERROR_CHECK(ret, -1, "send");
	return ret;
}

ssize_t Recv(int sockfd, void *buf, size_t len, int flags){
	ssize_t ret = recv(sockfd, buf, len, flags);
	ERROR_CHECK(ret, -1, "recv");
	return ret;
}

void Sendmsg(int sockfd, const struct msghdr *msg, int flags){
	int ret = sendmsg(sockfd, msg, flags);
	ERROR_CHECK(ret, -1, "sendmsg");
}

ssize_t Recvmsg(int sockfd, struct msghdr *msg, int flags){
	ssize_t ret = recvmsg(sockfd, msg, flags);
	ERROR_CHECK(ret, -1, "recvmsg");
	return ret;
}

