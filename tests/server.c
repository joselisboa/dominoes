#include "public.h"

/*
[1] Gerar FIFO público, e aguardar dados do cliente
<----------------------------------------------|
[2] Ler dados pelo FIFO público                |
[3] Abrir FIFO do cliente em modo de escrita   |
[4] Executar pedido do cliente                 |
[5] Enviar dados pelo FIFO do cliente          |
---------------------------------------------->|
*/

int main(){
    int client_fifo, server_fifo, aux_fifo, n, done, k=0;
    struct request dados;
    FILE *fin;
    static char buffer[PIPE_BUF];
    char teste[12];

    // [1]
    //mknod(DOMINOS, S_IFIFO | 0666, 0);
    mkfifo(DOMINOS, 0666);

    if((server_fifo = open(DOMINOS, O_RDONLY)) < 0 ||
            (aux_fifo = open(DOMINOS, O_WRONLY | O_NDELAY)) < 0) {
        perror(DOMINOS);
        exit(1);
    }
//---------------------------------------------------------------------
//while(1) {
//if((server_fifo = open(DOMINOS, O_RDONLY))<0) break;

    // [2]
    while(read(server_fifo, &dados, sizeof(dados)) > 0){
        n = 0;
        done = 0;

        do {
            // [3]
            if((client_fifo = open(dados.fifo, O_WRONLY | O_NDELAY)) == -1) {
                sleep(5);
            }
            else {
                // [4]
                fin = popen(dados.cmd, "r");
                write(client_fifo, "\n", 1);

                // [5]
                while((n = read(fileno(fin), buffer, PIPE_BUF)) > 0){
                    write(client_fifo, buffer, n);
                    memset(buffer, 0x0, PIPE_BUF);
                }

                //sprintf(teste, "\nN%4dK%4d\n", n, k++);
                //write(client_fifo, teste, 12);

                pclose(fin);
                close(client_fifo);
                done = 1;
            }

        } while(n++ < 5 && !done);

        if(!done) {
            perror("Did not access the client fifo\n");
            exit(1);
        }
    }
//}
//------------------------------------------------------------------------
    return 0;
}

