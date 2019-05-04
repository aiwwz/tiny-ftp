/******************************
 * 功能: 队列的类型声明 
 * 作者: 王志军
 * 日期: 20190430
 ******************************/
#ifndef __QUEUE_H__
#define __QUEUE_H__
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define MIN_QUEUE_SIZE 3 //队列最小容量

typedef struct{
    int newfd;
}elem_t; //elem_t未来可扩充

typedef struct queue_record{
    elem_t *array; //队列
    int capacity;  //容量
    int size;      //实际任务数
    int front;     //头下标
    int rear;      //尾下标
    pthread_mutex_t mutex;
}queue_t, *p_queue;

/* 返回值: 空/满--返回 1
 *         否则 --返回 0  */
int is_empty(p_queue que);
int is_full(p_queue que);

p_queue init_queue(int max_elems); //初始化
void destroy_queue(p_queue que);             //销毁队列
void enqueue(p_queue que, elem_t *p_elem);   //入队
void dequeue(p_queue que, elem_t *p_elem);   //出队
void   front(p_queue que, elem_t *p_elem);   //仅返回队首元素但不出队

/* 错误处理 */
#define Error(str) fprintf(stderr, "%s\n", str), exit(1)
#define FatalError(str) Error(str)

#endif /*__QUEUE_H__*/
