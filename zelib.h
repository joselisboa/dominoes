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

void _linha(char c){
	int i;
	char s[81];
	for(i=0; i<80; i++) s[i] = c;
	s[i] = '\0';
	printf("%s\n", s);
}

// backup text file
int _backup(char *filename){
	FILE *file, *backup;
	char c;

	if((file = fopen(filename, "r")) == NULL) return FALSE;
	strcat(filename, ".bak");
	if((backup = fopen(filename, "w")) == NULL) return FALSE;

	while((c = fgetc(file)) != EOF) fputc(c, backup);

	fclose(file);
	fclose(backup);

	return TRUE;
}

void _rename(char *filename, char *afix){
	struct filename file;

	file = _file(filename);

	strcpy(filename, file.name);
	strcat(filename, afix);
	strcat(filename, ".");
	strcat(filename, file.extension);
}

int copy_textfile(char *filename, char *copy){
	FILE *input, *output;
	char c;
	int i;

	if((input = fopen(filename, "r")) == NULL) return FALSE;
	if((output = fopen(copy, "w")) == NULL) return FALSE;

//fprintf(output, "%3d ", ++i);// %d na primeira linha

	while((c = fgetc(input)) != EOF) {
			fputc(c, output);
//if(c == '\n' || c == '\r') fprintf(output, "%3d ", ++i);
	}

	fclose(output);
	fclose(input);

	return TRUE;
}

void _entrelinhas(char c, char *format, ...){
	va_list args;

	//HANDLE  hConsole;
  	//hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    //SetConsoleTextAttribute(hConsole, 8);
	_linha(c);
	//SetConsoleTextAttribute(hConsole, 15);

	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	//SetConsoleTextAttribute(hConsole, 8);
	_linha(c);
    //SetConsoleTextAttribute(hConsole, 7);
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
