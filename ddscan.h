/*
*
*   2015/12/17 created
*/

#define VERSION "0.1"
#define NULL_CHAR '\0'
#define TRUE 1
#define FALSE 0
#define SUCCESS 0
#define FAIL -1
#define MAXOPSIZE 32

#define verbose_print(...)  do { if (verbose) printf(__VA_ARGS__ ); } while (0)
#define debug_print(...)  do { if (debug) printf(__VA_ARGS__ ); } while (0)
#define ddebug_print(...)  do { if (hello) printf(__VA_ARGS__ ); } while (0)


// the disk image is ASCII chars in rows of 16 bytes
//  + three chars per byte plus CR & LF
//  + 240 rows per sector
//  + 77 tracks
// .....row example 22 00 08 A9 01 20 B6 22 20 BC 26 A9 2A 85 FF 20
#define MAX_ASCII_SIZE ((16*3)+2)*240*77

// 0-76 tracks or 16 pages 
#define TRACKSIZE 4096
#define TRACKS 77
#define DIRTRACK 8
#define FULL_DISK TRACKS*TRACKSIZE

// content display formats
#define BAS 1
#define ASM 2
#define TXT 3
#define HEX 4

// Global Option Flags 
// extern int examine;
// extern int directory;
// extern int list;
extern int ascii;
extern int binary;
extern int content;
extern int verbose;
extern int help;
extern int version;
extern int hello;
extern int debug;

extern char *program_name;

struct index_t {   // 77 tracks 0..76
    int start;     // start of track in disk buffer
    int header;    // start of header, usually the same as start but not always
    int sector[7]; // sector locations using 1,2,3,4,5,6 (0-zero is not used) 
    int pages[7];  // sector page sizes
    int end;       // end of track data, may be some filler bytes beyond end
};

struct dir_t {        // max 64 directory entries
    char name[7];     // directory entry name 6 chars long
    int  start;       // start track
    int  end;         // end track
};


// Basic Tokens
struct basic_tokens {
    char *name;
    int token;
};


// OSI Utilities
void write_image(uint8_t disk[], struct index_t index[], char *fname);
int load_image(char *fname, uint8_t disk[], struct index_t index[], struct dir_t dir[]);
void examine_track(uint8_t disk[], int disksize, struct index_t index[], int track);
void print_track(uint8_t disk[], struct index_t index[], int track);
void print_directory(uint8_t disk[], struct index_t index[]);
int loadsector(uint8_t disk[], struct index_t index[], uint8_t buffer[], int sk_track, int sk_sector);
int seek_track(uint8_t disk[], int seek_tk);
int image_format(FILE *fp, int *asciiformat);

// Utility Functions
void chomp(char *s);
void lower(char s[]);
int fsize(const char *filename);
int bcdtobin(uint8_t bcd);
void get_optvalue(char *dest, char *optvalue, int max);
int hexbin(int hi, int low);
void printhex(uint8_t b[], int offset, int addr, int count);
void inst(char *iptr[], int status);

// Content Analysis Functions
int ismachine(uint8_t disk[], struct index_t index[], int track, int sector);
int isasm(uint8_t disk[], struct index_t index[], int track, int sector);
int isbasic(uint8_t disk[], struct index_t index[], int track, int sector);
void basic_print( uint8_t disk[], struct index_t index[], int track );
void asm_print( uint8_t disk[], struct index_t index[], int track );
// void token_print(int tok);
