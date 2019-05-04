#ifndef __TRAN_FILE_H__
#define __TRAN_FILE_H__

#include "head.h"

#define FILENAME "file"

//小火车
typedef struct{
	int data_len;	//装载内容长度
	char buf[1000];	//实际数据
}train_t;

void tran_file(int newfd);

#endif //__TRAN_FILE_H__
