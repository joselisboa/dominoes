#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>
#include <stdio.h>

#define B_SIZE (PIPE_BUF/2)

const char *DOMINOS = "/tmp/DOMINOS";

struct request {
    char fifo[B_SIZE];//FIFO name
    int pid;
    char cmd[B_SIZE];//command line
};

