#include <stdlib.h>
#include "public.h"
#include "zelib.h"
#include "client.c"

//-----------------------------------------------------------------------------
// DOMINOES Lab Work for SO (Operating Systems)
int main(int argc, char *argv[]){
    char buffer[256];

    // set signal handlers
    signal(SIGUSR1, play);  
    signal(SIGUSR2, quit);

    // server status
    status(argc, argv);

    // create the private fifo (client)
    sprintf(req.fifo, "%s_%d", FIFOPATH, getpid());
    if(mkfifo(req.fifo, 0666) < 0){
        perror(req.fifo);
        exit(1);
    }

    // open the public fifo (server) in read only mode
    if((server_fifo = open(DOMINOS, O_WRONLY)) < 0){
        unlink(req.fifo);
        perror(DOMINOS);
        exit(1);
    }
    
    // assume-se que o servidor está a correr...
    LOGIN:
    clear();

    // authenticate or exit
    if(!auth(req.name)) return cleanup();

    // reset login data
    strcpy(req.cmd, "login");
    req.pid = getpid();
    req.player_id = 0;
    res.msg[0] = '\0';
    res.cmd = 0;
    
    // send login request
    res = send(req);
    
    // login rejected
    if(!res.cmd){
        puts(chameleon(res.msg, 4));
        goto LOGIN;
    }

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

        // input from user (request)
        scanf(" %[^\n]", req.cmd);

        // validate request or prompt again
        if(!validate(req.cmd, buffer)) continue;

        // send request
        res = send(req);

        // copy the response message to the buffer
        strcpy(buffer, chameleon(res.msg, res.cmd?2:4));

        // LOGOUT
        if(!strcmp(req.cmd, "logout")) goto LOGIN;

        // EXIT
        if(!strncmp("exit", req.cmd, 3)) break;
    }
    
    return cleanup();
}
