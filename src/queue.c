/*************************************
 * 功能: 队列实现--采用动态分配的数组 
 * 作者: 王志军
 * 日期: 20190430
 *************************************/
#include "../include/queue.h"

int is_empty(p_queue que){
    if(0 == que->size){
        return 1;
    }
    else{
        return 0;
    }
}

int is_full(p_queue que){
    if(que->size == que->capacity){
        return 1;
    }
    else{
        return 0;
    }
}

p_queue init_queue(int max_elems){
    if(max_elems < MIN_QUEUE_SIZE){
        Error("Queue size is too small!");
    }

    //分配内存
    p_queue que;
    que = (p_queue)malloc(sizeof(queue_t));
    if(que == NULL){
        FatalError("Out of space!");
    }
    que->array = (elem_t*)malloc(sizeof(elem_t) * max_elems);
    if(que->array == NULL){
        FatalError("Out of space!");
    }

    //初始化
    que->capacity = max_elems;
    que->size = 0;
    que->front = 0;
    que->rear = -1; //初始rear在front前部
    //que->mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&que->mutex, NULL);

    return que;
}

void destory_queue(p_queue que){
    if(que != NULL){
        if(que->array != NULL){
            free(que->array);
        }
        free(que);
    }
}

static int next(p_queue que, int index){
    if(++index == que->capacity){
        index = 0;
    }
    return index;
}

void enqueue(p_queue que, elem_t *p_elem){
    if(is_full(que)){
        Error("Full queue!");
    }
    que->size++;
    que->rear = next(que, que->rear);
    que->array[que->rear] = *p_elem;
}

void dequeue(p_queue que, elem_t *p_elem){
    front(que, p_elem);
    que->size--;
    que->front = next(que, que->front);
}

void front(p_queue que, elem_t *p_elem){
    if(is_empty(que)){
        Error("Empty queue!");
    }
    *p_elem = que->array[que->front];
}
