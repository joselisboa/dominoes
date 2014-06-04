#include <stdlib.h>
#include "public.h"
#include "zelib.h"
#define SERVER "./server"
#define FIFOPATH "/tmp/fifo"

int server_fifo;
struct request req;
struct response res;

void init();
void play();
void quit();
void print_response(struct response res);
int scanz(char *buffer);
void getname(char name[]);//, char *prompt[], char *format[]){
struct response send(struct request req, int server_fifo, int client_fifo);
void shutdown(int spid);
void restart(char proc[]);
void start(char proc[]);
int validate_cmd(char command[]);
int getzpid(char proc[]);
int cleanup(){
    close(server_fifo);
    unlink(req.fifo);
    return 1;
}

//-----------------------------------------------------------------------------
// M A I N
//-----------------------------------------------------------------------------
int main(int argc, char charv[]){
    int client_fifo;
    int i, k, n, r, player_id;
    char buffer[256], name[32];

    // signals setup
    signal(SIGUSR1, quit);// "...avisa os outros programas..."
    signal(SIGUSR2, play);// turn to play
    signal(SIGALRM, init);// game started

INIT:

    // [0] executa o servidor se o seu FIFO não existir
    if(!getzpid(SERVER)) start(SERVER);
    else if(access(DOMINOS, F_OK)) restart(SERVER);

    // [1] cria o fifo privado (cliente)
    sprintf(req.fifo, "%s_%d", FIFOPATH, getpid());
    if(mkfifo(req.fifo, 0666) < 0){
        perror(req.fifo);
        exit(1);
    }

    // [2] abre o fifo do servidor em modo de escrita
    if((server_fifo = open(DOMINOS, O_WRONLY)) < 0){
        unlink(req.fifo);
        perror(DOMINOS);
        exit(1);
    }
// Jogador já tem FIFO e o servidor a correr (será que está mesmo a correr?)
LOGIN:

    //CLEAR DATA FROM PREV LOGIN
    res.msg[0] = '\0';
    res.cmd = 0;
    req.player_id = 0;

    // get player's name
    getname(req.name);//, " your name: ", " %[a-ZA-Z0-9_]");

    if(strcmp(req.name, "exit") == 0) return cleanup();

    req.pid = getpid();
    strcpy(req.cmd, "login");

    // first request
    res = send(req, server_fifo, client_fifo);
    //print_response(res);

    if(!res.cmd){
        _puts(res.msg, 4);
        goto LOGIN;
    }

    req.player_id = res.req.player_id;
    _printf(3, "welcome, %s\n", req.name);


    // CMD LOOP
    while(1){
        // [3] player input
        printf("$ ");
        scanf(" %[^\n]", req.cmd);

        // PARSE CMD (16 bytes) HERE

        // [4] enviar dados ao servidor
        if((r = validate_cmd(req.cmd)) > 0)
            write(server_fifo, &req, sizeof(req));
        else {
            if(!r) printf("no such command '%s'\n", req.cmd);
            continue;
        }

        // [5] abrir fifo privado em modo de leitura
        if((client_fifo = open(req.fifo, O_RDONLY)) < 0){
            perror(req.fifo);
            break;
        }

        // [6] ler dados
        read(client_fifo, &res, sizeof(res));
        // fechar o fifo do cliente
        close(client_fifo);

        // PARSE RES HERE
        //TODO response parser switch

        if(res.cmd) _puts(res.msg, 2);
        else _puts(res.msg, 4);

        //DEV
        //print_response(res);

        // LOGOUT
        if(strcmp(req.cmd, "logout") == 0) goto LOGIN;

        // EXIT
        if(strncmp("exit", req.cmd, 3) == 0) return cleanup();
    }
    
    return cleanup();
}

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------
// validates command
int validate_cmd(char command[]){
    char cmd[8], param1[16], param2[16];
    int i, k;

    // scan command
    if(!(k = sscanf(command, "%s %s %s", cmd, param1, param2))) return 0;
    //printf("k=%d, cmd=%s, param1=%s, param2=%s", k, cmd, param1, param2);

    // client commands
    for(i=0; i<C; i++)
        if(strcmp(C_CMDS[i], cmd) == 0) 
            break;

    switch(i){
        case 0://"login",
        case 1://"exit",
        case 2://"logout",
            return 1;

        case 3://"status",
            return 0;

        case 4://"users",
            return 1;

        case 5://"new",// new <nome> <s>
            if(k != 3){
                _printf(4, "'%s' requires two additional paramaters.\n", cmd);
                return -1;
            }

            if(atoi(param2) < 1){
                _puts("the second parameter must be a number greater than zero", 4);
                return -1;
            }

            return 1;

        case 6://"play",//'play 99 left' or 'play 18 right'
            // client command
            if(k == 1) return 1;

            // player command
            if(k != 3) {
                _printf(4, "'%s' requires two additional paramaters\n", cmd);
                return -1;
            }

            if(atoi(param1) > 28 || atoi(param1) < 1){
                _puts("the first parameter must be a number between 1 and 28", 4);
                return -1;
            }

            if(!(strcmp(param2, "left") == 0)
                    && !(strcmp(param2, "right") == 0)){
                _puts("the second parameter must be 'left' or 'right'", 4);
                return -1;
            }

            return 1;

        case 7://"quit",
            return 0;

        case 8://"start",
            start(SERVER);
            return -1;

        case 9:// shutdown
            shutdown(getzpid(SERVER));
            return -1;

        case 10:// restart
            restart(SERVER);
            return -1;

        case 11:// users
            return 1;

        default:

        // player commands
        for(i=0; i<C; i++)
            if(strcmp(C_CMDS[i], cmd) == 0)
                break;

        switch(i){
            case 0://"tiles",
            case 1://"info",
            case 2://"game",
            case 3://"play",
                // IT NEVER PASSES HERE

            case 4://"get",
            case 5://"pass",
            case 6://"help",
            case 7://"giveup"
            default:
                return 0;
        }
    }

    return 0;
}

// ----------------------------------------------------------------------------
// scanz all user's input
int scanz(char *buffer){
    scanf("%[^\n]", buffer);
}

// ----------------------------------------------------------------------------
// prompts player's name
void getname(char name[]){//, char *prompt[], char *format[]){
    char buffer[32];
    int r=0;

    while(!r){
        _puts("your name:", 3);
        printf("$ ");
        scanf(" %[^\n]", buffer);
        r = sscanf(buffer, " %s", name);
    }
}

//-----------------------------------------------------------------------------
// SEND REQUEST
struct response send(struct request req, int server_fifo, int client_fifo){
    struct response res;

    // enviar dados ao servidor
    write(server_fifo, &req, sizeof(req));

    _puts("wait", 8);

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
// SHUTDOWN
void shutdown(int spid){
    _printf(8, "kill -%d  %d [%d]\n", SIGUSR2, spid, kill(spid, SIGUSR2));
    //TODO check status?
    cleanup();
    exit(1);
}

// ----------------------------------------------------------------------------
// RESTART
void restart(char proc[]){
    int pid = getzpid(proc);
    _printf(8, "%s is restarting\n", proc);
    shutdown(pid);
    sleep(1);
    start(SERVER);
}

// ----------------------------------------------------------------------------
// START (starts the server)
void start(char proc[]){
    _printf(8, "starting %s\nwait\n", proc);
    system(proc);
    sleep(1);
}

// ----------------------------------------------------------------------------
// GET (RUNNING PROCESS) PID
int getzpid(char proc[]){
    char buffer[16], format[64];
    FILE *finput;
    int i, j, k = strlen(proc), n;

    for(i=k; i>0; i--){
        if(proc[i]=='/'){
            for(j=0; i<k; i++, j++) buffer[j] = proc[i+1];
            break;
        }
    }

    //ps X | grep server$ | grep -o [^ ]* | head -1
    sprintf(format, "pgrep %s | head -1", buffer);
    buffer[0] = '\0';

    finput = popen(format, "r");
    if((n = read(fileno(finput), buffer, 15)) > 0) buffer[n] = '\0';
    pclose(finput);

    return atoi(buffer);
}

//-----------------------------------------------------------------------------
// DEV
void print_response(struct response res){
    puts("res: {");
    printf("    pid: %d\n", res.pid);
    printf("    msg: \"%s\"\n", res.msg);
    puts("    req: {");
    printf("        pid = %d,\n", res.req.pid);
    printf("        name: \"%s\",\n", res.req.name);
    printf("        player_id: %d,\n", res.req.player_id);
    printf("        fifo: \"%s\",\n", res.req.fifo);
    printf("        cmd: \"%s\"\n", res.req.cmd);
    puts("    },");
    printf("    res: %d\n", res.cmd);
    puts("}");
}

// ----------------------------------------------------------------------------
void init(){
   _puts("game started", 3);
}

// ----------------------------------------------------------------------------
void play(){
   _puts("your turn to play", 3);
}

// ----------------------------------------------------------------------------
void quit(){
    _puts("server is shutting down", 3);
}
