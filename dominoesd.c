#include "game.h"
#include "public.h"
#include "server.c"

//-----------------------------------------------------------------------------
// DOMINOES daemon
int main(int argc, char *charv[]){
    int spid, aux_fifo, tile_id, i, k, n, A_SIGS[A];
    struct request req;
    struct response res;
    char action[8], side[8];

    // run in the background
    if(fork() > 0) return;

    // 2nd process
    if(procs() > 1) {
        spid = getzpid(SERVER);
        if(strcmp("show", charv[1]) == 0) kill(spid, SIGUSR1);
        else if(strcmp("close", charv[1]) == 0) kill(spid, SIGUSR2);
        exit(0);
    }

    // delete old named pipe
    if(!access(DOMINOS, F_OK)) unlink(DOMINOS);

    // signals setup
    signal(SIGUSR1, show);
    signal(SIGUSR2, stop);
    signal(SIGALRM, init);

    // FIFO público
    mkfifo(DOMINOS, 0666);
    
    // keeps fifo open (does not exit)
    while(1){
        // Abrir FIFO do cliente em modo de escrita
        if((server_fifo = open(DOMINOS, O_RDONLY)) < 0) {
            perror("FIFO ERROR");
            exit(1);
        }

        // Listen for requests on the public FIFO
        while(read(server_fifo, &req, sizeof(req)) > 0){
            // parse client request
            k = sscanf(req.cmd, "%s %d %s", action, &tile_id, side);
            
            for(i=0; i<C; i++) 
                if(strcmp(C_CMDS[i], action) == 0) break;
            
            // client commands
            switch(i) {
                case 0:// login 
                    res = login(req);
                    break;
                case 1:// exit
                case 2:// logout
                    res = leaves(req);
                    break;
                case 3:// status 
                    res = status(req);
                    break;
                case 4:// users 
                    res = users(req);
                    break;
                case 5:// new 
                    res = add_game(req);
                    break;
                case 6:// play 
                    res = k==1? play_game(req): play_tile(req);
                    break;
                case 7:// quit 
                    res = leaves(req);
                    break;
                case 11:// games 
                    res = list_games(req);
                    break;
                default:// Players commands
                for(i=0; i<P; i++) if(strcmp(P_CMDS[i], action) == 0) break;
                switch (i) {
                    case 1://info 
                        res = info(req);
                        break;
                    case 0:// tiles
                        res = player_tiles(req);
                        break;
                    case 2:// game 
                        res = game_tiles(req);
                        break;
                    case 4:// get 
                        res = get(req);
                        break;
                    case 5:// pass 
                        res = pass(req);
                        break;
                    case 8://hint
                    case 6://help
                        res = help(req);
                        break;
                    case 7://giveup 
                        res = giveup(req);
                        break;
                    case 9:// players 
                        res = list_players(req);
                        break;
                    default: 
                        res = ni(req);
                        res.cmd = 0;
                }
            }

            // send response through client's private FIFO
            if(!send(res, req)) {
                perror("Did not access the client fifo\n");
                break;
            }
        }
    }

    close(server_fifo);
    return cleanup();
}
