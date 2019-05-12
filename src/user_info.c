#include <stdio.h>
#include <mysql/mysql.h>
#include <string.h>
#include "../include/user_info.h"
#include <sys/stat.h>


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

    char query[512];
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

    char query[512];
    //调试信息
    //printf("db_find_dir:code:%d, name:%s\n", dir_code, dir_name);
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

/**********************************************************
 *   功能: 在dir_code对应的目录下寻找名为filename的文件
 * 返回值: 找到该文件 -- 该文件code
 *       未找到该文件 -- -1
 **********************************************************/
int db_find_file(const char *username, int dir_code, const char *filename){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    int ret_code = 0;

    connect_to_database(&conn); //连接到数据库

    char query[512];
    sprintf(query, "select code from file_tbl where owner='%s' and precode=%d and filename='%s'", username, dir_code, filename);
    int ret;
    ret = mysql_query(conn, query);
    if(ret){
        printf("Error making query:%s.\n", mysql_error(conn));
        ret_code = -1;
    }
    else{
        res = mysql_use_result(conn);
        if(res != NULL){
            row = mysql_fetch_row(res);
            if(row != NULL){
                ret_code = atoi(row[0]);
            }
            else{
                printf("Don't find data!\n");
                ret_code = -1;
            }
        }
        else{
            printf("Don't find data!\n");
            ret_code = -1;
        }
    }
    printf("ret_code:%d\n", ret_code);
    mysql_close(conn);
    return ret_code;
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
                if(row[5] != NULL){
                    strcpy(p_info->md5sum, row[5]);
                }
                else{
                    //do nothing!
                }
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


/**********************************************************
 *   功能: 在code等于dir_code的目录下创建名为dir_name的目录
 * 返回值: 0 -- 创建目录成功
 *        -1 -- 失败: 目标目录已存在
 *        -2 -- 失败: 编号为code的文件不是目录
 *        -3 -- 失败: 数据库处理失败
 **********************************************************/
int db_create_dir(const char *username, int dir_code, const char *dir_name){
    MYSQL *conn;
    file_info_t file;
    int ret;

    //检查code对应的文件是不是目录
    if(dir_code == 0){
        //是根目录
        file.st_mode = ST_MODE_DIR;
    }
    else{
        //非根目录,查找数据库验证
        db_get_file(dir_code, &file);
    }
    if(!S_ISDIR(file.st_mode)){
        return -2; //编号为code的文件不是目录
    }

    //检查目标目录是否已存在
    ret = db_find_dir(dir_code, dir_name);
    if(ret != -1){
        return -1; //目标目录已存在
    }

    //创建新目录
    connect_to_database(&conn); //连接到数据库

    char query[512];
    sprintf(query, "insert into file_tbl(precode, filename, st_mode, st_size, owner) values(%d, '%s', %d, %d, '%s')", dir_code, dir_name, ST_MODE_DIR, 0, username);
    ret = mysql_query(conn, query);
    if(ret){
        printf("Error makeing query:%s\n", mysql_error(conn));
        return -3;
    }
    else{
        printf("insert success!\n");
    }
    mysql_close(conn);
    return 0;
}


/*****************************************************
 *   功能: 在code等于dir_code的目录下创建文件file
 * 返回值: 0 -- 创建文件成功
 *        -1 -- 失败: 目标目录已存在
 *        -2 -- 失败: 编号为code的文件不是目录
 *        -3 -- 失败: 数据库处理失败
 *****************************************************/
int db_create_file(const char *username, int dir_code, p_file_infofile file){
    MYSQL *conn;
    file_info_t file;
    int ret;

    //检查code对应的文件是不是目录
    if(dir_code == 0){
        //是根目录
        file.st_mode = ST_MODE_DIR;
    }
    else{
        //非根目录,查找数据库验证
        db_get_file(dir_code, &file);
    }
    if(!S_ISDIR(file.st_mode)){
        return -2; //编号为code的文件不是目录
    }

    //检查目标目录是否已存在
    ret = db_find_dir(dir_code, dir_name);
    if(ret != -1){
        return -1; //目标目录已存在
    }

    //创建新目录
    connect_to_database(&conn); //连接到数据库

    char query[512];
    sprintf(query, "insert into file_tbl(precode, filename, st_mode, st_size, owner) values(%d, '%s', %d, %d, '%s')", dir_code, dir_name, ST_MODE_DIR, 0, username);
    ret = mysql_query(conn, query);
    if(ret){
        printf("Error makeing query:%s\n", mysql_error(conn));
        return -3;
    }
    else{
        printf("insert success!\n");
    }
    mysql_close(conn);
    return 0;
}



/***********************************************
 *   功能: 判断dir_code对应的目录是不是空目录
 * 返回值: 1 -- 是空目录
 *         0 -- 非空目录
 *        -1 -- 失败: dir_code对应的文件不存在
 *        -2 -- 失败: dir_code对应的文件不是目录
 *        -3 -- 失败: 数据库处理失败
 ***********************************************/
int db_dir_empty(const char *username, int dir_code){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    file_info_t file;
    int ret;

    //检查dir_code对应的文件是不是目录
    if(dir_code == 0){
        //是根目录
        file.st_mode = ST_MODE_DIR;
    }
    else{
        //非根目录,查找数据库验证
        ret = db_get_file(dir_code, &file);
        if(ret == -1){
            return -1; //该文件不存在
        }
    }
    if(!S_ISDIR(file.st_mode)){
        return -2; //编号为dir_code的文件不是目录
    }

    //判断该目录是不是空目录
    connect_to_database(&conn); //连接到数据库

    char query[512];
    sprintf(query, "select * from file_tbl where owner='%s' and precode=%d", username, dir_code);
    ret = mysql_query(conn, query);
    if(ret){
        printf("Error makeing query:%s\n", mysql_error(conn));
        return -3; //数据库处理失败
    }
    else{
        res = mysql_use_result(conn);
        if(res){
            row = mysql_fetch_row(res);
            if(row != NULL){ //目录非空
                return 0;
            }
            else{
                printf("Don't find data!\n");
                return 1;
            }
        }
        else{
            printf("Don't find data!\n");
            return 1;
        }
    }
    mysql_close(conn);
}


/*****************************************
 *   功能: 删除dir_code对应的目录
 * 返回值: 0 -- 删除成功
 *        -1 -- 失败: dir_code对应的文件不存在
 *        -2 -- 失败: dir_code对应的文件不是目录
 *        -3 -- 失败: 数据库处理失败
 ***********************************************/
int db_remove_dir(const char *username, int dir_code){
    MYSQL *conn;
    file_info_t file;
    int ret, ret_val = 0;

    //检查dir_code对应的文件是不是目录
    if(dir_code == 0){
        //根目录无法删除
        ret_val = -1;
    }
    else{
        //非根目录,查找数据库验证
        ret = db_get_file(dir_code, &file);
        if(ret == -1){
            ret_val = -1; //该文件不存在
        }
    }
    if(!S_ISDIR(file.st_mode)){
        ret_val = -2; //编号为dir_code的文件不是目录
    }

    if(ret_val == 0){
        //该目录存在, 则删除该目录
        connect_to_database(&conn); //连接到数据库

        char query[512];
        sprintf(query, "delete from file_tbl where owner='%s' and code=%d", username, dir_code);
        ret = mysql_query(conn, query);
        if(ret){
            printf("Error makeing query:%s\n", mysql_error(conn));
            ret_val = -3; //数据库处理失败
        }
        else{
            printf("delete success,delete row=%ld\n",(long)mysql_affected_rows(conn));
            ret_val = 0; //删除成功
        }
        mysql_close(conn);
    }
    return ret_val;
}


/************************************************
 *   功能: 删除file_code对应的普通文件
 * 返回值: 0 -- 删除成功
 *        -1 -- 失败: file_code对应的文件不存在
 *        -2 -- 失败: file_code对应的文件不是普通文件
 *        -3 -- 失败: 数据库处理失败
 ***********************************************/
int db_remove_file(const char *username, int file_code){
    MYSQL *conn;
    file_info_t file;
    int ret, ret_val = 0;

    //检查dir_code对应的文件是不是普通文件
    if(file_code == 0){
        //根目录无法删除
        ret_val = -2;
    }
    else{
        //非根目录,查找数据库验证
        ret = db_get_file(file_code, &file);
        if(ret == -1){
            ret_val = -1; //该文件不存在
        }
    }
    if(!S_ISREG(file.st_mode)){
        ret_val = -2; //编号为dir_code的文件不是普通文件
    }

    if(ret_val == 0){
        //该文件存在, 则删除该文件
        connect_to_database(&conn); //连接到数据库

        char query[512];
        sprintf(query, "delete from file_tbl where owner='%s' and code=%d", username, file_code);
        ret = mysql_query(conn, query);
        if(ret){
            printf("Error makeing query:%s\n", mysql_error(conn));
            ret_val = -3; //数据库处理失败
        }
        else{
            printf("delete success,delete row=%ld\n",(long)mysql_affected_rows(conn));
            ret_val = 0; //删除成功
        }
        mysql_close(conn);
    }
    return ret_val;
}


/*******************************************
 *   功能: 获取dir_code目录下的所有文件信息
 * 返回值: 0 -- 成功
 *        -1 -- 未找到该文件
 *   参数: pp_info -- 指向文件信息列表
 *******************************************/
int db_list_info(const char *username, int dir_code, file_info_t **pp_info){
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300];
    file_info_t file;
    int ret, ret_val, rows;

    if(dir_code == 0){ //根目录
        file.st_mode = ST_MODE_DIR;
    }
    else{ //非根目录,查询数据库
        db_get_file(dir_code, &file);
    }

    if(S_ISDIR(file.st_mode)){ //是目录则获取目录中所有文件的信息
        sprintf(query, "select code, filename, st_mode, st_size from file_tbl where owner='%s' and precode=%d", username, dir_code);
    }
    else{ //是普通文件则仅获取该文件信息
        sprintf(query, "select code, filename, st_mode, st_size from file_tbl where owner='%s' and code=%d", username, dir_code);
    }

    connect_to_database(&conn); //连接到数据库
    
    ret = mysql_query(conn, query);
    if(ret){
        printf("Error making query:%s.\n", mysql_error(conn));
        ret_val = -1;
    }
    else{
        res = mysql_store_result(conn);
        if(res != NULL){
            rows = mysql_num_rows(res); 
            if(rows != 0){
                *pp_info = (p_file_info)calloc(rows+1, sizeof(file_info_t)); //1是为了存储结束标志
                int i;
                for(i = 0; i < rows; i++){ //获取文件信息
                    row = mysql_fetch_row(res);
                    (*pp_info)[i].code = atoi(row[0]);
                    strcpy((*pp_info)[i].filename, row[1]);
                    (*pp_info)[i].st_mode = atoi(row[2]);
                    (*pp_info)[i].st_size = atoi(row[3]);
                }
                (*pp_info)[i].code = 0; //作为结束标志
                ret_val = 0;
            }
            else{
                printf("1:Don't find data!\n");
                ret_val = -1;
            }
        }
        else{
            printf("2:Don't find data!\n");
            ret_val = -1;
        }
    }
    mysql_close(conn);
    return ret_val;
}
