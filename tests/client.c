#include <stdlib.h>
#include "public.h"

/*
[1] criar FIFO privado (cliente)
[2] abrir o FIFO público (servidor) em mode de escrita
[3] interagir com a linha de comandos (jogador)
[4] enviar dados do jogador pelo FIFO público
[5] abrir o FIFO privado em mode de leitura
[6] receber dados do servidor pelo FIFO privado
->[3]
*/

int main(int argc, char charv[]){
    int server_fifo, client_fifo, n;
    static char buffer[PIPE_BUF];//mantem valor entre invocações
    struct request dados;//estrutura para enviar ao servidor

    //TODO dominosd (server), dominos (client)
    // executa o servidor de dominos
    if(access(DOMINOS, F_OK)){
        system("./server &");
    }

    sleep(1);

    // nome do fifo (privado) do cliente
    sprintf(dados.fifo, "/tmp/fifo%d", getpid());

    // [1] criar fifo privado
    //if(mknod(dados.fifo, S_IFIFO | 0666, 0) < 0){
    if(mkfifo(dados.fifo, 0666) < 0){
        perror(dados.fifo);
        exit(1);
    }

    puts("[2]");
    // [2] abrir o fifo do servidor em modo de escrita
    if((server_fifo = open(DOMINOS, O_WRONLY)) < 0){
        unlink(dados.fifo);
        perror(DOMINOS);
        exit(1);
    }

    while(1){
        puts("[3]");
        // [3] linha de comandos
        write(fileno(stdout), "\ncmd>", 5);
        memset(dados.cmd, 0x0, B_SIZE);
        n = read(fileno(stdin), dados.cmd, B_SIZE);
        if(strncmp("quit", dados.cmd, n-1) == 0){
            break;
        }

        puts("[4]");
        // [4] enviar dados ao servidor
        write(server_fifo, &dados, sizeof(dados));

        // [5] abrir fifo privado em modo de leitura
        if((client_fifo = open(dados.fifo, O_RDONLY)) < 0){
            puts("1");
            perror(dados.fifo);
            break;
        }

        // [6] ler dados
        while((n = read(client_fifo, buffer, PIPE_BUF)) > 0) {
            write(fileno(stderr), buffer, n);
        }

        // fechar o fifo
        close(client_fifo);
    }

    close(server_fifo);
    unlink(dados.fifo);

}
