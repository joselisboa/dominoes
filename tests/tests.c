#include <stdio.h>

#ifdef _WIN32
#include "zelib_win32.h"
#else
#include "zelib.h"
#endif

void main(int varc, char *charv[]){
    int k;

    for(k=0; k<16; k++){
        printf("%d ", k); 
        _puts("_puts(\"Hello\", k);", k);
    }
    
    die("Good Bye");
}
