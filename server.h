#define A 2

void init(int sig);
void inform(int sig);
void stop(int sig);
void show(int sig);
int cleanup();
int procs();
int count_players();
int send(struct response res, struct request req);

struct response leave(struct request req);
struct response logout(struct request req);
struct response status(struct request req);
struct response users(struct request req);
struct response add_game(struct request req);
struct response list_games(struct request req);
struct response list_players(struct request req);
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
struct response ni(struct request req);
struct response resdef(int cmd, char msg[], struct request req);

struct game *games = NULL;
struct player *players = NULL;
struct player *playing = NULL;
struct move move;

int client_fifo, server_fifo;// order important!

void games_string(char string[]);
void tiles_string(char string[], struct domino *tiles);
void mosaic_string(char string[]);
//-----------------------------------------------------------------------------
//                                                                     server.h
// TODO split - - - > - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                     server.c
//-----------------------------------------------------------------------------
// CLOSE / STOP  (kill -s USR2 <pid>)
void stop(int sig){
    struct player *node = players;

    while(node != NULL) {
        kill((*node).pid, SIGUSR2);
        node = node->prev;
    }
    cleanup();
}

void show(int sig){
    printf("\nnot implemented yet");
    printf("\npress enter to continue\n");
    fflush(stdout);
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
    char msg[512], line[64];
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
// GAMES (List)
struct response list_games(struct request req){
    struct response res = resdef(1, "OK games", req);
    //struct game *node = games;
    char msg[512];//, line[64], status[16];
    
    if(games == NULL){
        res.cmd = 0;
        strcpy(res.msg, "there aren't any games");
        return res;
    }

    games_string(msg);
    strcpy(res.msg, msg);

    return res;
}
// games stringify games
void games_string(char string[]){
    struct game *node = games;
    char line[64], status[16];

    line[0] = string[0] = '\0';
    
    while(node != NULL){
        if(!node->start_t) {
            if(!node->done) strcpy(status, "waiting");
            else strcpy(status, "expired");
        }
        else {
            if(!node->done) strcpy(status, "playing");
            else if(node->winner == NULL) strcpy(status, "none");
            else sprintf(status, "winner: %s", node->winner->name);
        }
       
        sprintf(line, "%d %s (%s)", node->id, node->name, status);
        strcat(string, line);       
        node = node->prev;
        if(node != NULL) strcat(string, "\n");
    }
}


//-----------------------------------------------------------------------------
// PLAYERS lists players in live game
struct response list_players(struct request req){
    struct response res = resdef(1, "OK listing players", req);
    struct player *node = NULL;
    char msg[512];
    char line[64];
    
    if(games == NULL){
        res.cmd = 0;
        strcpy(res.msg, "there aren't any players");
        return res;
    }
    
    node = games->players;
    
    msg[0] = '\0';
    line[0] ='\0';
    
    while(node != NULL){
        sprintf(line, "%d %s %d %s (%d)",
            node->id, node->name, node->pid, node->fifo, node->wins);
        strcat(msg, line);       
        node = node->prev;
        if(node != NULL) strcat(res.msg, "\n");
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
    name[0] = '\0';

    if(games != NULL && !games->done) {
        strcpy(res.msg, "there's a live game");
        res.cmd = 0;
        return res;
    }

    sscanf(req.cmd, "new %s %d", name, &t);
    sprintf(res.msg, "game '%s' created", name);

    games = new_game(games);
    strcpy(games->name, name);
    games->t = t;

    signal(SIGALRM, init);

    if(fork() == 0){
        sleep(t);
        kill(getppid(), SIGALRM);
        exit(0);
    }

    return res;
}

//-----------------------------------------------------------------------------
// PLAY (adds player to game)
struct response play_game(struct request req){
    struct response res = resdef(1, "OK play", req);
    struct player node, *user = NULL, *player = NULL;
    char msg[512];
    int i, n;

    // there are no games
    if(games == NULL){
        strcpy(res.msg, "create a game first");
        res.cmd = 0;
        return res;
    }
    
    n = count_players();

    // player already subscribed?
    player = get_player_by_name(req.name, games->players);
    if(player != NULL){
        if(games->start_t) strcpy(res.msg, "you are already playing");
        else sprintf(res.msg, "you already subscribed to '%s'", games->name);
        res.cmd = 0;
        return res;
    }
    // available game
    else if(!games->done){
        sprintf(res.msg, "subscribed to '%s'", games->name);
        if(games->start_t == 0) {
            user = get_player_by_id(req.player_id, players);
            node.id = user->id;
            node.pid = user->pid;
            node.login_t = user->login_t;
            strcpy(node.name, user->name);
            strcpy(node.fifo, user->fifo);
            add_player(node, games);
        }
        else {
            sprintf(msg,"game '%s' already started", games->name);
            strcpy(res.msg, msg);
            res.cmd = 0;
        }
    }
    else {
        sprintf(msg,"'%s' expired", games->name);
        strcpy(res.msg, msg);
        res.cmd = 0;
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
    struct response res = resdef(1, "OK tiles", req);
    struct player *node = games->players;
    char string[128];
    string[0] = '\0';

    sprintf(res.msg, "Game: %s\n", games->name);
    sprintf(string, "Stock: %d tiles\n", count_tiles(games->tiles));
    strcat(res.msg, string);
    sprintf(string, "Mosaic: %d tiles\n", count_tiles(games->mosaic));
    strcat(res.msg, string);
    
    while(node != NULL){        
        if(playing == node) sprintf(string, "%s: %d tiles (playing turn)", 
            node->name, count_tiles(node->tiles)); 
        else sprintf(string, "%s: %d tiles", 
            node->name, count_tiles(node->tiles));
        strcat(res.msg, string);
        node = (*node).prev;
        if(node != NULL) strcat(res.msg, "\n");
    }

    return res;
}

//-----------------------------------------------------------------------------
// TILES response with player tiles
struct response player_tiles(struct request req){
    struct response res = resdef(1, "OK tiles", req);
    struct player *player = get_player_by_name(req.name, games->players);
    char hand[128];
    sprintf(res.msg, "%s, your dominoes", player->name);   
    tiles_string(hand, player->tiles);
    strcat(res.msg, hand);
    return res;
}

//-----------------------------------------------------------------------------
// GAME response with mosaic tiles
struct response game_tiles(struct request req){
    struct response res = resdef(1, "OK tiles", req);
    struct game *node = games;
    char hand[128];
    sprintf(res.msg, "%s's mosaic\n", node->name);   
    mosaic_string(hand);
    strcat(res.msg, hand);
    return res;
}

//-----------------------------------------------------------------------------
// PLAY places tile on mosaic
struct response play_tile(struct request req){
    struct response res = resdef(1, req.cmd, req);
    struct player *player = NULL, *next = NULL;
    struct domino *tile = NULL;
    int tile_id, mask[2], n, i, j;
    char pos[8], string[64];

    string[0] = '\0';
    sscanf(req.cmd, "play %d %s", &tile_id, pos);

    player = get_player_by_name(req.name, games->players);
    
    //1. player == NULL => player is not playing the game
    if(player != playing){
        strcpy(res.msg, "its not your turn");
        res.cmd = 0;
        return res;
    }

    tile = get_tile_by_id(tile_id, player->tiles);
    //2. tile == NULL => player does not have that tile
    if(tile == NULL){
        sprintf(res.msg, "you don't have tile %d", tile_id);
        res.cmd = 0;
        return res;
    }
    
    get_ends(mask, games->mosaic);
    //3. mosaic is empty
    if(count_tiles(games->mosaic) == 0 || validate_tile(mask, tile)){
        move.move++;        
        // remove a peça ao jogador
        tile = remove_tile(tile_id, player);
        // coloca a peça no mosaico
        place_tile(tile, games);

        // next player
        n = count_players();
        for(i=0; i<n; i++) if(strcmp(playing->name, move.players[i]) == 0){
            j = (i+1 == n) ? 0 : i + 1;
            playing = get_player_by_name(move.players[j], games->players);
            move.turn = j;
            break;
        }

        // move message
        sprintf(move.msg, "\n%s played tile %d:[%d,%d]", 
            player->name, tile->id, tile->mask[0], tile->mask[1]);

        // player's last tile? won
        if(!count_tiles(player->tiles)) {
            player->wins++;

            sprintf(string, "\nit was %s's last tile", player->name);
            strcat(move.msg, string);
            
            sprintf(string, "\n%s is the Winner!", player->name);
            strcat(move.msg, string);

            move.winner = i-1;
            games->done = 1;
        }
        else {
            sprintf(string, "\033[0m\nwaiting for \033[0;32m%s\033[0m to play", playing->name);
            strcat(move.msg, string);
        }
        //TODO inform players  
        //TODO left/right side
        //TODO move to move
        //TODO remove after inform implant
        sprintf(res.msg, "placed tile %d on the %s", tile_id, pos);
    }
    //5. tile does NOT fit mosaic    
    else{
        strcpy(res.msg, "tile does not fit");
        res.cmd = 0;
    }

    if(fork() == 0){
        sleep(2);
        kill(getppid(), SIGALRM);
        exit(0);
    }

    return res;
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
// INIT (start game) SIGALRM handler
void init(int sig){
    struct player *node, *player = games->players;;
    int player_fifo;
    int done, k, i=0, n = count_players();
    char hand[128], tile[16];
    struct domino *tiles;

    if(count_players() > 1){
        signal(SIGALRM, inform);
  
        // start game (distribute dominoes)
        start(games);
        time(&games->start_t);
        
        // menssagem
        strcpy(move.name, games->name);
        move.winner = 0;
        move.move = 1;
        
        player = games->players;
        while(player != NULL){
            strcpy(move.players[i++], player->name);
            if(player->prev == NULL) playing = player;
            player = player->prev;
        }
        for(i; i<4; i++) move.players[i][0] = '\0';

        // enviar dados aos jogadores do jogo
        player = games->players;
        while(player != NULL){
            kill(player->pid, SIGUSR1);
            done = 0;
            k = 0;
            do {// abrir FIFO do jogador em modo de escrita
                if((player_fifo = open(player->fifo, O_WRONLY | O_NDELAY)) == -1){
                  sleep(5);
                }
                else {
                    move.msg[0] = '\0';
                    tile[0] = '\0';

                    strcpy(move.msg, "Starting ");
                    strcat(move.msg, games->name);                    
                    
                    sprintf(hand, "\n\033[0;32m%s, your dominoes", player->name);
                    strcat(move.msg, hand);

                    hand[0] = '\0';
                    tiles_string(hand, player->tiles);
                    strcat(move.msg, hand);  

                    move.turn = 1;
                    sprintf(hand, "\033[0m\nwaiting for \033[0;32m%s\033[0m to play", playing->name);
                    strcat(move.msg, hand);  
                    // enviar resposta pelo FIFO do jogador
                    write(player_fifo, &move, sizeof(move));
                    close(player_fifo);
                    sleep(1);
                    done = 1;
                }
            } while(k++ < 5 && !done);
            
            player = player->prev;
        }
    }
    else games->done = 1;
}

//-----------------------------------------------------------------------------
// DEFRES (Default Response)
struct response resdef(int cmd, char msg[], struct request req){
    struct response res;
    res.pid = getpid();
    strcpy(res.msg, msg);
    res.req = req;
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
// Counts Processes
int procs(){
    int server_pid, n;
    char buffer[32];
    FILE *finput;
    buffer[0] = '\0';
    finput = popen("pgrep dominoesd | wc -l", "r");
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

    res.msg[0] = '\0';

    return done;
}

int cleanup(){
    unlink(DOMINOS);
    exit(0);
}

//-----------------------------------------------------------------------------
// Stringify tiles
void tiles_string(char string[], struct domino *tiles){
    char tile[16];
    string[0] = '\0';
    tile[0] = '\0';

    if(tiles == NULL) strcpy(string, "[]");
    else while(tiles != NULL){
        sprintf(tile, "\n%2d:[%d,%d]", tiles->id, tiles->mask[0], tiles->mask[1]);
        strcat(string, tile);
        tiles = tiles->next;
    }
}

//-----------------------------------------------------------------------------
// stringify mosaic tiles
void mosaic_string(char string[]){
    struct domino *node = games->mosaic;
    char tile[16];
    string[0] = '\0';
    tile[0] = '\0';

    if(node == NULL) strcpy(string, "[]");
    else while(node != NULL){
        sprintf(tile, "[%d,%d]", node->mask[0], node->mask[1]);
        strcat(string, tile);
        node = node->next;
    }
}

//-----------------------------------------------------------------------------
// informs players of latest move 
void inform(int sig){
    struct player *node, *player = games->players;
    int player_fifo;
    int done, k, i=0, n = count_players();

    // enviar dados aos jogadores do jogo
    player = games->players;
    while(player != NULL){
        kill(player->pid, SIGUSR1);
        done = 0;
        k = 0;
        do {// abrir FIFO do jogador em modo de escrita
            if((player_fifo = open(player->fifo, O_WRONLY | O_NDELAY)) == -1){
              sleep(5);
            }
            else {
                write(player_fifo, &move, sizeof(move));
                close(player_fifo);
                sleep(1);
                done = 1;
            }
        } while(k++ < 5 && !done);
        
        player = player->prev;
    }

    return;
}