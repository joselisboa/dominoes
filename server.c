#define A 2 

void init(int);
void inform(int);
void stop(int);
void show(int);
void buzz(int);
int blocked();
int cleanup();
int procs();
int count_players();
int send(Response);
Response player_tiles();
Response game_tiles();
Response play_tile();
Response play_game();
Response giveup();
Response leaves();
Response login();
Response info();
Response info();
Response get();
Response pass();
Response help();
Response ni();
Response users();
Response status();
Response add_game();
Response list_games();
Response list_players();
Response router();
Response resdef(int, char []);
struct player *players = NULL;
struct player *playing = NULL;
struct player *last_move = NULL;
struct game *games = NULL;
Move move;
Request req;
int client_fifo, server_fifo;// important!
void players_string(char []);
void games_string(char []);
void tiles_string(char [], struct domino *);
void mosaic_string(char []);

//TODO move to public.h
char messages[8][32] = {
    "there's no live game",
    "create a game first",
    "you're not in a game",
    "there aren't any games",//3
    "there aren't any players",
    "can't satisfy",
    "it's not your turn",
     "you are already playing"
};
//-----------------------------------------------------------------------------
//                                                                     server.h
//TODO split file  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ->
//                                                                     server.c
//-----------------------------------------------------------------------------
int t(){
    time_t now_t, diff_t;

    if(!games->done && !games->start_t){
        time(&now_t);
        diff_t = difftime(now_t, games->create_t);        
        return (games->t - (int) diff_t);
    }

    return -1;
}

// request router
Response router(){
    char action[8], param2[8];
    int n, i, param1;

    // parse client request
    n = sscanf(req.cmd, "%s %d %s", action, &param1, param2);
    
    for(i=0; i<C; i++) 
        if(strcmp(C_CMDS[i], action) == 0) 
            break;
    
    // client commands
    switch(i) {
        case 0: return login(req);
        case 1:// exit
        case 2: return leaves(req);// logout   
        case 3: return status(req);        
        case 4: return users(req);
        case 5: return add_game(req);// new
        case 6: return (n == 1) ? play_game(req) : play_tile(req);
        case 7: return giveup(req);
        case 11: return list_games(req);
        default: 
            for(i=0; i<P; i++) 
                if(strcmp(P_CMDS[i], action) == 0) 
                    break;

        // Players commands
        switch (i) {
            case 1: return info(req);
            case 0: return player_tiles(req); 
            case 2: return game_tiles(req);
            case 4: return get(req);
            case 5: return pass(req);
            case 8:// hint
            case 6: return help(req);
            case 7: return giveup(req);
            case 9: return list_players(req);
            default: return ni(req);
        }
    }
}

//-----------------------------------------------------------------------------
// Blocked, none of the players have a tile that fits the mosaic
int blocked(){
    struct player *player = NULL;
    struct domino *tile = NULL;
    int mask[2];

    if(games == NULL || (player = games->players) == NULL) return 0;
    
    get_ends(mask, games->mosaic);
    while(player != NULL){
        tile = player->tiles;
        while(tile != NULL) if(tile_exists(mask, games->mosaic)) return 0; 
        player = player->prev;
    }
    return 1;
}

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

    exit(cleanup());
}

//-----------------------------------------------------------------------------
// SHOW 
void show(int sig){
    struct game *game = games;
    struct player *player = NULL;
    char msg[256];
    int n;

    printf("\nGAMES");
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
    printf("\nUSERS\n");
    players_string(msg);
    puts(msg);

    printf("press enter to continue\n");
    fflush(stdout);
}

//-----------------------------------------------------------------------------
// STATUS
Response status(){
    Response res = resdef(1, "OK status");
    struct game *game = games;
    struct player *player = NULL;
    char msg[256] = {'\0'};
    int n;

    strcpy(res.msg, "GAME STATUS");
    
    if(game == NULL) strcat(res.msg, " (no games)");
    else while(game != NULL){
        sprintf(msg, "\n%d %s", game->id, game->name);
        strcat(res.msg, msg);
        if(!game->start_t && !game->done) {  
            sprintf(msg, " (starts in %d sec)", t());
            strcat(res.msg, msg);
        }
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
// USERS list users logged in
Response users(){
    Response res = resdef(1, "OK users");
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
        sprintf(line, "%d %s %s wins:%d",
            node->id, node->name, node->fifo, node->wins);
        strcat(string, line);
        if(node->prev != NULL) strcat(string, "\n");       
        node = node->prev;
    }
}

//-----------------------------------------------------------------------------
// GAMES list games
Response list_games(){
    Response res = resdef(1, "OK games");
    char msg[512];
    if(games == NULL){
        res.cmd = 0;
        strcpy(res.msg, messages[3]);
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
            else if(node->winner == NULL) strcpy(status, "done");
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
Response list_players(){
    Response res = resdef(1, "OK listing players");
    struct player *node = NULL;
    char msg[512] = {'\0'};
    char line[64] = {'\0'};
    
    if(games == NULL || games->done){
        res.cmd = 0;
        strcpy(res.msg, messages[0]);
        return res;
    }
    
    node = games->players;    
    while(node != NULL){
        sprintf(line, "%d %s", node->id, node->name);
        strcat(msg, line);       
        if((node = node->prev) != NULL) strcat(msg, "\n");
    }
    
    strcpy(res.msg, msg);
    return res;
}

//-----------------------------------------------------------------------------
// NEW (Add Game)
Response add_game(){
    Response res = resdef(1, "OK new");
    char name[32];
    int t, pid;
    name[0] = '\0';

    if(games != NULL && !games->done) {
        strcpy(res.msg, messages[0]);
        res.cmd = 0;
        return res;
    }

    sscanf(req.cmd, "new %s %d", name, &t);
    sprintf(res.msg, "game '%s' created", name);

    games = new_game(games);
    strcpy(games->name, name);
    games->t = t;
    time(&games->create_t);

    signal(SIGALRM, init);
    alarm(t);
    return res;
}

//-----------------------------------------------------------------------------
// PLAY (adds player to game)
Response play_game(){
    Response res = resdef(1, "OK play");
    struct player node, *user = NULL, *player = NULL;
    int i, n;

    // there are no games
    if(games == NULL){
        strcpy(res.msg, messages[1]);
        res.cmd = 0;
        return res;
    }
    
    n = count_players();

    // player already subscribed?
    player = get_player_by_name(req.name, games->players);
    if(player != NULL){
        if(games->start_t) strcpy(res.msg, messages[7]);
        else if(games->done) sprintf(res.msg,"'%s' expired", games->name);
        else sprintf(res.msg, "you already subscribed to '%s'", games->name);
        

        res.cmd = 0;
        return res;
    }
    // available game
    else if(!games->done){
        sprintf(res.msg, 
            "subscribed to '%s'\nthe game starts in %d seconds", 
            games->name, t());

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
// LOGIN
Response login(){
    struct player *aux_player = NULL;
    Response res = resdef(1, "OK welcome");

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
Response info(){
    Response res = resdef(1, "OK info");
    struct player *node = NULL;
    char string[128];
    string[0] = '\0';

    if(games == NULL || games->done) {
        strcpy(res.msg, "there's no live game");
        res.cmd = 0;
        return res;
    }

    sprintf(res.msg, "Game: %s\n", games->name);
    sprintf(string, "Stock: %d tiles\n", count_tiles(games->tiles));
    strcat(res.msg, string);
    sprintf(string, "Mosaic: %d tiles\n", count_tiles(games->mosaic));
    strcat(res.msg, string);
    
    node = games->players;
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
Response player_tiles(){
    Response res = resdef(1, "OK tiles");
    struct player *player = get_player_by_name(req.name, games->players);
    char hand[128];
    strcpy(res.msg, "your tiles");   
    tiles_string(hand, player->tiles);
    strcat(res.msg, hand);
    return res;
}

//-----------------------------------------------------------------------------
// GAME response with mosaic tiles
Response game_tiles(){
    Response res = resdef(1, "OK tiles");
    char hand[128];
    
    if(games != NULL && !games->done){
        sprintf(res.msg, "%s's mosaic\n", games->name);
        mosaic_string(hand);
        strcat(res.msg, hand);
    }
    else {
        strcpy(res.msg, messages[0]);
        res.cmd = 0;
    } 

    return res;
}

//-----------------------------------------------------------------------------
// PLAY places tile on mosaic
Response play_tile(){
    Response res = resdef(1, req.cmd);
    struct player *winner = NULL, *player = NULL, *next = NULL;
    struct domino *tile = NULL;
    int tile_id, mask[2], n, i, j, p=0;
    char pos[8], string[64];
    pos[0]='\0';

    string[0] = '\0';
    sscanf(req.cmd, "play %d %s", &tile_id, pos);

    player = get_player_by_name(req.name, games->players);
    
    // player == NULL => player is not playing the game
    if(player != playing){
        strcpy(res.msg, "its not your turn");
        res.cmd = 0;
        return res;
    }

    tile = get_tile_by_id(tile_id, player->tiles);
    // tile == NULL => player does not have that tile
    if(tile == NULL){
        sprintf(res.msg, "you don't have tile %d", tile_id);
        res.cmd = 0;
        return res;
    }
    
    get_ends(mask, games->mosaic);
    // mosaic is empty
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

        // move message
        sprintf(move.msg, "\n%s played tile %d:[%d,%d]", 
            player->name, tile->id, tile->mask[0], tile->mask[1]);

        // bocked game?
        if(games->tiles == NULL && blocked()){
            strcat(move.msg, "\ngame is blocked");            
            if((winner = has_less()) == NULL){
                // no winner
                strcat(move.msg, "\nthere are no winners");
            }
            else {
                sprintf(string, "%s Wins!", winner->name);
                strcat(move.msg, "\nthere are no winners");
            }

            goto DELIVERY;
        }

        // next player//playing_next();      
        n = count_players();
        for(i=0; i<n; i++) if(strcmp(playing->name, move.players[i]) == 0){
            j = (i+1 == n) ? 0 : i + 1;
            last_move = playing;
            playing = get_player_by_name(move.players[j], games->players);
            move.turn = j;
            break;
        }

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
            sprintf(string, "\nwaiting for %s to play", chameleon(playing->name, 3));
            strcat(move.msg, string);
        }

        DELIVERY:
        sprintf(res.msg, "you played tile %d:[%d,%d]", 
            tile_id, tile->mask[0], tile->mask[1]);

        buzz(1);
    }
    // tile does NOT fit mosaic    
    else{
        strcpy(res.msg, "tile does not fit");
        res.cmd = 0;
    }

    return res;
}

//-----------------------------------------------------------------------------
// GET
Response get(){
    Response res = resdef(1, "OK get");
    struct player *player = NULL;
    struct domino *tile = NULL;
    int mask[2];

    if(games == NULL || games->players == NULL || games->done) {
        strcpy(res.msg, messages[5]);
        res.cmd = 0;
        return res;        
    }

    player = get_player_by_name(req.name, games->players);
    if(player == NULL){
        strcpy(res.msg, messages[5]);
        res.cmd = 0;
        return res;
    }

    if(player != playing){
        strcpy(res.msg, messages[6]);
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
        sprintf(res.msg, "no more tiles");
        res.cmd = 0;
        return res;
    }

    sprintf(res.msg, "%s, your new tile is %d:[%d,%d]",
        player->name, tile->id, tile->mask[0], tile->mask[1]);
    sprintf(move.msg, "\n%s received a tile", player->name);
    
    buzz(1);

    return res;
}

//-----------------------------------------------------------------------------
// PASS
Response pass(){
    Response res = resdef(1, "OK pass");
    struct player *player = NULL;
    struct domino *tile = NULL;

    if(games == NULL || games->players == NULL || games->done) {
        strcpy(res.msg, messages[5]);
        res.cmd = 0;
        return res;        
    }

    player = get_player_by_name(req.name, games->players);
    if(player == NULL){
        strcpy(res.msg, messages[5]);
        res.cmd = 0;
        return res;
    }

    if(player != playing){
        strcpy(res.msg, messages[6]);
        res.cmd = 0;
        return res;
    }

    // set next playing
    last_move = playing;
    playing = playing_next();

    sprintf(move.msg, 
        "\n%s passes\n\033[0mwaiting for \033[0;32m%s\033[0m to play",
        req.name, playing->name);

    buzz(1);

    return res;
}

//-----------------------------------------------------------------------------
// HELP/HINT
Response help(){
    Response res = resdef(1, "OK help");
    struct player *player = NULL;
    struct domino *tile = NULL;
    char item[16];
    int mask[2];

    if(games == NULL || games->players == NULL || games->done) {
        strcpy(res.msg, messages[5]);
        res.cmd = 0;
        return res;        
    }

    player = get_player_by_name(req.name, games->players);
    if(player == NULL){
        strcpy(res.msg, messages[5]);
        res.cmd = 0;
        return res;
    }
    
    if(player != playing){
        strcpy(res.msg, messages[6]);
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
            sprintf(item, "\033[0;35m%2d:[%d,%d]\033[0m\n", 
                tile->id, tile->mask[0], tile->mask[1]);
            strcat(res.msg, item);
        }
        tile = tile->next;
    }
    
    res.msg[strlen(res.msg)-1] = '\0';

    if(res.msg[0] == '\0') {
        strcpy(res.msg, "you have nothing");
        res.cmd = 0;
    }

    return res;
}

//-----------------------------------------------------------------------------
// EXIT/LOGOUT leaves
Response leaves(){
    Response res = giveup(req);
    
    // delete user
    players = delete_player_by_name(req.name, players);
    strcpy(res.msg, "OK bye");
    return res;
}

//-----------------------------------------------------------------------------
// GIVEUP quits game
Response giveup(){
    Response res = resdef(1, "OK quitter");
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
            if(playing == player) {
                last_move = playing;
                playing = playing_next();
            }
            
            // delete player
            games->players = delete_player_by_name(req.name, games->players);

            // set move message
            sprintf(move.msg, "\n%s quits", req.name);

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
            
            // inform
            buzz(1);
        }
    }
    else {
        strcpy(res.msg, messages[2]);
        res.cmd = 0;
    }

    return res;
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
// INIT SIGALRM handler for game start
void init(int sig){
    struct player *node, *player = games->players;;
    int player_fifo;
    int done, k, i=0, n = count_players();
    char hand[128], tile[16];
    struct domino *tiles;

    if(count_players() > 1){
        signal(SIGALRM, inform);
  
        // distribute dominoes
        start(games);
        time(&games->start_t);
        
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

        // send data to players
        player = games->players;
        while(player != NULL){
            kill(player->pid, SIGUSR1);
            done = 0;
            k = 0;
            
            //TODO send_move(fifo, move);

            do {// abrir FIFO do jogador em modo de escrita
                if((player_fifo = open(player->fifo,O_WRONLY|O_NDELAY)) == -1){
                  sleep(5);
                }
                else {
                    move.turn = 1;
                    move.msg[0] = '\0';
                    tile[0] = '\0';

                    sprintf(move.msg, "\nstarting %s", games->name);                   
                    strcat(move.msg, chameleon("\nyour dominoes", 2));

                    tiles_string(hand, player->tiles);
                    strcat(move.msg, chameleon(hand, 2));  
                    
                    sprintf(hand, "\nwaiting for %s to play", 
                        chameleon(playing->name, 3));
                    strcat(move.msg, hand);
                                        
                    // enviar resposta pelo FIFO do jogador
                    write(player_fifo, &move, sizeof(move));
                    close(player_fifo);
                    sleep(1);// sleep existe (para alguma coisa serve) 
                    done = 1;
                }
            } while(k++ < 5 && !done);
            player = player->prev;
        }
    }
    else games->done = 1;
}

//-----------------------------------------------------------------------------
// DEFRES Default Response
Response resdef(int cmd, char msg[]){
    Response res;
    
    res.pid = getpid();
    strcpy(res.msg, msg);
    res.req = req;
    res.cmd = cmd;
    return res;
}

//-----------------------------------------------------------------------------
// NI Not Implemented Response
Response ni(){
    Response res;
    
    res.pid = getpid();
    strcpy(res.msg, "NOT IMPLEMENTED");
    res.req = req;
    res.cmd = 0;
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
int send(Response res){
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

//-----------------------------------------------------------------------------
// Stringify tiles
void tiles_string(char string[], struct domino *tiles){
    char tile[16] = {'\0'};
    string[0] = '\0';

    if(tiles == NULL) strcpy(string, "\n[]");
    else while(tiles != NULL){
        sprintf(tile, "\n%2d:[%d,%d]", tiles->id, tiles->mask[0], 
            tiles->mask[1]);
        strcat(string, tile);
        tiles = tiles->next;
    }
}

//-----------------------------------------------------------------------------
// stringify mosaic tiles
void mosaic_string(char string[]){
    struct domino *node = games->mosaic;
    char tile[16] = {'\0'};
    string[0] = '\0';

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
    int deliver, done, k, i=0, n = count_players();

    // enviar dados aos jogadores do jogo
    player = games->players;
    while(player != NULL){
        deliver = games->done ? TRUE : (player != last_move);
        if(deliver){
            kill(player->pid, SIGUSR1);
            done = 0;
            k = 0;
            do {// abrir FIFO do jogador em modo de escrita
                if((player_fifo = open(player->fifo, 
                    O_WRONLY|O_NDELAY)) == -1)
                    sleep(5);
                else {
                    write(player_fifo, &move, sizeof(move));
                    close(player_fifo);
                    sleep(1); // sleep existe, gostem ou não
                    done = 1;
                }
            } while(k++ < 5 && !done);          
        }
        player = player->prev;
    }
    return;
}

//-----------------------------------------------------------------------------
// like alarm
void buzz(int t){
    sleep(t);// sleep(1) is here, deal with it 
    if(fork() == 0){
        kill(getppid(), SIGALRM);
        exit(0);
    }    
}

//-----------------------------------------------------------------------------
// deleted public fifo
int cleanup(){
    unlink(DOMINOS);
    return 1;
}
