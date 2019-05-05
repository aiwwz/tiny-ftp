#include "../include/conf.h"

/* 全局变量 */
char ch = '\0';        //当前字符
int line_num = 0;      //行号

enum tag token;        //当前符号
enum tag pre_token;    //上一个符号
char str[256] = {0};   //临时存放字符串
char item[256] = {0};  //存放配置项
struct in_addr ip_addr;//存放ip地址
short int num = 0;     //存放数字

FILE *fin;             //打开的配置文件


/* 错误处理 */
#define Error(Str) fprintf(stderr, "%s\n", Str)
#define FatalError(Str) fprintf(stderr, "%s\n", Str), exit(1)


/* 合法待配置项 */
const char *conf_items[] = {"server_ip",     //服务器ip
                            "server_port",   //监听端口号
                            "thread_num",    //线程数量
                            "queue_capacity",//任务队列容量
                            NULL
                            };

/* 读字符 */
void next_char() {
    if ((ch = fgetc(fin)) == '\n') {
        line_num++; //换行增加行号
    }

}


/* 回写 */
void write_back() {
    if (ungetc(ch, fin) == EOF) {
        FatalError("error: 回写失败!");
    }
}


/* 解析配置项 */
void parse_item() {
    memset(item, 0, sizeof(item)); //标识符初始化
    int i = 0;
    while (IsLetter(ch) || IsDigit(ch) || IsUnderline(ch)) {
        item[i++] = ch;
        next_char();
    }

    //多读了一个字符,回写
    write_back();

    //判断是否是合法配置项
    int ret = 0;
    if((ret = check(item)) == -1){
        fprintf(stderr, "%d:无效的配置项:%s\n", line_num, item);
        exit(1);
    }
    else{
        pre_token = token;
        token = (tag_t)ret;
    }

}


/* 解析数字常量与ip地址 */
void parse_num() {
    int i = 0;
    memset(str, 0, sizeof(str));
    if(pre_token == SERVER_IP){
        //解析ip地址
        while (IsDigit(ch) || IsDot(ch)) {
            str[i++] = ch;
            next_char();
        }
        //多读了一个字符,回写
        write_back();

        ip_addr.s_addr = inet_addr(str);
        pre_token = token;
        token = IP;
    }
    else{
        //解析数字常量
        while (IsDigit(ch)) {
            str[i++] = ch;
            next_char();
        }
        //多读了一个字符,回写
        write_back();

        num = atoi(str);
        pre_token = token;
        token = NUM;
    }
}


void parse_comment() {
    while (ch != '\n') { //读取到行尾
        next_char();
    }
}


/* 获取下一个可用的字符 */
void next_useful_char() {
    next_char();
    //跳过空格, 制表符, 换行符, 注释
    while (IsNullChar(ch) || ch == '#') {
        if (IsComment(ch)) {
            parse_comment(); //解析注释
        }
        next_char();
    }
}


/* 获取下一个token */
void next_token(){
    //跳过空格,制表符,换行符,注释
    next_useful_char();

    if (IsLetter(ch) || IsUnderline(ch)) { //配置项
        parse_item();
                    
    }
    else if (IsDigit(ch)) { //数字或ip地址
        parse_num();
    }
    else{ //其他
        switch(ch){
        case '=':
            pre_token = token;
            token = ASSIGN;
            break;
        case EOF:
            pre_token = token;
            token = END;
            break;
        default:
            break;
        }
    }
}


/* 检查标识符是不是待配置项 */
int check(const char *id){
    const char *item;
    int i = 0;
    while((item = conf_items[i]) != NULL){
        if(strcmp(item, id) == 0){
            //合法配置项
            return i; //注:此标号和enum tag中对应
        }
        i++;
    }
    return -1;
}


void expect(tag_t expect_token){
    if(token != expect_token){
        fprintf(stderr, "expect token:%d, but token is %d\n", expect_token, token);
        exit(1);
    }
}

void skip(tag_t expect_token){
    if(token == expect_token){
        next_token();
    }
}

/* 默认配置 */
void default_config(p_config conf){
    conf->sin_addr.s_addr = 0; //默认捆绑本机ip
    conf->sin_port = 2000;
    conf->thread_num = 5;
    conf->queue_capacity = 10;
}


/*****************************************
 * <config> -> <item> ASSIGN <right_val> *
 *****************************************
 * <item> -> SERVER_IP                   *
 *         | SERVER_PORT                 *
 *         | THREAD_NUM                  *
 *         | QUEUE_CAPACITY              *
 *****************************************
 * <right_val> -> IP                     *
 *              | NUM                    *
 *****************************************/
void config(p_config conf){
    switch(token){
        case SERVER_IP:
            next_token();
            expect(ASSIGN);
            next_token();
            expect(IP);
            conf->sin_addr = ip_addr;
            break;
        case SERVER_PORT:
            next_token();
            expect(ASSIGN);
            next_token();
            expect(NUM);
            conf->sin_port = num;
            break;
        case THREAD_NUM:
            next_token();
            expect(ASSIGN);
            next_token();
            expect(NUM);
            conf->thread_num = num;
            break;
        case QUEUE_CAPACITY:
            next_token();
            expect(ASSIGN);
            next_token();
            expect(NUM);
            conf->queue_capacity = num;
            break;
        default:
            break;
    }
}


/* 解析配置 */
void parse_config(p_config conf){
    next_token();
    while(token != END){
        config(conf);
        next_token();
    }
}


/* 加载配置 */
void load_config(p_config conf){
    fin = fopen("../conf/server.conf", "rb");
    if(fin == NULL){                                        
        printf("未找到配置文件, 将使用默认配置!\n");
    }

    default_config(conf);

    parse_config(conf);
}

