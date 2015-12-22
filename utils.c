/*
*
* 2014/10/30 created
* 2015/12/17 adapted for dds - disk dump scanner
*
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include "ddscan.h"

// *************************************************
// trim whitespace from left and right of string
//   2015/05/10 created
char *trimwhite(char *s)
{
char *tail;

    // trim left (front) 
    while(isspace(*s)) s++;

    // trim right (back/tail)
    tail = s + strlen(s);
    while(isspace(*--tail));
    *(tail+1) = '\0';

    return s;
}
    // char *trimwhite (char *string) {
    //   register char *s, *t;
    //   for (s = string; isspace (*s); s++)
    //     ;
    //   if (*s == 0)
    //     return (s);
    //   t = s + strlen (s) - 1;
    //   while (t > s && isspace (*t))
    //     t--;
    //   *++t = '\0';
    //   return s;
    // }

// *************************************************
// chmop off CR and/or LF from string
// 2015/11/25 created
// 2015/12/18 added check for tail moving pass s 
//            this is to catch string with all control chars
//              
void chomp(char *s) 
{
char *tail;
 printf("chomp length =%i ", strlen(s));

    tail = s + strlen(s);
    while(iscntrl(*--tail) && tail>=s);
    *(tail+1) = '\0';
 printf("chomp length =%i\n ", strlen(s));
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void lower(char s[]) {
int i;
for(i=0; i==strlen(s); ++i){
printf("%i", i);
       s[i] = tolower(s[i]);
}
}

// copy option argument value to dest string
// ....with a bit of error  checking
void get_optvalue(char *dest, char *optvalue) {

    memset(dest, '\0', sizeof(dest));

    if(optvalue==NULL) return;
    if(strlen(optvalue) > sizeof(dest)) return;

    strcpy(dest, optvalue);
}


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// file size -- figure out the size of the file 
//
int fsize(const char *filename) {
    struct stat st; 

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1; 
}

// -----------------------------------------------------
// convert byte from BCD to binary
int bcdtobin(uint8_t bcd)
{
    return (bcd&0x0f) + ((bcd&0xf0)>>4)*10;
}

// *************************************************
// hex to binary
//  2015/12/18 created
//
int hexbin(int hi, int low) {
    int low_nibble, hi_nibble;

    low_nibble = low<0x3a ? (low & 0x0f) :  (low &0x0f)+9;
    hi_nibble = hi<0x3a ? hi&0x0f : (hi&0x0f)+9;

    return low_nibble + (hi_nibble << 4);
}


// *************************************************
// hex print 
//  2015/12/18 created
//
void printhex(uint8_t b[], int addr, int count) {
int i, j;
int start, end;

    start = addr & 0xfffffff0;
    end = (addr+count) | 0x0000000f;
// printf("address=%04x count=%04x\n", addr, count);
// printf("start=%04x end=%04x\n", start, end);
    for (i=start; i<=end; i=i+16) {
        printf("%06x  ", i);
        for(j=0; j<16; j++) {
            if((i+j>=addr) && (i+j<addr+count) && (i+j<end))
                printf("%02x ", b[i+j]); 
            else
                printf(".. ");
        }
        printf("  |");

        for(j=0; j<16; j++) {
            if(j+i <= end)
                printf("%c", isprint(b[i+j]) ? b[i+j] : '.' );
        }
        printf("|\n");
    }

}


/* Print Usage Instructions */
void inst(char *iptr[], int status)
{
if(status != 0)
    fprintf(stderr,"Try, %s --help for more information.\n", program_name);

else {
    if (version) {
        printf("%s %s \n", program_name, VERSION);
        exit(SUCCESS);
        }

    printf("Usage: %s [OPTION]... [FILE]...\n", program_name);
    while (*iptr != NULL_CHAR)
      puts(*iptr++);
    }

exit(status == 0 ? SUCCESS : FAIL);
} /* inst */
