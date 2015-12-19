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

#include "ddscan.h"

// global variables
char *program_name;


// Option Flags set 
int examine = TRUE;
int ascii   = TRUE;
int binary  = FALSE;
int verbose = FALSE;
int help    = FALSE;
int version = FALSE;
int debug   = FALSE;

struct option long_options[] =
     {
       {"help",    no_argument, &help,    TRUE},
       {"version", no_argument, &version, TRUE},
       {"debug",   no_argument,       0, 'd'},
       {"verbose", no_argument,       0, 'v'},
       {"examine", no_argument,       0, 'x'},
       {"ascii", no_argument,         0, 'a'},
       {"binary", no_argument,        0, 'b'},
       {"output", required_argument,  0, 'o'},
       {NULL, 0, NULL, 0}
     };

char *instructions[] = {
    "  Disk Dump Scanner ",
    "    +scan OSI disk image for valid tracks/sector format",
    "    +convert disk image from ascii to binary",
    "    +convert disk image from binary to ascii",
    " ",
    " dds [options] FILE ",
    "   -x   --examine      : examine all tracks for valid headers/sectors",
    "   -a   --ascii        : write disk image in ascii ",
    "   -b   --binary       : write disk image in binary ",
    "   -o   --output fname : output file name",
    "   -v,  --verbose      : enable verbose output",
    "   -d,  --debug        : display program debug information",
    "        --help         : display help and exit",
    "        --version      : print version information and exit",
    " ",
    " ",
    " With no FILE, or when FILE is -, read standard input",
    "                                                     john 2015/12",
    NULL_CHAR
    };



// *****************************************************
int main (int argc, char **argv)
{
int optc, option_index;
FILE *infile;              // pointer to the input file stream
char *outfile;
int i = 0;                  // misc index
int total_bytes;            // total number of bytes in disk image
uint8_t disk[FULL_DISK];    // buffer for disk data

program_name=argv[0];
if (help || version) inst(instructions,0);
while ((optc=getopt_long(argc, argv, "xabo:dv", long_options, &option_index)) != -1)
{
switch (optc) {
    case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0) break;
        /* Process other long options here */
        /* option name     = long_options[option_index].name */
        /* option argument = optarg */
        /* In this case we don't have any, so quit with error */
        inst(instructions, 1);
        break;

    case 'x':
        examine=TRUE;
        puts ("option x\n");
        break;

    case 'a':
        ascii=TRUE;
        puts ("option a\n");
        break;

    case 'b':
        binary=TRUE;
        puts ("option b\n");
        break;

    case 'o':
        printf ("option -o with value `%s'\n", optarg);
        outfile=optarg;
        break;

    case 'd':
        debug = TRUE;
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
    }
} // while options
if (help || version) inst(instructions,0);

if ((argc-optind) == 0) {                       // no arguments; process stdin
    total_bytes=load_file(stdin, "stdin", disk);
}
else {
    if(strcmp("-", argv[optind]) == 0)          // process dash as stdin 
        total_bytes=load_file(stdin, "stdin", disk);
    else {                                      // open file 
        if((infile=fopen(argv[optind], "r")) == NULL) {
            fprintf(stderr, "%s: error opening file <%s>\n", \
                    program_name, argv[optind]);
            exit(FAIL);
        }
        total_bytes=load_file(infile, argv[optind], disk);     //process each file
    }
}

printf("disk size = %i (0x%06x)\n", total_bytes, total_bytes);
//     printhex(disk, 0, 3000);
scandisk(disk, total_bytes);

exit(SUCCESS);
}   /* main */







