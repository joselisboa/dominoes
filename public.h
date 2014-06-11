#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include "zelib.h"

const char *DOMINOS = "/tmp/DOMINOS";

#define SERVER "./dominoesd"
#define NUMPECAS 7
#define P 10
#define C 12

// comandos do jogador (ao cliente)
char *P_CMDS[P] = {
    "tiles", "info", "game", "play", "get",
    "pass", "help", "giveup", "hint", "players"};

// comandos do cliente (ao servidor)
char *C_CMDS[C] = {
    "login", "exit", "logout", "status", "users",
    "new", "play", "quit", "start", "shutdown",
    "restart", "games"};

typedef struct request {
	int pid;
	int player_id;
    char name[32];
	char fifo[32];
	char cmd[64];
} Request;

typedef struct response {
	int pid;
	char msg[512];
	struct request req;
	int cmd;// 0:msg, 1:quit, -1:error
} Response;

typedef struct move {
    int move;// {1,2,3,...} 
    char name[32];// name of the game
    char msg[256];// server message
    int turn; // next player {-1,0,1,2,3} (-1 for game over) 
    int winner; // {-1,0,1,2,3} (-1 for no winner)  
    char players[4][32];// [0,1,2,3]
} Move;

int send_signal(int pid, int SIG){
    int ret;
    ret = kill(pid, SIG);
    //printf("ret : %d", ret);
    return ret;
}

// ----------------------------------------------------------------------------
// Gets the PID from a running process 
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
