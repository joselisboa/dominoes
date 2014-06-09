#include "game.h"
#include "public.h"
#include "server.c"

//-----------------------------------------------------------------------------
// DOMINOES daemon
int main(int argc, char *charv[]){
    char actions[A][5] = {"show", "close"};   
    int i, sigs[A] = {SIGUSR1, SIGUSR2};
    struct request req;
    struct response res;

    // run in the background
    if(fork() > 0) return;

    // 2nd process
    if(procs() > 1) {
        for(i=0; i<A; i++)
            if(!strcmp(actions[i], charv[1])) {
                kill(getzpid(SERVER), sigs[i]);
                break;
            }

        exit(0);
    }

    // delete old named pipe
    if(!access(DOMINOS, F_OK)) unlink(DOMINOS);

    // signal handlers setup
    signal(SIGUSR1, show);
    signal(SIGUSR2, stop);
    signal(SIGALRM, init);

    // create public FIFO (server)
    if(mkfifo(DOMINOS, 0666) < 0) exit(1);
    
    // keep public fifo open
    while(TRUE){

        // open public FIFO in read only mode
        if((server_fifo = open(DOMINOS, O_RDONLY)) < 0) {
            perror("FIFO ERROR");
            exit(1);
        }

        // Listen for requests on the public FIFO
        while(read(server_fifo, &req, sizeof(req)) > 0){
            res = router(req, res);

            // send response through client's FIFO
            if(!send(res, req)) {
                perror("Did not access the client fifo\n");
                break;
            }
        }
    }

    close(server_fifo);

    return cleanup();
}
