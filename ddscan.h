/*
*
*   2015/12/17 created
*/

#define VERSION "0.01"
#define NULL_CHAR '\0'
#define TRUE 1
#define FALSE 0
#define SUCCESS 0
#define FAIL -1

#define verbose_print(...)  do { if (verbose) printf(__VA_ARGS__ ); } while (0)


// the disk image is ASCII chars in rows of 16 bytes
//  + three chars per byte plus CR & LF
//  + 240 rows per sector
//  + 77 tracks
// .....row example 22 00 08 A9 01 20 B6 22 20 BC 26 A9 2A 85 FF 20
#define MAX_FILE_SIZE ((16*3)+2)*240*77

// 0-76 tracks or 16 pages 
#define TRACKSIZE 4096
#define TRACKS 77
#define FULL_DISK TRACKS*TRACKSIZE

/* Option Flags set by `--options' */
extern int examine;
extern int ascii;
extern int binary;
extern int verbose;
extern int help;
extern int version;
extern int debug;

extern char *program_name;
// extern char *instructions[];

// print help and instructions
void inst(char *iptr[], int status);

// load the binary file into the memory buffer
int load_file(FILE *fp, char *fname, uint8_t buffer[FULL_DISK]);

// print buffer out as hex dump
void printhex(uint8_t b[], int addr, int count);

// trimwhite space from front/back of a command line
char *trimwhite (char *string);

// remove control chars (cr/lf) from end of string
void chomp(char *s);

// OSI Utilities
void scandisk(uint8_t disk[], int disksize);
