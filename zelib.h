#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#define TRUE 1
#define true 1
#define FALSE 0
#define false 0

char _colors[16][10] = {
    "\033[0;30m",//black
    "\033[0;34m",//blue2
    "\033[0;32m",//pale green2
    "\033[0;36m",//cyan2
    "\033[0;31m",//red/brown2
    "\033[0;35m",//mangeta2
    "\033[0;33m",//pale yellow2
    "\033[0;37m",//silver
    "\033[1;30m",//gray
    "\033[1;34m",//blue
    "\033[1;32m",//green
    "\033[1;36m",//cyan
    "\033[1;31m",//red
    "\033[1;35m",//mangeta
    "\033[1;33m",//yellow
    "\033[1;37m"//white
};

struct filename {
	char name[64];
	char extension[16];
};

struct filename _file(char filename[]){
	int i, j, l, len;
	struct filename file;

	for(len=strlen(filename), l=1; l<len; l++)
		if(filename[len-l] == '.') break;

	for(i=0; i<len-l; i++) file.name[i] = filename[i];
	file.name[i] = '\0';

	for(++i, j=0; i+j<len; j++) file.extension[j] = filename[i+j];
	file.extension[j] = '\0';

	return file;
}

void _puts(char *format, int k);

// like PHP function die
void die(char *msg,...){
    va_list args;

    _puts(msg, 8);
	exit(0);
}

void _puts(char *format, int k){
    if(k>15 || k <0) k = 7;
    printf("%s", _colors[k]);
    puts(format);
    printf("\033[0m");
}

void _printf(int k, char *format, ...){
	va_list args;

    if(k>15 || k<0) k = 7;
    printf("%s",_colors[k]);
    va_start(args, format);
	vprintf(format, args);
	va_end(args);
    printf("\033[0m");
}
