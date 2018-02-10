/* 
*  dds -- disk dump scan 
*   disk dump scanner for OSI 8in floppy disk images 
*  
*
*   2015/12/17 created
*
*
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <string.h>

#include "ddscan.h"

// global variables
char *program_name;


// Option Flags
int examine = FALSE;
int directory = FALSE;
int content = FALSE;
int ascii   = FALSE;
int binary  = TRUE;
int nocolour= FALSE;
int verbose = FALSE;
int help    = FALSE;
int version = FALSE;
int hello = FALSE;
int debug = FALSE;

struct option long_options[] =
     {
       {"examine",  no_argument,         0, 'e'},
       {"track",    required_argument,   0, 't'},

       {"directory", no_argument,       0, 'd'},

       {"output",   required_argument,  0, 'o'},
       {"ascii",    no_argument,        0, 'a'},
       {"binary",   no_argument,        0, 'b'},

       {"content",  no_argument,        0, 'c'},
       {"list",    required_argument,   0, 'l'},

       {"verbose",  no_argument,        0, 'v'},
       {"nocolor",  no_argument, &nocolour,    TRUE},
       {"nocolour", no_argument, &nocolour,    TRUE},
       {"help",     no_argument, &help,    TRUE},
       {"hello",    no_argument, &hello,   TRUE},
       {"debug",    no_argument, &debug,   TRUE},
       {"version",  no_argument, &version, TRUE},
       {NULL, 0, NULL, 0}
     };

char *instructions[] = {
    "  osidd - OSI Disk Dump",
    "    +scan OSI disk image for valid tracks/sector format",
    "    +convert disk image from ascii to binary",
    "    +display directory information",
    "    +display content from tracks",
    " ",
    " ",
    " Usage osidd [options] FILE ",
    "   -e   --examine      : examine all tracks for valid headers/sectors",
    "   -t   --track  n     : track to examine, can be a range 0-4 or all (default)",
    " ",
    "   -d   --directory    : display directory listing from disk image",
    " ",
    "   -o   --output fname : write image to output file",
    "   -a   --ascii        : write disk image in ascii hex ",
    "   -b   --binary       : write disk image in binary (default output format)",
    " ",
    "   -c   --content      : display content of track/sector ",
    "   -l   --list type    : list track as basic, asm, hex or raw",
    "        --nocolour     : turn off colour highlight for raw output",
    " ",
    "   -v,  --verbose      : enable verbose output",
    "        --help         : display help and exit",
    "        --version      : print version information and exit",
    " ",
    " When displaying content the application will guess the format type.",
    " The format will be one of Basic, Assembler, Data (hex).",
    " This guess can be overridden by the list option. Overriding the with ",
    " the wrong format can icreate some very messy output.",
    " ",
    " The raw format will display the entire track including format bytes with",
    " colour highlights; green=track header, yellow=sector header, ",
    " cyan=sector footer, red=filler bytes that are not part of the OS65D format",
    " ",
    "                                                     john 2016/01",
    NULL_CHAR
    };



// *****************************************************
int main (int argc, char **argv)
{
int optc, option_index;     // option processing
char xfrom[16];             // track option processing 
char xto[16];               // track option processing 
char *token;                // track option processing 
const char delm[2] = "-";   // track option processing

char in_track[MAXOPSIZE];   // track number from options (set default to all)
char ofile[MAXOPSIZE];      // output file name from options
char ltype[MAXOPSIZE];      // list type from options

int ctype=HEX;              // content type default it to data(hex) 
int fromtk, totk;           // track range from...to

uint8_t disk[FULL_DISK];    // buffer for disk data
struct  index_t index[77];  // track/sector/page index built during load process
struct dir_t dir[64];       // directory data read during disk image load
int disksize;               // total number of bytes in disk image

// misc general variables
int i = 0;                  // index
int dirty=FALSE;            // used if no options provided to default something 


program_name=argv[0];
if (help || version) inst(instructions,0);

// some defaults
strcpy(in_track, "all");    // default all tracks
ofile[1]='\0';              // default write file is none

while ((optc=getopt_long(argc, argv, "et:do:abcl:v", long_options, &option_index)) != -1) {
switch (optc) {
    case 0:
        // getop returns zero for long options with no short values
        // in our case help and verbose get set to TRUE so nothing else to do
        if (long_options[option_index].flag != 0) break;
        // you could process other long options here 
        // option name     = long_options[option_index].name
        // option argument = optarg
        fprintf(stderr, "wtf... unknown long option \n");
        inst(instructions, 1);
        break;

    case 'e':
        examine=TRUE;
        break;

    case 't':
        get_optvalue(in_track, optarg, MAXOPSIZE-1);
        break;

    case 'd':
        directory = TRUE;
        break;

    case 'o':
        get_optvalue(ofile,optarg, MAXOPSIZE-1);
        break;

    case 'a':
        ascii=TRUE;
        binary=FALSE;
        break;

    case 'b':
        binary=TRUE;
        ascii=FALSE;
        break;

    case 'c':
        content=TRUE;
        break;

    case 'l':
        get_optvalue(ltype,optarg, MAXOPSIZE-1);
        break;

    case 'v':
        verbose = TRUE;
        break;

    case '?':
        /* `getopt_long' already printed an error message. */
        inst(instructions, 1);
        break;

    default:
        fprintf(stderr,"%s: illegal option, try --help\n", program_name);
        exit(1);
    } // switch options
} // while options

// post option processing
if (help || version) inst(instructions,0);

strcpy(xfrom, "0");
strcpy(xto, "0");

// figure out the from-to track numbers
if ((strcmp(in_track, "all") ==0)) 
    fromtk=totk=99;
else {
        token = strtok(in_track, delm);
        if(token != NULL) strcpy(xfrom, token);
        else strcpy(xfrom, "1");

        token = strtok(NULL, delm);
        if(token != NULL) strcpy(xto, token);
        else strcpy(xto, "0");

        fromtk = atoi(xfrom);
        totk = atoi(xto);
        // debug_print(">>Tracks: xfrom=%s xto=%s\n", xfrom, xto);
        if(fromtk >76) fromtk=76;
        if(totk >76) totk=76;
        if(fromtk > totk) totk=fromtk;
    }
debug_print(">>Tracks: from=%i to=%i\n", fromtk, totk);

lower(ltype);
if(strcmp(ltype, "basic") == 0) ctype = BAS;
if(strcmp(ltype, "asm") == 0)   ctype = ASM;
if(strcmp(ltype, "text") == 0)  ctype = TXT;
if(strcmp(ltype, "hex") == 0)   ctype = HEX;
if(strcmp(ltype, "raw") == 0)   ctype = RAW;

if ((argc-optind) == 0) {                       // no arguments
    fprintf(stderr, "%s: No disk image file to process\n", program_name);
    exit(1);
}

// ok, finally done processing options and stuff load the disk image
disksize=load_image(argv[optind], disk, index);     //process each file
load_directory(disk, index, dir);

verbose_print("Disk Image: %i bytes (0x%06x)\n\n", disksize, disksize);

if(examine) {
  if(totk==99) 
       for(i=0; i<=76; i++) {
        examine_track(disk, disksize, index, i);
       }
   else {
        for(i=fromtk; i<=totk; i++) {
            examine_track(disk,disksize, index, i);
        }
    }
    dirty=TRUE;
}

if(content) {
  if(totk==99) 
       for(i=0; i<=76; i++) { // fix this, just set from/to
        if(ctype == HEX) hex_print(disk, index, i);
        if(ctype == BAS) basic_print(disk, index, i);
       }
   else {
        for(i=fromtk; i<=totk; i++) {
            if(ctype == HEX) hex_print(disk,index, i);
            if(ctype == BAS) basic_print(disk, index, i);
            if(ctype == ASM) asm_print(disk, index, i);
            if(ctype == RAW) raw_print(disk, index, i);
        }
    }
    dirty=TRUE;
}

if( (ofile[1] != '\0')) {
     write_image(disk, index, ofile);
     dirty=TRUE;
}

if(directory || !dirty) print_directory(dir);

exit(SUCCESS);
}   /* main */

