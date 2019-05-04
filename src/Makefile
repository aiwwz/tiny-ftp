SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:=../bin/thread_pool_server
CC:=gcc
CFLAGS:=-g -Wall
$(ELF):$(OBJS)
	gcc $^ -o $@ -pthread
.PHONY:clean
clean:
	rm -rf $(ELF) $(OBJS)

