#ifndef TRANSLATION_H
#define TRANSLATION_H


//! for file
int send_command(char* cmd, char* buf, int sockfd);
int create_data_sock();
int set_bin_mode(int mode, int sockfd);
int file_copy(int srcfd, int destfd, int* psize);
long get_remote_file_size(char* remote_file, int sockfd);
int download(char* remote_file, char* local_file);
int upload(char* local_file, char* remote_file);

//! for dir
int make_remote_dir(char* remote_dir, int sockfd);
int upload_dir(char* local_dir);

#endif // TRANSLATION_H
