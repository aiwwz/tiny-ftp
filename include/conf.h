#ifndef __CONF_H__
#define __CONF_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* 常用宏函数 */
#define IsLetter(ch)    (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z')
#define IsDigit(ch)     (ch >= '0' && ch <= '9')
#define IsDot(ch)       (ch == '.')
#define IsUnderline(ch) (ch == '_')
#define IsNullChar(ch)  (ch == ' ' || ch == '\t' || ch == '\n')
#define IsComment(ch)   (ch == '#')

/* 配置信息 */
typedef struct{
    struct in_addr sin_addr;   //ip
    unsigned short sin_port;   //port
    unsigned short thread_num; //线程数
    unsigned short queue_capacity; //任务队列容量
}config_t, *p_config; 

/* 单词标签 */
typedef enum tag{
    /* 配置项 */
    SERVER_IP,      //服务器ip
    SERVER_PORT,    //监听端口
    THREAD_NUM,     //线程数
    QUEUE_CAPACITY, //队列容量

    /* 运算符 */
    ASSIGN,  //赋值运算符 ‘=’

    /* 常量 */
    NUM,     //数字
    IP,      //ip地址

    /* 文件结束符 */
    END      //文件结束符EOF
}tag_t;

/* 函数声明 */
void next_char();
void write_back();
void parse_item();
void parse_num();
void next_useful_char();
void next_token();
int check(const char *item);
void expect();
void skip();
void default_config(p_config conf);
void config(p_config conf);
void parse_config(p_config conf);
void load_config(p_config conf);

#endif /*__CONF_H__*/
