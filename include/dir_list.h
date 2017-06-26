#ifndef _DIR_LIST_H
#define _DIR_LIST_H


typedef struct _dir_node {
    char* dir_name;
    int is_dir;
} dir_node;

typedef struct _dir_list {
    dir_node** list;
    int count;
    int is_local;
} dir_list;


dir_list* create_dir_list(int length, int dir_is_local);
dir_node* create_dir_node(char* up_level_name, char* dir_name);
int add_dir_list(dir_list* p_dir_list, char* up_level_name, char* dir_name);
int get_last_node_mode(dir_list* p_dir_list);
char* get_last_node_name(dir_list* p_dir_list);
int clean_dir_list(dir_list* p_dir_list);
int local_is_dir(char* dirname);
int get_dir_list(dir_list* p_dir_list, char* cur_dir);

#endif
