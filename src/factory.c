#include "../include/factory.h"

void* download_file(void *p){
    //判断队列是否为空,为空就等待,否则就工作
    p_factory pfac = (p_factory)p;
    p_queue que = pfac->task_queue;
    elem_t new_task;    
    while(1){
        pthread_mutex_lock(&que->mutex);
        if(is_empty(que)){
            //队列为空,等待任务到来
            printf("队列为空,先等等!我是线程:%lu\n", pthread_self()%100);
            pthread_cond_wait(&pfac->cond, &que->mutex);
        }
        printf("队列非空!我要干活啦！我是线程:%lu\n", pthread_self()%100);
        dequeue(que, &new_task); //获取任务
        pthread_mutex_unlock(&que->mutex);
        printf("获取任务成功，fd=%d 开始干！我是线程:%lu\n", new_task.newfd, pthread_self()%100);
        tran_file(new_task.newfd); //发送文件
    }

    return NULL;
}

void init_factory(p_factory pfac, p_config pconf){
    pfac->p_threads = (pthread_t*)calloc(pconf->thread_num, sizeof(pthread_t));
    if(pfac->p_threads == NULL){
        FatalError("Out of space!");
    }
    pfac->thread_num = pconf->thread_num;;
    pthread_cond_init(&pfac->cond, NULL);
    pfac->task_queue = init_queue(pconf->queue_capacity);
    pfac->start_flag = 0;
}

void start_factory(p_factory pfac){
    int i;
    if(pfac->start_flag == 0){
        for(i = 0; i < pfac->thread_num; i++){
            pthread_create(pfac->p_threads+i, NULL, download_file, pfac);
        } 
    }
}
