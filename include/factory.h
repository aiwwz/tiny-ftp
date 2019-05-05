#ifndef __FACTORY_H__
#define __FACTORY_H__
#include "head.h" 
#include "queue.h"
#include "conf.h"

typedef struct{
    pthread_t *p_threads; //存储线程
    int thread_num;       //线程数目
    pthread_cond_t cond;  //每个线程都要使用的条件变量
    p_queue task_queue;   //任务队列
    short start_flag;     //工厂启动标志
}factory_t, *p_factory;

void init_factory(p_factory pfac, p_config pconf);
void start_factory(p_factory pfac);

#endif /*__FACTORY_H__*/
