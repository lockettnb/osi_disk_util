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
int content=FALSE;
int ascii   = FALSE;
int binary  = TRUE;
int verbose = FALSE;
int help    = FALSE;
int version = FALSE;
int hello = FALSE;
int debug = FALSE;

struct option long_options[] =
     {
       {"examine",  no_argument,        0, 'e'},
       {"track",    required_argument,   0, 't'},
       {"sector",   required_argument,   0, 's'},

       {"directory", no_argument,       0, 'd'},

       {"output",   required_argument,  0, 'o'},
       {"ascii",    no_argument,        0, 'a'},
       {"binary",   no_argument,        0, 'b'},

       {"content",  no_argument,        0, 'c'},
       {"force",    required_argument,  0, 'f'},

       {"verbose",  no_argument,        0, 'v'},
       {"help",     no_argument, &help,    TRUE},
       {"hello",     no_argument, &hello,   TRUE},
       {"debug",     no_argument, &debug,   TRUE},
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
    "   -t   --track  n     : track to examine, can be a range 0-4 or all (default=all)",
    "   -s   --sector n     : sector to examine (default=sector 1)",
    " ",
    "   -d   --directory    : display directory listing from disk image",
    " ",
    "   -o   --output fname : write image to output file",
    "   -a   --ascii        : write disk image in ascii hex ",
    "   -b   --binary       : write disk image in binary (default output format)",
    " ",
    "   -c   --content      : display content of track/sector ",
    "   -f   --force type   : force display to basic, asm, text, or hex",

    " ",
    "   -v,  --verbose      : enable verbose output",
    "        --help         : display help and exit",
    "        --version      : print version information and exit",
    " ",
    " When displaying content the application will guess the format type.",
    " The format will be one of Basic, Assembler, Text or Data (hex).  ",
    " This guess can be overridden by the force option.",
    " ",
    "                                                     john 2015/12",
    NULL_CHAR
    };



// *****************************************************
int main (int argc, char **argv)
{
int optc, option_index;     // option processing
char in_track[MAXOPSIZE];   // track number from options (set default to all)
char in_sector[MAXOPSIZE];  // sector number from options (set default to 1)
char ofile[MAXOPSIZE];      // output file name from options
char ftype[MAXOPSIZE];      // force type from options
int track;                  // track to examine, set by option
int sector;                 // sector to examine, set by option
int ctype=HEX;              // content type default it to data(hex) 


int disksize;            // total number of bytes in disk image
uint8_t disk[FULL_DISK]; // buffer for disk data
struct  index_t index[77];
struct dir_t dir[64];   

// misc general variables
FILE *fp;               // file name of input disk image
int i = 0;              // misc index

int from, to;          // tracks
char xfrom[16];
char xto[16];
char *token;
const char delm[2] = "-";

strcpy(in_track, "all");    // default all tracks
ofile[1]='\0';              // default write file is none
program_name=argv[0];
if (help || version) inst(instructions,0);

while ((optc=getopt_long(argc, argv, "et:s:dlo:abcf:dv", long_options, &option_index)) != -1) {
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

    case 's':
        get_optvalue(in_sector,optarg , MAXOPSIZE-1);
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

    case 'f':
        get_optvalue(ftype,optarg, MAXOPSIZE-1);
        break;

    case 'v':
        verbose = TRUE;
        break;

    case '?':
        /* `getopt_long' already printed an error message. */
        inst(instructions, 1);
        break;

    default:
        abort ();
    } // switch options
} // while options

// post option processing
if (help || version) inst(instructions,0);

strcpy(xfrom, "0");
strcpy(xto, "0");

// figure out the from-to track numbers
if ((strcmp(in_track, "all") ==0)) 
    from=to=99;
else {
        token = strtok(in_track, delm);
        if(token != NULL) strcpy(xfrom, token);
        else strcpy(xfrom, "1");

        token = strtok(NULL, delm);
        if(token != NULL) strcpy(xto, token);
        else strcpy(xto, "0");

        from = atoi(xfrom);
        to = atoi(xto);
        // debug_print(">>Tracks: xfrom=%s xto=%s\n", xfrom, xto);
        if(from >76) from=76;
        if(to >76) to=76;
        if(from > to) to=from;
    }
debug_print(">>Tracks: from=%i to=%i\n", from, to);

sector = atoi(in_sector);

lower(ftype);
if(strcmp(ftype, "basic") == 0) ctype = BAS;
if(strcmp(ftype, "asm") == 0)   ctype = ASM;
if(strcmp(ftype, "text") == 0)  ctype = TXT;
if(strcmp(ftype, "hex") == 0)   ctype = HEX;

if ((argc-optind) == 0) {                       // no arguments
    fprintf(stderr, "%s: No disk image file to process\n", program_name);
    exit(1);
}

// ok, finally done processing options and stuff load the disk image
disksize=load_disk_image(argv[optind], disk, index, dir);     //process each file
verbose_print("Disk Image: %i bytes (0x%06x)\n\n", disksize, disksize);

if(examine) {
  if(to==99) 
       for(i=0; i<=76; i++) {
        print_track(disk, disksize, index, i);
       }
   else {
        for(i=from; i<=to; i++) {
            print_track(disk,disksize, index, i);
        }
    }
}

if(content) {
  if(to==99) 
       for(i=0; i<=76; i++) {
        if(ctype == HEX) show_track(disk, disksize, index, i);
        if(ctype == BAS) basic_print(disk, disksize, index, i);
       }
   else {
        for(i=from; i<=to; i++) {
            if(ctype == HEX) show_track(disk,disksize, index, i);
            if(ctype == BAS) basic_print(disk, disksize, index, i);
        }
    }
}

if(directory) print_directory(disk, disksize, index);
if( (ofile[1] != '\0')) write_image(disk, disksize, index, ofile);


exit(SUCCESS);
}   /* main */







