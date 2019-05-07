SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:=ftp_client
CC:=gcc
CFLAGS:=-g -Wall
$(ELF):$(OBJS)
	gcc $^ -o $@ -pthread -I /usr/include/mysql/ -lmysqlclient
.PHONY:clean
clean:
	rm -rf $(ELF) $(OBJS)

