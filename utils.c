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
// file size -- figure out the size of the file 
//
int fsize(const char *filename) {
    struct stat st; 

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1; 
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


// *************************************************
// load file into  buffer
//  tests if it is a ascii or binary image file
//
//  2015/12/18 created
//
int load_file(FILE *fp, char *fname, uint8_t buffer[])
{
int filesize;                   // size of the file we are trying to load
int loadsize;                   // number of byte we will actually load into memory
int  c0, c1, c2;
int  i;
int byte_cnt=0;
int  asciimode;

    filesize = fsize(fname);
    if (filesize == -1) {
        fputs("Error: cannot reading file", stderr);
        exit(1);
    }

    if (filesize == 0) {
        fputs("Error: file is empty", stderr);
        exit(1);
    }
    
    if(filesize > MAX_FILE_SIZE) {
        fputs("Error: file is too big", stderr);
        exit(1);
    }

// track zero should alway start with loadhi=0x22 loadlow=0x00 page count=0x08
// ... so load first 3 chars from file
// if chars are string "22 " then this is ascii image
// if chars are 0x22 0x00 0x08 then this is binary image
    c0=fgetc(fp);
    c1=fgetc(fp);
    c2=fgetc(fp);
    if(c0=='2' && c1=='2' && c2==' ') {
         asciimode=TRUE;
         buffer[byte_cnt++]= hexbin(c0, c1);
    }
    if(c0==0x22 && c1==0x00 && c2==0x08) {
        asciimode=FALSE;
        buffer[byte_cnt++]=c0;
        buffer[byte_cnt++]=c1;
        buffer[byte_cnt++]=c2;
    }

    if(asciimode) {
        while((c0 = fgetc(fp)) != EOF) {
            // skip over whitespace and if no hex digit (wtf??)
            if(!isxdigit(c0) || isspace(c0)) continue;
            // need to check we are not at EOF
            c1=fgetc(fp);
            if(!isxdigit(c1) || isspace(c1)) continue;
            buffer[byte_cnt++] = hexbin(c0,c1); 
        }
        fclose (fp);
    } else { // binary image
        while((c0 = fgetc(fp)) != EOF) {
            buffer[byte_cnt++] = c0; 
        }
        fclose (fp);
    }

    return byte_cnt;
}

// extra code
//     while ( fgets ( line, sizeof line, fp ) != NULL ) {
//         chomp(line);
//         printf("<%s>\n",  line);
//       }


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
