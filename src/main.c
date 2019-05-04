#include "../include/factory.h"

typedef struct{
    struct in_addr sin_addr;       //ip
    unsigned short int sin_port;   //port
    unsigned short int thread_num; //线程数
    unsigned short int queue_capacity; //任务队列容量
}config_t;

void load_config(){
    FILE *fp = fopen("../conf/server.conf", "rb");
    if(fp == NULL){
        printf("未找到配置文件, 将使用默认配置!\n");
    }

    config_t conf;
    
    //默认配置
    conf.sin_addr.s_addr = 0; //默认捆绑本机ip
    conf.sin_port = 2000;
    conf.thread_num = 5;
    conf.queue_capacity = 10;

    //读取配置文件
    char ch = '\0';
    char line[256] = {0};
    char  key[256] = {0};
    char  val[256] = {0};
    int i;
    while(fgets(line, sizeof(line), fp) != NULL){
        i = 0;
        while(line[i] == ' '){ //忽略开头空格
            i++;
        }
        while((ch = line[i]) != '=' && ch != ' '){ //获取待配置项
            key[i] = line[i];
            i++;
        }
        while((ch = line[i]) != '\n') //获取配置值     
    }
}

int main(int argc, char *argv[]){
    ARGS_CHECK(argc, 5);

    factory_t fac;
    int thread_num = atoi(argv[3]); //线程数
    int queue_capacity = atoi(argv[4]);  //队列最大容量

    load_config();

    init_factory(&fac, thread_num, queue_capacity);
    start_factory(&fac);

    int listenfd, newfd;
    listenfd = Tcp_init(argv[1], argv[2]); 

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
