#define FIFOPATH "/tmp/fifo"

int server_fifo, client_fifo;
struct request req;
struct response res;

void play(int sig);
void quit(int sig);
void print_response(struct response res);
int auth(char name[]);//, char *prompt[], char *format[]){
struct response send(struct request req);
void shutdown(int spid);
void start(char proc[]);
int validate(char command[], char buffer[]);
int cleanup();

int playing = false;
int is_playing();
void status(int argc, char *argv[]);
int help(char buffer[]);
//-----------------------------------------------------------------------------
//                                                                     client.h
// TODO split - - - > - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                     client.c
//-----------------------------------------------------------------------------
// validates command
int validate(char command[], char buffer[]){
    char cmd[8], param1[32], param2[16];
    int i, k;

    // scan user commands
    if(!(k = sscanf(command, "%s %s %s", cmd, param1, param2))) return 0;

    // client commands
    for(i=0; i<C; i++)
        if(strcmp(C_CMDS[i], cmd) == 0) 
            break;

    switch(i){
        case 0:// login
        case 1:// exit
        case 2:// logout
        case 3:// status
        case 4:// users
            return 1;

        case 5:// new <nome> <s>
            if(k != 3){
                strcpy(buffer, chameleon("usage", 8));
                strcat(buffer, chameleon(" new myGame 180", 4));
                return 0;
            }
            
            if(atoi(param2) < 1){
                strcpy(buffer, 
                    chameleon("parameter 2 must be a positive number", 4));
                return 0;
            }
            else if(atoi(param2) > 600) {
                strcpy(buffer, chameleon("the interval is too long", 4));
                return 0;
            }

            return 1;

        // play
        case 6:            
            // request to join game
            if(k == 1) return 1;

            // the user is not playing
            if(!is_playing(buffer)) return 0;

            // player command
            if(k < 2) {
                strcpy(buffer, chameleon(cmd, 12));
                strcat(buffer, 
                    chameleon(" usage: play 23 [right/left]", 4));
                return 0;
            }
            
            if(atoi(param1) > 28 || atoi(param1) < 1){
                strcpy(buffer, chameleon("tile id is a number between 1 and 28", 4));
                return 0;
            }
            
            if(k == 3) if(!(strcmp(param2, "left") == 0)
                    && !(strcmp(param2, "right") == 0)){
                sprintf(buffer, "parameter two is %s or %s", 
                    chameleon("left", 15), chameleon("right", 15));
                return 0;
            }

            return 1;

        case 7:// quit
        case 11:// users 
            return 1;

        case 8:// start
            start(SERVER);
            return 0;

        case 9:// shutdown
            kill(getzpid(SERVER), SIGUSR2);
            //shutdown(getzpid(SERVER));
            sleep(1);
            return -1;

        case 10:// restart
        default:

        // player commands
        for(i=0; i<P; i++) if(strcmp(P_CMDS[i], cmd) == 0) break;

        switch(i){            
            case 1:// info           
            case 0:// tiles*
            case 2:// game*
            case 4:// get
            case 5:// pass
            //case 6:// help
            case 7:// giveup*
            case 8:// hint
                return is_playing(buffer);

            case 6: 
                printf("type %s for help on choosing a tile to play\n", 
                    chameleon("hint", 5));
                return help(buffer);

            // players (list)
            case 9: return 1;
            default: 
                strcpy(buffer, chameleon("don't know ", 4));
                strcat(buffer, chameleon(req.cmd, 12));                
                return 0;
        }
    }

    return 0;
}

// ----------------------------------------------------------------------------
// prompts player's name
int auth(char name[]){
    char buffer[32];
    int i=C, j=P, r = 0;

    puts(chameleon("DOMINOES", 15));
    fflush(stdout);

    while(!r){
    BEGINING:   
        puts(chameleon("enter your name", 3));
        printf("> ");
        scanf(" %[^\n]", buffer);
        r = sscanf(buffer, " %s", name);
        
        if(!strcmp(name, "exit")) return 0;

        // client commands
        for(i=0; i<C; i++) 
            if(!strcmp(C_CMDS[i], name))
                break;

        // player commands
        if(i == C) 
            for(j=0; j<P; j++) 
                if(!strcmp(P_CMDS[j], name)) 
                    break;

        if((i != C || j != P)){ 
            printf("the name %s is reserved\n", chameleon(name, 15));
            goto BEGINING;
        }
        else break;
    }

    return 1;
}

//-----------------------------------------------------------------------------
// sends request to server
struct response send(struct request req){
    struct response res;

    // enviar dados ao servidor
    write(server_fifo, &req, sizeof(req));

    // abrir fifo privado em modo de leitura
    if((client_fifo = open(req.fifo, O_RDONLY)) < 0){
        perror(req.fifo);
        exit(-1);
    }
    // ler dados
    read(client_fifo, &res, sizeof(res));

    // fechar o fifo
    close(client_fifo);

    return res;
}

// ----------------------------------------------------------------------------
// SHUTDOWN shuts down server and exits
void shutdown(int spid){
    kill(spid, SIGUSR2);
    //sprintf(leonbuffer, "kill -%d  %d [%d]\n", SIGUSR2, spid, kill(spid, SIGUSR2));
    //puts(chameleon(leonbuffer, 8));
    //TODO check status?
    sleep(1);
}

// ----------------------------------------------------------------------------
// START (starts a process)
void start(char proc[]){
    _printf(8, "starting %s\nwait\n", proc);
    system(proc);
    sleep(1);// give 1 sec for the process to start
}

//-----------------------------------------------------------------------------
// PLAY (game play) SIGUSR1 handler
void play(int sig){
    struct move status;
    int len;

    // [5] abrir fifo privado em modo de leitura
    if((client_fifo = open(req.fifo, O_RDONLY)) < 0){
        perror(req.fifo);
        return;
    }

    // [6] ler dados
    read(client_fifo, &status, sizeof(status));
    // fechar o fifo do cliente
    close(client_fifo);

    // get game info
    if(status.move == 1) playing = true;
    else if(status.turn == -1) playing = false;

    puts(status.msg);
    printf("> ");
    fflush(stdout);
}

//-----------------------------------------------------------------------------
// QUIT closes client (SIGUSR2 handler)
void quit(int sig){
    int i = 4;
    puts(chameleon("the server is shutting down", 3));
    sleep(1);// 1 sec before terminating
    puts(chameleon("client terminated", 5));
    fflush(stdout);
    exit(cleanup());
}

//-----------------------------------------------------------------------------
int cleanup(){
    close(server_fifo);
    unlink(req.fifo);
    return 1;
}

int is_playing(char buffer[]){
    if(!playing) {
        strcpy(buffer, chameleon("you aren't playing",4));
        return 0;
    }

    return 1;
}

// executa o servidor se o seu FIFO não existir
void status(int argc, char *argv[]){
    if(!getzpid(SERVER)) {
         if(argc > 1 && strcmp(argv[1], "admin") == 0) start(SERVER);
         else {
             puts("the server is not running");
             exit(1);
         }
    }
    else if(access(DOMINOS, F_OK)) {
        puts("can't access the server's FIFO");
        exit(1);
    }    
}

int help(char buffer[]){
    char c;
    int in, n;

    in = open("HELP", O_RDONLY);
    while(read(in, &c, sizeof(c)) == 1){
        write(2, &c, sizeof(c));
    }

    close(in);

    return 0;
}