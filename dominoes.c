#include <stdlib.h>
#include "public.h"
#include "client.c"

// DOMINOES Lab Work for SO (Operating Systems)
int main(int argc, char *argv[]){
    char c, buffer[256];
    int n = 0;

    // set signal handlers
    signal(SIGUSR1, play);  
    signal(SIGUSR2, quit);

    // server status
    status(argc, argv);

    // create the private fifo (client)
    sprintf(req.fifo, "%s_%d", FIFOPATH, getpid());

    if(mkfifo(req.fifo, 0666) < 0) {
        exit(1);
    }

    // open the public fifo (server) in read only mode
    if((server_fifo = open(DOMINOS, O_WRONLY)) < 0) {
        exit(unlink(req.fifo));
    }
    
    LOGIN:
    
    // authenticate or exit
    if(!auth(req.name)) {
        exit(cleanup("bye"));
    }

    // send login request to server
    send();
    
    // login was rejected
    if(!res.cmd) {
        puts(chameleon(res.msg, 4));
        goto LOGIN;
    }
  
    // update request form
    req = res.req;
    
    sprintf(buffer, "welcome, %s", req.name);
    strcpy(buffer, chameleon(buffer, 3));

    // request loop
    while(TRUE){     
        // output to user
        if(buffer[0] != '\0'){
            write(2, buffer, strlen(buffer));
            buffer[0] = '\0';
        }

        write(2, "\n> ", 3);

        // input from user
         while(read(0, &c, 1) >  0){
            if(c != '\n' && n < 63){
                req.cmd[n++] = c;
            }
            else break;
        }
        
        req.cmd[n] = '\0';
        n = 0;

        // validate request or prompt again
        if(!validate(req.cmd, buffer)){
            continue;
        }

        // send request
        send();

        // copy the response message to the buffer
        strcpy(buffer, chameleon(res.msg, res.cmd?2:4));

        // LOGOUT
        if(!strcmp(req.cmd, "logout")){
            goto LOGIN;
        }

        // EXIT
        if(!strncmp("exit", req.cmd, 3)){
            break;
        }
    }
    
    exit(cleanup("bye"));
}
