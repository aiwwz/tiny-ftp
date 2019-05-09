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
    new_elem.sin_addr.s_addr = conf.sin_addr.s_addr; //为pasv模式提供本机ip
    socklen_t addrlen = sizeof(new_elem.client_addr);

    while(1){
        newfd = Accept(listenfd, (struct sockaddr*)&new_elem.client_addr, &addrlen);
        printf("new_client! ip:%s, port:%d\n", inet_ntoa(new_elem.client_addr.sin_addr), ntohs(new_elem.client_addr.sin_port));
        new_elem.newfd = newfd;
        pthread_mutex_lock(&task_queue->mutex);
        enqueue(task_queue, &new_elem);
        pthread_mutex_unlock(&task_queue->mutex);
        printf("伙计们:队列中有任务了!\n");
        pthread_cond_signal(&fac.cond);
    }

    return 0;
}
