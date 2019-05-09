#ifndef __USER_INFO_H__
#define __USER_INFO_H__

typedef struct{
    char username[128];
    char salt[16];
    char passwd[128];
}user_info_t, *p_user_info;

int get_user_info(p_user_info p_info, const char *username);


#endif /* __USER_INFO_H__ */
