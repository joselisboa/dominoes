#define A 2

void init(int sig);
void inform(int sig);
void stop(int sig);
void show(int sig);
int cleanup();
int procs();
int count_players();
int send(struct response res, struct request req);

struct response leaves(struct request req);
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

int client_fifo, server_fifo;// important!

void players_string(char string[]);
void games_string(char string[]);
void tiles_string(char string[], struct domino *tiles);
void mosaic_string(char string[]);
//-----------------------------------------------------------------------------
//                                                                     server.h
// TODO split - - - > - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                     server.c
//-----------------------------------------------------------------------------
// get next playing player
struct player *playing_next(){
    if(playing == NULL || playing->prev == NULL) return games->players;
    else return playing->prev;
}
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
//-----------------------------------------------------------------------------
// SHOW 
void show(int sig){
    struct game *game = games;
    struct player *player = NULL;
    char msg[256];
    int n;

    printf("\nGAMES:");
    if(game == NULL) puts(" (no games)");
    else while(game != NULL){
        printf("\n%d %s", game->id, game->name);
        if(!game->start_t && !game->done) printf(" (waiting)");
        
        player = game->players;
        n = count_players();
        if(players == NULL) puts("\n    (no players)");
        else while(player != NULL){
            printf("\n    %s", player->name);
            if(player == game->winner) puts(" (winner)");
            player = player->prev;
            if(player == NULL) putchar('\n');
        }

        game = game->prev;
    }
    
    // users
    printf("\nUSERS:\n");
    players_string(msg);
    puts(msg);

    printf("press enter to continue\n");
    fflush(stdout);
}
//-----------------------------------------------------------------------------
// STATUS
struct response status(struct request req){
    struct response res = resdef(1, "OK status", req);
    struct game *game = games;
    struct player *player = NULL;
    char msg[256];
    int n;
    msg[0]='\0';

    strcpy(res.msg, "GAME STATUS:");
    
    if(game == NULL) strcat(res.msg, " (no games)");
    else while(game != NULL){
        sprintf(msg, "\n%d %s", game->id, game->name);
        strcat(res.msg, msg);
        if(!game->start_t && !game->done) strcat(res.msg, " (waiting)");
        else if(!game->done) strcat(res.msg, " (playing)");
        else if(game->start_t)  strcat(res.msg, " (done)");
        else strcat(res.msg, " (expired)");
        
        player = game->players;
        n = count_players();
        if(player == NULL) strcat(res.msg, "\n    (no players)");
        else while(player != NULL){
            sprintf(msg, "\n    %s", player->name);
            strcat(res.msg, msg);
            if(player == game->winner) strcat(res.msg, " (winner)");
            player = player->prev;
        }

        game = game->prev;
    }

    return res;
}

//-----------------------------------------------------------------------------
// USERS (list)
struct response users(struct request req){
    struct response res = resdef(1, "OK users", req);
    char msg[512];
    players_string(msg);
    strcpy(res.msg, msg);
    return res;
}
//-----------------------------------------------------------------------------
// players stringify
void players_string(char string[]){
    struct player *node = players;
    char line[64], status[16];
    line[0] = string[0] = '\0';
    while(node != NULL){
        sprintf(line, "%d %s %d %s wins:%d\n",
            node->id, node->name, node->pid, node->fifo, node->wins);
        strcat(string, line);       
        node = node->prev;
    }
}
//-----------------------------------------------------------------------------
// GAMES (List)
struct response list_games(struct request req){
    struct response res = resdef(1, "OK games", req);
    char msg[512];
    if(games == NULL){
        res.cmd = 0;
        strcpy(res.msg, "there aren't any games");
        return res;
    }
    games_string(msg);
    strcpy(res.msg, msg);
    return res;
}
//-----------------------------------------------------------------------------
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
        else if(games->done) sprintf(res.msg,"'%s' expired", games->name);
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
            sprintf(res.msg, "game '%s' already started", games->name);
            res.cmd = 0;
        }
    }
    else {
        sprintf(res.msg,"'%s' expired", games->name);
        res.cmd = 0;
    }

    return res;
}
//-----------------------------------------------------------------------------
// EXIT/LOGOUT (leaves)
struct response leaves(struct request req){
    struct response res = resdef(1, "OK bye", req);
    struct player *winner, *player = NULL;
    struct domino *tile = NULL;
    int i=0, n=0;
   
    if(games != NULL) {
        player = get_player_by_name(req.name, games->players);
        n = count_players();
    } 
    
    // remove player
    if(games != NULL &&  player != NULL){
        // game going on
        if(games->start_t && !games->done){
            // recover tiles
            append_tiles(games->tiles, player->tiles);
           
            // set next playing player
            if(playing == player) playing = playing_next();
            
            // delete player
            games->players = delete_player_by_name(req.name, games->players);

            // set move message
            sprintf(move.msg, "\n%s quit", req.name);

            if(count_players() < 2){
                sprintf(move.msg, "%s\n\033[0;35m%s Wins!\033[0m", move.msg, playing->name);
                move.winner = 0;// might be wrong beacuse move players wasn't reset yet
                winner = get_player_by_name(playing->name, players);
                winner->wins++;                
                games->done = 1;
                move.turn = -1;
            }
            else {
                // reset move players
                player = games->players;
                while(player != NULL){
                    // set move turn
                    if(strcmp(playing->name, player->name) == 0) move.turn = i;
                    strcpy(move.players[i++], player->name);
                    player = player->prev;
                }
                for(i; i<4; i++) move.players[i][0] = '\0';
            }
            // inform other players...only if gsme is playing
            //*
            if(fork() == 0){
                sleep(1);
                kill(getppid(), SIGALRM);
                exit(0);
            }//*/
        }
    }

    // delete user
    players = delete_player_by_name(req.name, players);

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
    struct response res = resdef(1, "OK info", req);
    struct player *node = NULL;//= games->players;
    char string[128];
    string[0] = '\0';

    if(games == NULL || games->done) {
        strcpy(res.msg, "no game playing");
        res.cmd = 0;
        return res;
    }

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
    struct player *winner = NULL, *player = NULL, *next = NULL;
    struct domino *tile = NULL;
    int tile_id, mask[2], n, i, j, p=0;
    char pos[8], string[64];
    pos[0]='\0';

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
        
        if(strcmp(pos, "right") == 0) p = 1;

        // coloca a peça no mosaico
        if(place_tile2(tile, games, p) == NULL){
            sprintf(res.msg, "tile does not fit on the %s", pos);
            res.cmd = 0;
            return res;
        };

        // next player//playing_next();
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
            winner = get_player_by_name(player->name, players);
            winner->wins++;

            sprintf(string, "\nit was %s's last tile", player->name);
            strcat(move.msg, string);
            
            sprintf(string, "\n\033[0;35m%s Wins!\033[0m", player->name);
            strcat(move.msg, string);

            move.winner = i-1;
            games->done = 1;
            move.turn = -1;
        }
        else {
            sprintf(string, "\033[0m\nwaiting for \033[0;32m%s\033[0m to play", playing->name);
            strcat(move.msg, string);
        }
        //TODO inform players  
        //TODO left/right side
        //TODO move to move
        //TODO remove after inform implant
        sprintf(res.msg, "placed tile %d", tile_id);
    
        if(fork() == 0){
            sleep(1);
            kill(getppid(), SIGALRM);
            exit(0);
        }
    }
    //5. tile does NOT fit mosaic    
    else{
        strcpy(res.msg, "tile does not fit");
        res.cmd = 0;
    }

    return res;
}

struct response get(struct request req){
    struct response res = resdef(1, "OK get", req);
    struct player *player = NULL;
    struct domino *tile = NULL;
    int mask[2];

    if(games == NULL || games->players == NULL || games->done) {
        strcpy(res.msg, "can't satisfy");
        res.cmd = 0;
        return res;        
    }

    player = get_player_by_name(req.name, games->players);
    if(player == NULL){
        strcpy(res.msg, "can't satisfy");
        res.cmd = 0;
        return res;
    }

    if(player != playing){
        strcpy(res.msg, "it's not your turn");
        res.cmd = 0;
        return res;
    }

    // tem dominó?
    // tests if a tile (that fits in the mosaic) exists
    get_ends(mask, games->mosaic);
    if(!count_tiles(games->mosaic) || tile_exists(mask, player->tiles)){
        strcpy(res.msg, "you don't need a tile");
        res.cmd = 0;
        return res;
    }

    // peça de domino da casa (saco do jogo)
    if((tile = give_tile_by_id(player->id, games)) == NULL){
        sprintf(res.msg, "no more tiles", move.msg, playing->name);
        res.cmd = 0;
        return res;
    }

    sprintf(res.msg, "%s, your new tile is %d:[%d,%d]",
        player->name, tile->id, tile->mask[0], tile->mask[1]);
    sprintf(move.msg, "\n%s received a tile", player->name);
    if(fork() == 0){
        sleep(1);
        kill(getppid(), SIGALRM);
        exit(0);
    }

    return res;
}

struct response pass(struct request req){
    struct response res = resdef(1, "OK pass", req);
    struct player *player = NULL;
    struct domino *tile = NULL;

    if(games == NULL || games->players == NULL || games->done) {
        strcpy(res.msg, "can't satisfy");
        res.cmd = 0;
        return res;        
    }

    player = get_player_by_name(req.name, games->players);
    if(player == NULL){
        strcpy(res.msg, "can't satisfy");
        res.cmd = 0;
        return res;
    }

    if(player != playing){
        strcpy(res.msg, "it's not your turn");
        res.cmd = 0;
        return res;
    }

    // set next playing player
    playing = playing_next();

    sprintf(move.msg, 
        "\n%s passes\n\033[0mwaiting for \033[0;32m%s\033[0m to play",
        req.name, playing->name);

    if(fork() == 0){
        sleep(1);
        kill(getppid(), SIGALRM);
        exit(0);
    }

    return res;
}

struct response help(struct request req){
    struct response res = resdef(1, "OK help", req);
    struct player *player = NULL;
    struct domino *tile = NULL;
    char item[16];
    int mask[2];

    if(games == NULL || games->players == NULL || games->done) {
        strcpy(res.msg, "can't satisfy");
        res.cmd = 0;
        return res;        
    }

    player = get_player_by_name(req.name, games->players);
    if(player == NULL){
        strcpy(res.msg, "can't satisfy");
        res.cmd = 0;
        return res;
    }
    
    if(player != playing){
        strcpy(res.msg, "it's not your turn");
        res.cmd = 0;
        return res;
    }

    if(!count_tiles(games->mosaic)){
        strcpy(res.msg, "\033[0;35mpick any tile\033[0m");
        res.cmd = 1;
        return res;    
    }

    res.msg[0] = '\0';
    tile = player->tiles;
    get_ends(mask, games->mosaic);
    while(tile != NULL){
        // tests if a tile fits in the mosaic
        if(validate_tile(mask, tile)){
            sprintf(item, "\033[0;35m%2d:[%d,%d]\033[0m", 
                tile->id, tile->mask[0], tile->mask[1]);
            strcat(res.msg, item);
            if(tile->next != NULL) strcat(res.msg, "\n");
        }
        tile = tile->next;
    }
    return res;
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
    int done, k, i, n = count_players();
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
        
        i=0;
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
