#include "../include/factory.h"

int main(int argc, char *argv[]){

    config_t conf;
    load_config(&conf);
    printf("配置成功:\n");
    printf("ip:%u\nport:%u\nthread_num:%u\nqueue_capacity:%u\n", conf.sin_addr.s_addr, conf.sin_port, conf.thread_num, conf.queue_capacity);

    factory_t fac;
    init_factory(&fac, &conf);
    start_factory(&fac);

    int listenfd, newfd;
    listenfd = Tcp_init(conf.sin_addr, conf.sin_port); 

    p_queue task_queue = fac.task_queue;
    elem_t new_elem;
    while(1){
        newfd = Accept(listenfd, NULL, NULL);
        printf("Yes! newfd=%d is coming!\n", newfd);
        new_elem.newfd = newfd;
        pthread_mutex_lock(&task_queue->mutex);
        enqueue(task_queue, &new_elem);
        pthread_mutex_unlock(&task_queue->mutex);
        printf("伙计们:队列中有任务了!\n");
        pthread_cond_signal(&fac.cond);
    }

    return 0;
}
