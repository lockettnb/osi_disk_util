/*
*
*   2015/12/17 created
*/

#define VERSION "0.1"
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

// 0-76 tracks of max 15 pages each 
#define TRACKSIZE 256*16
#define TRACKS 77
#define DIRTRACK 8
#define FULL_DISK TRACKS*TRACKSIZE

// C3A memory size (48k)
#define FULL_MEMORY 1024*48
#define OS65D_BUFFER 0x3179

// content display formats
#define BAS 1
#define ASM 2
#define TXT 3
#define HEX 4
#define RAW 5

// colour for ansi terminals
#define BACKGND 39
#define BLACK 30
#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 97

// Global Option Flags 
// extern int examine;
// extern int directory;
// extern int list;
extern int ascii;
extern int binary;
extern int content;
extern int nocolour;
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
int  load_image(char *fname, uint8_t disk[], long disk_size, struct index_t index[], long index_size);
void load_directory(uint8_t disk[], struct index_t index[], struct dir_t dir[], long dir_size);
void write_image(uint8_t disk[], struct index_t index[], char *fname);
void examine_track(uint8_t disk[], int disksize, struct index_t index[], int track);
void print_track(uint8_t disk[], struct index_t index[], int track);
void print_directory(struct dir_t dir[]);
int  loadsector(uint8_t disk[], struct index_t index[], uint8_t buffer[], int sk_track, int sk_sector);
int  seek_track(uint8_t disk[], int seek_tk);
int  image_format(FILE *fp, int *asciiformat);
int loadmemory(uint8_t disk[], struct index_t index[], uint8_t buffer[], int sk_track);
void get_content_type(uint8_t disk[], struct index_t index[], int track, int sector, char *type);
void cout(FILE *fp, uint8_t byte, int *count);

// Utility Functions
void colour(int c);
void chomp(char *s);
void lower(char *s);
int fsize(const char *filename);
int bcdtobin(uint8_t bcd);
void get_optvalue(char *dest, char *optvalue, int max);
int hexbin(int hi, int low);
void hex(uint8_t b[], int offset, int addr, int count);
void inst(char *iptr[], int status);

// Content Analysis Functions
int ismachine(uint8_t disk[], struct index_t index[], int track, int sector);
int isasm(uint8_t disk[], struct index_t index[], int track, int sector);
int isbasic(uint8_t disk[], struct index_t index[], int track, int sector);
void basic_print( uint8_t disk[], struct index_t index[], int track );
void asm_print( uint8_t disk[], struct index_t index[], int track );
void raw_print(uint8_t disk[], struct index_t index[], int track);
void hex_print(uint8_t disk[], struct index_t index[], int track);
