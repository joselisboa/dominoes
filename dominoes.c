#include <stdlib.h>
#include "public.h"
#include "zelib.h"
#include "client.c"

//-----------------------------------------------------------------------------
// DOMINOES
//-----------------------------------------------------------------------------
int main(int argc, char *argv[]){
    int i, k, n, r, player_id;
    char buffer[256], name[32];

    signal(SIGUSR1, play);  
    signal(SIGUSR2, quit);

INIT:
    
    // [0] executa o servidor se o seu FIFO não existir
    if(!getzpid(SERVER)) {
         if(argc > 1 && strcmp(argv[1], "admin") == 0) start(SERVER);
         else {
             puts("the server is not running");
             exit(1);
         }
    }
    else if(access(DOMINOS, F_OK)) {
         if(argc > 1 && strcmp(argv[1], "admin") == 0) restart(SERVER);
         else {
             puts("the server is not running");
             exit(1);
         }
    }

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
    
    // assume-se que o servidor está a correr...
LOGIN:
    printf("\e[H\e[2J");//printf("\33[H\33[2J"); 
    // clear data from previous login
    res.msg[0] = '\0';
    res.cmd = 0;
    req.player_id = 0;    
    
    puts("DOMINOES");
    
    // get player's name
    getname(req.name);

    if(strcmp(req.name, "exit") == 0) return cleanup();
    
    req.pid = getpid();
    strcpy(req.cmd, "login");
    res = send(req);
    
    //print_response(res);

    if(!res.cmd){
        _puts(res.msg, 4);
        goto LOGIN;
    }

    req.player_id = res.req.player_id;
    _printf(3, "welcome, %s\n", req.name);

    // CMD loop
    while(1){

        // [3] player input
        printf("> ");
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

        // parse response here
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
