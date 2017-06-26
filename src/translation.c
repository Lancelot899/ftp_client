#include "translation.h"
#include "connection.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "dir_list.h"
#include <errno.h>

int make_remote_dir(char* remote_dir, int sockfd) {
    int ret = 0;
    char cmd[40];
    sprintf(cmd, "MKD %s\r\n", remote_dir);
    ret = send_command(cmd, NULL, sockfd);
    if (ret == 257)
        return (0);

    return -1;
}

int upload_dir(char* local_dir) {
    int i = 0;
    dir_list* p_dir_list = create_dir_list(1024, 1);
    add_dir_list(p_dir_list, "", local_dir);
    get_dir_list(p_dir_list, local_dir);

    for (i = 0; i < p_dir_list->count; i++) {
        if (p_dir_list->list[i]->is_dir)
            make_remote_dir(p_dir_list->list[i]->dir_name, sock_control);
    }

    for (i = 0; i < p_dir_list->count; i++) {
        if (!p_dir_list->list[i]->is_dir)
            upload(p_dir_list->list[i]->dir_name, p_dir_list->list[i]->dir_name);
    }
    clean_dir_list(p_dir_list);
    return 0;
}

int send_command(char* cmd, char* buf, int sockfd) {
    int read_size = 0;
    int write_size = 0;
    int need_free_buf = 0;
    int ret_value = -1;
    if (sockfd == -1)
    {
        return (-1);
    }
    if (!buf) {
        buf = (char*)malloc(sizeof(char) * BUFSIZE);
        need_free_buf = 1;
    }

    if (cmd)
        write_size = write(sockfd, cmd, strlen(cmd));

    read_size = read(sockfd, buf, sizeof(char) * BUFSIZE);

    if (read_size > 0) {
        buf[read_size] = '\0';
        printf("%s", buf);
        ret_value = atoi(buf);
    }

    if (need_free_buf)	free(buf);
    return (ret_value);
}

int create_pasv_data_sock() {
    int ret_value = 0;
    char* buf;
    char* pos_start;
    char* pos_end;
    char* pos_cur;
    char* part[6];
    char dest_ip[20];
    int data_port;
    int i = 0;
    int sockfd = -1;
    struct sockaddr_in server_addr;
    struct hostent* host_ent;
    buf = (char*)malloc(sizeof(char) * BUFSIZE);
    if ((ret_value = send_command("PASV\r\n", buf, sock_control)) != 227) {
        fprintf(stderr, "connection is closed by remote server\n");
        exit(1);
    }

    pos_start = strchr(buf, '(');
    pos_end = strchr(buf, ')');
    *pos_end = '\0';

    for (i = 0; i < 6; i++)
        part[i] = (char*)malloc(sizeof(char) * 4);

    for (i = 0; i < 6; i++) {
        pos_start++;
        pos_cur = strchr(pos_start, ',');
        if (pos_cur)	*pos_cur = '\0';
        strcpy(part[i], pos_start);
        pos_start = pos_cur;
    }

    free(buf);
    sprintf(dest_ip, "%s.%s.%s.%s", part[0], part[1], part[2], part[3]);
    data_port = atoi(part[4]) * 256 + atoi(part[5]);
    printf("connect to %s:%d\n", dest_ip, data_port);
    if ((host_ent = gethostbyname(dest_ip)) == NULL) {
        perror("gethostbyname");
        return (-1);
    }

    for (i = 0; i < 6; i++)
        free(part[i]);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return (-1);
    }

    bzero(&(server_addr), sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data_port);
    server_addr.sin_addr = *((struct in_addr*)host_ent->h_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        return (-1);
    }

    return sockfd;
}

int create_port_data_sock() {
    return xconnect_ftpdata();
}

int create_data_sock() {
    if (mode)
        return create_pasv_data_sock();
    else
        return create_port_data_sock();
}

int get_port_data_connection(int sockfd) {
    int i = 0;
    int new_sock = -1;
    int set = 0;
    struct sockaddr_in local_host;
    memset(&local_host, 0, sizeof(struct sockaddr_in));
    for(i = 0; i < 3; i++) {
        new_sock = accept(sockfd, (struct sockaddr *)&local_host, (socklen_t *)&set);
        if(new_sock == -1) {
            printf("accept return:%s errno: %d\n", strerror(errno),errno);
            continue;
        }
        else break;
    }
    close(sockfd);
    if(new_sock == -1)
        printf("Sorry, you can't use PORT mode. There is something wrong when the server connect to you.\n");

    return new_sock;
}

int set_bin_mode(int mode, int sockfd) {
    int ret_value = 0;
    char buf[BUFSIZE];
    if (mode)
        ret_value = send_command("TYPE I\r\n", buf, sockfd);

    else
        ret_value = send_command("TYPE A\r\n", buf, sockfd);

    if (ret_value != 200) {
        printf(buf);
        printf("\n");
        return (-1);
    }

    return (0);
}

int file_copy(int srcfd, int destfd, int* psize) {
    char* buf;
    char* ptr;
    int read_size = 0;
    int write_size = 0;
    *psize = 0;
    if (-1 == srcfd || -1 == destfd) {
        fprintf(stderr, "fd error");
        return (-1);
    }

    buf = (char*)malloc(sizeof(char) * BUFSIZE);
    memset(buf, 0, sizeof(char) * BUFSIZE);
    while (0 != (read_size = read(srcfd, buf, sizeof(char) * BUFSIZE))) {
        if (read_size == -1 && errno != EINTR)
            break;

        else if (read_size > 0) {
            ptr = buf;
            while (0 != (write_size = write(destfd, ptr, read_size))) {
                *psize += write_size;
                if (write_size == -1 && errno != EINTR)
                    break;

                else if (write_size == read_size)
                    break;
                else if (write_size > 0) {
                    ptr += write_size;
                    read_size -= write_size;
                }
            }
            if (write_size == -1)
                break;
        }
    }

    free(buf);
    return (0);

}

long get_remote_file_size(char* remote_file, int sockfd) {
    char buf[BUFSIZE];
    char cmd[40];
    char* pos;
    int ret_value = 0;
    int file_size = 0;
    sprintf(cmd, "SIZE %s\r\n", remote_file);
    ret_value = send_command(cmd, buf, sockfd);
    if (ret_value != 213)
        return (-1);

    pos = buf + 4;
    file_size = atoi(pos);
    printf("get_remote_file_size: %d\n", file_size);
    return ((long)file_size);
}


int download(char* remote_file, char* local_file) {
    int filefd = -1;
    int sockfd = -1;
    int ret_value = 0;
    int get_size = 0;
    char cmd[40];
    char rest_cmd[40];
    struct stat* buf;
    long file_size = 0;
    long remote_file_size = 0;
    char usr_input;

    if (0 == access(local_file, F_OK)) {
        buf = (struct stat*)malloc(sizeof(struct stat));
        stat(local_file, buf);
        if (S_ISDIR(buf->st_mode)) {
            printf("%s is existed and it is directory\n", local_file);
            free(buf);
            return (-1);
        }

        file_size = (long)buf->st_size;
        free(buf);
        remote_file_size = get_remote_file_size(remote_file, sock_control);
        if (file_size < remote_file_size)
            printf("%s with %ld bytes is existed, abort(a)/delete(d)/resume(r)? enter for d:", local_file, file_size);
        else
            printf("%s with %ld bytes is existed, abort(a)/delete(d)? enter for d:", local_file, file_size);

        usr_input = (char)getchar();
        if (usr_input == 'a')	return (0);
        else if (usr_input == 'r' && file_size < remote_file_size) {
            sprintf(rest_cmd, "REST %ld\r\n", file_size);
            ret_value = send_command(rest_cmd, NULL, sock_control);
            if (ret_value == 350)
                filefd = open(local_file, O_APPEND | O_WRONLY);

            else
                filefd = open(local_file, O_TRUNC | O_WRONLY);

        }

        else
            filefd = open(local_file, O_TRUNC | O_WRONLY);

    }

    else
        filefd = open(local_file, O_CREAT | O_WRONLY, S_IRUSR|S_IWUSR | S_IRGRP | S_IROTH);

    if (-1 == filefd) {
        perror("open");
        return (-1);
    }

    if (-1 == (sockfd = create_data_sock()))
        return (-1);

    set_bin_mode(1, sock_control);

    sprintf(cmd, "RETR %s\r\n", remote_file);
    ret_value = send_command(cmd, NULL, sock_control);
    if(!(ret_value == 150 || ret_value == 125)) {
        fprintf(stderr, "\n");
        return (-1);
    }

    if (!mode && (-1 ==(sockfd = get_port_data_connection(sockfd)))) {
        close(filefd);
        return (-1);
    }

    file_copy(sockfd, filefd, &get_size);
    close(sockfd);
    close(filefd);
    send_command(NULL, NULL, sock_control);
    printf("received with %d bytes\n", get_size);
    return (0);
}

int upload(char* local_file, char* remote_file) {
    int filefd = -1;
    int sockfd = -1;
    int ret_value = 0;
    int send_size = 0;
    long file_size = 0;
    long remote_file_size = 0;
    char cmd[40];
    char rest_cmd[40];
    char usr_input;
    struct stat* buf;

    if (-1 == (filefd = open(local_file, O_RDONLY))) {
        perror("open");
        return (-1);
    }

    remote_file_size = get_remote_file_size(remote_file, sock_control);

    if (remote_file_size > 0) {
        buf = (struct stat*)malloc(sizeof(struct stat));
        stat(local_file, buf);
        file_size = (long)buf->st_size;
        free(buf);
        if (file_size > remote_file_size)
            printf("%s with %ld bytes is existed, abort(a)/delete(d)/resume(r)? enter for d:", remote_file, remote_file_size);

        else
            printf("%s with %ld bytes is existed, abort(a)/delete(d)? enter for d:", remote_file, remote_file_size);

        usr_input = (char)getchar();
        if (usr_input == 'a')	return (0);
        else if (usr_input == 'r' && file_size > remote_file_size) {
            sprintf(rest_cmd, "REST %ld\r\n", remote_file_size);
            ret_value = send_command(rest_cmd, NULL, sock_control);
            if (ret_value == 350)
                lseek(filefd, remote_file_size, SEEK_SET);
        }
    }

    if (-1 == (sockfd = create_data_sock()))
        return (-1);
    set_bin_mode(1, sock_control);
    sprintf(cmd, "STOR %s\r\n", remote_file);
    ret_value = send_command(cmd, NULL, sock_control);

    if(!(ret_value == 150 || ret_value == 125)) {
        fprintf(stderr, "\n");
        return (-1);
    }

    if (!mode && (-1 ==(sockfd = get_port_data_connection(sockfd)))) {
        close(filefd);
        return (-1);
    }

    file_copy(filefd, sockfd, &send_size);
    close(sockfd);
    close(filefd);
    send_command(NULL, NULL, sock_control);
    printf("sent with %d bytes\n", send_size);
    return (0);
}





