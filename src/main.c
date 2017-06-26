#include "connection.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[]) {
    if(argc != 2 && argc != 3) {
        printf("Usage: %s <ftp server ip> [port]\n",argv[0]);
        exit(1);
    }

    else {
        if(argv[2]==NULL)
            start_ftp_cmd(argv[1], DEFAULT_FTP_PORT);
        else
            start_ftp_cmd(argv[1], atol(argv[2]));
    }

    return 1;
}
