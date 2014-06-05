#include "game.h"
#include "public.h"
#define A 2

//stock, tiles, start, new_game,
//add_player, get_player_by_id, get_tile_by_id, count_tiles
//add_tile, remove_tile, give_tile_by_id, place_tile,
//get_ends (mosaic_ends), validate_tile, tile_exists (has_tile),
//player_status, get_winner

void init(int sig);
void stop(int sig);
void show(int sig);
int cleanup();
int getspid();
int procs();
int count_players();

int send(struct response res, struct request req);
struct response resdef(int cmd, char msg[], struct request req);

struct response leave(struct request req);// same as leave?
struct response logout(struct request req);
struct response status(struct request req);
struct response users(struct request req);
struct response add_game(struct request req);
struct response list_games(struct request req);
struct response play_tile(struct request req);
struct response leaves(struct request req);
struct response login(struct request req);
struct response info(struct request req);
struct response player_tiles(struct request req);
struct response info(struct request req);
struct response game_tiles(struct request req);
struct response play_game(struct request req);
struct response get(struct request req);
struct response pass(struct request req);
struct response help(struct request req);
struct response giveup(struct request req);

// not implemented
struct response ni(struct request req);

struct game *games = NULL;
struct player *players = NULL;

int client_fifo, server_fifo;
//-----------------------------------------------------------------------------
// USERS
struct response list_players(struct request req){
    struct response res = resdef(1, "OK players", req);
    struct player *node = games->players;
    char msg[512];
    char line[64];

    msg[0] = '\0';
    line[0] ='\0';

    while(node != NULL){
        sprintf(line, "%d %s %d %s (%d)\n",
            node->id, node->name, node->pid, node->fifo, node->wins);
        strcat(msg, line);       
        node = node->prev;
    }
    strcpy(res.msg, msg);
    return res;
}

//-----------------------------------------------------------------------------
// M A I N
//-----------------------------------------------------------------------------
int main(int argc, char *charv[]){
    int spid, aux_fifo, tile_id;
    int i, k, n, A_SIGS[A];
    struct request req;
    struct response res;
    char action[8], side[8];

    // administrator commands
    char *A_CMDS[A] = {
        "show",
        "close"
    };

    // administrator signals
    A_SIGS[0] = SIGUSR1;//show
    A_SIGS[1] = SIGUSR2;//close

    // run in the background
    if(fork() > 0) return;

    // 2nd process
    if(procs() > 1) {
        spid = getspid();
        for(i=0; i<A; i++)
            if(strcmp(A_CMDS[i], charv[1]) == 0) kill(spid, A_SIGS[i]);
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

    // fifo cicle
    while(1) {
        // [3] Abrir FIFO do cliente em modo de escrita
        if((server_fifo = open(DOMINOS, O_RDONLY)) < 0) {
            perror("BROKE UP");
            break;
        }

        // [2] Ler pedido pelo FIFO público
        while(read(server_fifo, &req, sizeof(req)) > 0){

            // [4] Satisfazer pedido do cliente
            k = sscanf(req.cmd, "%s %d %s", action, &tile_id, side);

            // Client commands
            for(i=0; i<C; i++)
                if(strcmp(C_CMDS[i], action) == 0)
                    break;

            // command router
            switch(i) {

                // login
                case 0:
                    res = login(req);
                    break;

                // exit
                case 1:
                    res = leaves(req);

                    break;

                case 2://logout
                    res = logout(req);
                    break;

                case 3://status
                    res = status(req);
                    break;

                case 4://users
                    res = users(req);
                    break;

                case 5://new
                    res = add_game(req);
                    break;

                case 6://play
                    res = play_game(req);
                    break;

                case 7://quit
                    res = leaves(req);
                    break;

                case 11://games
                    res = list_games(req);
                    break;

                default:

                // Players commands
                for(i=0; i<P; i++)
                    if(strcmp(P_CMDS[i], action) == 0)
                        break;
                
                switch (i) {
                    case 1://info
                        res = info(req);
                        break;

                    case 0://tiles
                        res = player_tiles(req);
                        break;

                    case 2://game
                        res = game_tiles(req);
                        break;

                    case 3://play
                        res = play_tile(req);
                        break;

                    case 4://get
                        res = get(req);
                        break;

                    case 5://pass
                        res = pass(req);
                        break;

                    case 6://help
                        res = help(req);
                        break;

                    //giveup
                    case 7:
                        res = giveup(req);
                        break;
                    
                    // players
                    case 8:
                        res = list_players(req);
                        break;

                    default:
                        // not implemented
                        res = ni(req);//continue;
                }
            }

            //[5] Enviar dados pelo FIFO do cliente
            if(!send(res, req)) {
                perror("Did not access the client fifo\n");
                exit(1);
            }
        }

        close(server_fifo);
    }

    return cleanup();
}

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------
// CLOSE / STOP  (kill -s USR2 <pid>)
void stop(int sig){
    struct player *node = players;

    while(node != NULL) {
        kill((*node).pid, SIGUSR2);
        node = node->prev;
    }
    //sleep(5);
    cleanup();
}

void show(int sig){
    printf("\nSHOW NOT IMPLEMENTED YET\n$ ");
}

//-----------------------------------------------------------------------------
// LOGOUT
struct response logout(struct request req){
    struct response res = resdef(1, "OK bye", req);
    //struct player *aux = players;

    //TODO restore unused tiles
    //TODO remove player from players
    //TODO remove player from active game (go to next player)

    return res;
}

//-----------------------------------------------------------------------------
// STATUS
struct response status(struct request req){
    return ni(req);
}

//-----------------------------------------------------------------------------
// USERS
struct response users(struct request req){
    struct response res = resdef(1, "OK users", req);
    struct player *node = players;
    char msg[512];
    char line[64];

    msg[0] = '\0';
    line[0] ='\0';

    while(node != NULL){
        sprintf(line, "%d %s %d %s (%d)\n",
            node->id, node->name, node->pid, node->fifo, node->wins);
        strcat(msg, line);       
        node = node->prev;
    }
    strcpy(res.msg, msg);
    return res;
}

//-----------------------------------------------------------------------------
// NEW (Add Game)
struct response add_game(struct request req){
    struct response res = resdef(1, "OK new", req);
    char name[32];
    int t, pid;
    
    if(games != NULL && !games->done) {
        strcpy(res.msg, "there's a live game");
        res.cmd = 0;
        return res;
    }

    sscanf(req.cmd, "new %s %d", name, &t);
    games = new_game(games);
    strcpy(games->name, name);
    games->t = t;

    if(fork() == 0){
        sleep(t);
        kill(getppid(), SIGALRM);
        exit(0);
    }

    return res;
}

//-----------------------------------------------------------------------------
// GAMES (List)
struct response list_games(struct request req){
    struct response res = resdef(1, "OK games", req);
    struct game *aux = games;
    char msg[512];
    int i, j=0, l;

    while(aux != NULL){
        l = strlen(aux->name);
        for(i=0; i<l; i++){
            msg[j++] = aux->name[i];
            if(j==511) break;
        }
        if(j==511) break;

        //TODO active game, winner

        aux = aux->prev;

        if(aux != NULL) msg[j++] = '\n';
    }
    msg[j] = '\0';
    strcpy(res.msg, msg);

    return res;
}

//-----------------------------------------------------------------------------
// PLAY (adds player to game)
struct response play_game(struct request req){
    struct response res = resdef(1, "OK play", req);
    struct player node, *user = NULL;
    char msg[512];

    if(games == NULL){
        strcpy(res.msg, "create a game first");
        res.cmd = 2;
        return res;
    }


// check if player already exists


    if(!games->done){
        user = get_player_by_id(req.player_id, players);
        
        node.id = user->id;
        
        node.pid = user->pid;
        
        node.login_t = user->login_t;
        
        strcpy(node.name, user->name);
        strcpy(node.fifo, user->fifo);
        
        add_player(node, games);
    }
    else {
        sprintf(msg,"game '%s' expired", games->name);
        strcpy(res.msg, msg);
        res.cmd = 2;
    }

    return res;
}

//-----------------------------------------------------------------------------
// EXIT (leaves)
struct response leaves(struct request req){
    struct response res = resdef(1, "OK bye", req);

    // remove player

    // add tiles to stock

    // inform the player has left

    return res;
}

//-----------------------------------------------------------------------------
// LOGIN
struct response login(struct request req){
    struct player *aux_player = NULL;
    struct response res = resdef(1, "OK welcome", req);

    aux_player = get_player_by_name(req.name, players);

    //TODO reject user if other user has the same fifo

    if(aux_player == NULL){
        players = new_player(req.name, players);
        strcpy(players->fifo, req.fifo);
        res.req.player_id = players->id;
        players->pid = req.pid;
    }
    else {
        res.cmd = 0;
        res.req.player_id = 0;
        sprintf(res.msg, "name '%s' already taken", req.name);
    }

    return res;
}

//-----------------------------------------------------------------------------
// INFO
struct response info(struct request req){
    struct response res;
    res.pid = getpid();
    strcpy(res.msg, "Info");
    res.req = req;
    res.cmd = 0;
    return res;
}

struct response player_tiles(struct request req){
     return ni(req);
}
struct response game_tiles(struct request req){
     return ni(req);
}
struct response play_tile(struct request req){
     return ni(req);
}
struct response get(struct request req){
     return ni(req);
}
struct response pass(struct request req){
     return ni(req);
}
struct response help(struct request req){
     return ni(req);
}
struct response giveup(struct request req){
     return ni(req);
}

//-----------------------------------------------------------------------------
// counts players in current game
int count_players(){
    struct player *node = games->players;
    int i=0;
    while(node != NULL){
        i++;
        node = node->prev;
    }
    return i;
}

//-----------------------------------------------------------------------------
// INIT (Start Game) SIGALRM handler
void init(int sig){
    struct player *node = games->players;
    int player_fifo;

    struct status stat;
    int done = 0, n = 0;

    strcpy(stat.name, games->name);
    strcpy(stat.tiles, "[0,0][0,3]");
    stat.winner = 0;
    stat.done = 0;
  
    if(count_players() > 1){
        while(node != NULL){
            kill(node->pid, SIGUSR1);
            sleep(1);

    //.........................................................................
    done = 0;
    n = 0;
    do {// [3] Abrir FIFO do cliente em modo de escrita
        if((player_fifo = open(node->fifo, O_WRONLY | O_NDELAY)) == -1){
          sleep(5);  
        } 
        else {// [5] Enviar resposta pelo FIFO do cliente
            write(player_fifo, &stat, sizeof(stat));
            close(player_fifo);
            done = 1;
        }
    } while(n++ < 5 && !done);
    //.........................................................................
 
            node = node->prev;
        }
    }
    else games->done = 1;
}

//-----------------------------------------------------------------------------
// DEFRES (Default Response)
struct response resdef(int cmd, char msg[], struct request req){
    struct response res;
    //struct player *user = get_player_by_name(req.name, players);

    res.pid = getpid();
    strcpy(res.msg, msg);
    res.req = req;
    //if(user != NULL) res.req.player_id = user->id;
    res.cmd = cmd;

    return res;
}

//-----------------------------------------------------------------------------
// NI (Not Implemented Response)
struct response ni(struct request req){
    struct response res;
    res.pid = getpid();
    strcpy(res.msg, "NOT IMPLEMENTED");
    res.req = req;
    res.cmd = -1;
    return res;
}

//-----------------------------------------------------------------------------
// Get 'server' PID
int getspid(){
    int server_pid, n;
    char buffer[16];
    FILE *finput;

    buffer[0] = '\0';
    finput = popen("pgrep server | head -1", "r");
    if((n = read(fileno(finput), buffer, 15)) > 0) buffer[n] = '\0';
    pclose(finput);
    return atoi(buffer);
}

//-----------------------------------------------------------------------------
// Counts Processes
int procs(){
    int server_pid, n;
    char buffer[32];
    FILE *finput;

    buffer[0] = '\0';
    finput = popen("pgrep server | wc -l", "r");
    if((n = read(fileno(finput), buffer, 30) > 0)) buffer[n] = '\0';
    pclose(finput);
    return atoi(buffer);
}

//-----------------------------------------------------------------------------
// Send
int send(struct response res, struct request req){
    int done = 0, n = 0;

    do {// Abrir FIFO do cliente em modo de escrita
        if((client_fifo = open(req.fifo, O_WRONLY | O_NDELAY)) == -1) sleep(5);
        else {// [5] Enviar resposta pelo FIFO do cliente
            write(client_fifo, &res, sizeof(res));
            close(client_fifo);
            done = 1;
        }
    } while(n++ < 5 && !done);

    return done;
}

int cleanup(){
    unlink(DOMINOS);
    exit(0);
}
