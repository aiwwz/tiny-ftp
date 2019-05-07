SRCS:=$(wildcard *.c)
ELFS:=$(SRCS:%.c=%)
all:$(ELFS)
		@for i in $(ELFS);do gcc -o $${i} $${i}.c;done
clean:
		rm -rf $(ELFS)

