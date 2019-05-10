#include <stdio.h>
#include <mysql/mysql.h>
#include <string.h>
#include "../include/user_info.h"

int connect_to_database(MYSQL **conn){
    const char *server = "localhost";
    const char *user = "root";
    const char *password = "123";
    const char *database = "ftp_database"; //ftp服务器的数据库

    *conn = mysql_init(NULL);
    if(!mysql_real_connect(*conn, server, user, password, database, 0, NULL, 0)){
        printf("Error connecting to database:%s.\n", mysql_error(*conn));
        return -1;
    }
    else{
        printf("Connected to database...\n");
        return 0;
    }
}

int db_get_user(p_user_info info){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    connect_to_database(&conn); //连接到数据库

    char query[300];
    sprintf(query, "select * from user_tbl where username='%s'", info->username);
    int ret;
    ret = mysql_query(conn, query);
    if(ret){
        printf("Error making query:%s.\n", mysql_error(conn));
        return -1;
    }
    else{
        res = mysql_use_result(conn);
        if(res){
            row = mysql_fetch_row(res);
            if(row != NULL){
              //sprintf(info->username, "%s", row[0]);
              sprintf(info->salt, "%s", row[1]);
              sprintf(info->passwd, "%s", row[2]);
            }
            else{
                printf("Don't find data!\n");
                return -1;
            }
        }
        else{
            printf("Don't find data!\n");
            return -1;
        }
    }
    mysql_close(conn);
    return 0;
}

/********************************************************
 *   功能: 在code为dir_code的目录下寻找名为dir_name的目录
 * 返回值: 未找到该目录 -- -1
 *           找到该目录 -- 该目录的code
 ********************************************************/
int db_find_dir(int dir_code, const char *dir_name){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    int curr_code = -1;

    connect_to_database(&conn); //连接到数据库

    char query[300];
    sprintf(query, "select code from file_tbl where precode=%d and filename='%s' and st_mode=%d", dir_code, dir_name, ST_MODE_DIR);
    int ret;
    ret = mysql_query(conn, query);
    if(ret){
        printf("Error making query:%s.\n", mysql_error(conn));
        return -1;
    }
    else{
        res = mysql_use_result(conn);
        if(res != NULL){
            row = mysql_fetch_row(res);
            if(row != NULL){
                curr_code = atoi(row[0]);
            }
            else{
                printf("Don't find data!\n");
            }
        }
        else{
            printf("Don't find data!\n");
        }
    }
    mysql_close(conn);
    return curr_code;
}


/*****************************************
 *   功能: 获取code为file_code的文件信息
 * 返回值: 0 -- 找到该文件
 *        -1 -- 未找到该文件
 *   参数: p_info -- 返回值参数
 *****************************************/
int db_get_file(int file_code, p_file_info p_info){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    connect_to_database(&conn); //连接到数据库

    char query[300];
    sprintf(query, "select * from file_tbl where code=%d", file_code);
    int ret;
    ret = mysql_query(conn, query);
    if(ret){
        printf("Error making query:%s.\n", mysql_error(conn));
        return -1;
    }
    else{
        res = mysql_use_result(conn);
        if(res != NULL){
            row = mysql_fetch_row(res);
            if(row != NULL){
                p_info->precode = atoi(row[0]);
                p_info->code = atoi(row[1]);
                strcpy(p_info->filename, row[2]);
                p_info->st_mode = atoi(row[3]);
                p_info->st_size = atoi(row[4]);
                strcpy(p_info->md5sum, row[5]);
                strcpy(p_info->owner, row[6]);
            }
            else{
                printf("Don't find data!\n");
                return -1;
            }
        }
        else{
            printf("Don't find data!\n");
            return -1;
        }
    }
    mysql_close(conn);
    return 0;
}

/* 翻转begin与end之间的字符串 */
void reverse(char *begin, char *end){
    if(begin == NULL || end == NULL){
        return;
    }
    char tmp;
    while(begin != end && begin != end+1){ //注意奇数和偶数个字符的区别
        tmp = *begin;
        *begin = *end;
        *end = tmp;
        begin++;
        end--;
    }

}

/******************************************
 *   功能:通过工作目录code获取当前工作路径
 * 返回值: 0 -- 成功
 *        -1 -- 失败
 *   参数: pwd -- 返回值参数
 ******************************************/
int db_get_pwd(int dir_code, char *pwd){
    file_info_t file;
    int code = dir_code;
    int ret = 0;
    int filename_len = 0, pwd_len = 0;

    sprintf(pwd, "/");

    //获取从当前目录到根目录的路径
    while(code != 0){
        ret = db_get_file(code, &file);
        if(ret == -1){
            return -1; //查找目录失败
        }
        else{
            code = file.precode; //继续查找其父目录
            filename_len = strlen(file.filename);
            sprintf(pwd+pwd_len, "%s/", file.filename);
            pwd_len = pwd_len + filename_len + 1; //1是为了'/'
        }
    }
    //此时pwd是逆置的,需将其翻转
    //首先翻转两个/之间的字符串
    char *begin = pwd;
    char *end   = strchr(pwd, '/');
    while(end != NULL){
        reverse(begin, end-1);
        begin = end + 1;
        end = strchr(end+1, '/');
    }
    //然后翻转整体
    reverse(pwd, pwd+pwd_len-1);

    return 0;
}

