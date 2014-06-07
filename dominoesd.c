#include "game.h"
#include "public.h"
#include "server.c"

//-----------------------------------------------------------------------------
// DOMINOES DAEMON
//-----------------------------------------------------------------------------
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
        sleep(1);
        exit(0);
    }

    // delete old named pipe
    if(!access(DOMINOS, F_OK)) unlink(DOMINOS);

    // signals setup
    signal(SIGUSR1, show);
    signal(SIGUSR2, stop);
    signal(SIGALRM, init);

    // [1] Gerar FIFO público, e aguardar dados do cliente
    mkfifo(DOMINOS, 0666);
    
    // the keeps its fifo open and does not exit
    while(1){

        // [3] Abrir FIFO do cliente em modo de escrita
        if((server_fifo = open(DOMINOS, O_RDONLY)) < 0) {
            perror("BROKE UP");
            exit(1);
        }

        // [2] Ler pedido pelo FIFO público
        while(read(server_fifo, &req, sizeof(req)) > 0){
            // [4] Satisfazer pedido do cliente
            k = sscanf(req.cmd, "%s %d %s", action, &tile_id, side);

            // Client commands
            for(i=0; i<C; i++) if(strcmp(C_CMDS[i], action) == 0) break;
            switch(i) {
                // login
                case 0: res = login(req);
                    break;
                // exit/logout
                case 1:
                case 2: res = leaves(req);
                    break;
                // status
                case 3: res = status(req);
                    break;
                // users
                case 4: res = users(req);
                    break;
                // new
                case 5: res = add_game(req);
                    break;
                // play
                case 6: res = (k == 1)? play_game(req) : play_tile(req);
                    break;
                // quit
                case 7: res = leaves(req);
                    break;
                // games
                case 11: res = list_games(req);
                    break;
                default:
                // Players commands
                for(i=0; i<P; i++) if(strcmp(P_CMDS[i], action) == 0) break;
                switch (i) {
                    
                    //info
                    case 1: res = info(req);
                        break;
                    
                    // tiles
                    case 0:res = player_tiles(req);
                        break;
                    
                    // game
                    case 2: res = game_tiles(req);
                        break;
                    
                    // get
                    case 4: res = get(req);
                        break;
                    
                    // pass
                    case 5: res = pass(req);
                        break;
                    
                    //help/hint
                    case 8:
                    case 6: res = help(req);
                        break;
                    
                    //giveup
                    case 7: res = giveup(req);
                        break;
                    // players
                    case 9: res = list_players(req);
                        break;
                        
                    default: 
                            res = ni(req);
                            res.cmd = 0;
                }
            }

            //[5] Enviar dados pelo FIFO do cliente
            if(!send(res, req)) {
                perror("Did not access the client fifo\n");
                break;
            }
        }
    }

    close(server_fifo);

    return cleanup();
}
