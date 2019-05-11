#ifndef __USER_INFO_H__
#define __USER_INFO_H__

//下面两个宏是stat中st_mode文件类型对应于普通文件和目录文件的值
#define ST_MODE_REG 33204 
#define ST_MODE_DIR 16893
#define ROOT_DIR_CODE 0

typedef struct{
    char username[128];     //用户名
    char salt[16];          //盐值
    char passwd[128];       //密码密文
}user_info_t, *p_user_info;


typedef struct{
    int precode;       //该文件所属目录的编码
    int code;          //文件唯一编码
    char filename[64]; //文件名
    unsigned short st_mode; //文件类型
    long int st_size;  //文件大小
    char md5sum[33];   //md5码
    char owner[64];    //所有者
}file_info_t, *p_file_info;

int db_get_user(p_user_info p_info);
int db_find_dir(int dir_code, const char *dir_name);
int db_find_file(const char *username, int dir_code, const char *filename);
int db_get_file(int file_code, p_file_info p_info);
int db_get_pwd(int dir_code, char *buf);
int db_create_dir(const char *username, int dir_code, const char *dir_name);
int db_dir_empty(const char *username, int dir_code);
int db_remove_dir(const char *username, int dir_code);
int db_remove_file(const char *username, int file_code);
int db_list_info(const char *username, int dir_code, file_info_t **pp_info);


#endif /* __USER_INFO_H__ */
