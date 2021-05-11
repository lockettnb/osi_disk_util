/*
*
* 2015/12/28 created 
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

#define TEST_LIMIT 256 

//--------------------------------------------------------
// scan for assembler instructions
int asm_instruction_scan(uint8_t buff[], int bsize)
{
int i;
int j=0;
int opcount=0;
char opvalue[4];
char *codes[] = {   // a few popular instructions 
            "LDA", "STA", "DEC", "INC", "AND", "CMP", "SBC",
            "LDX", "STX", "DEX", "INX", "TSX",  
            "LDY", "STY", "DEY", "INY",
            "BEQ", "BNE", "BCC", "BCS", "BPL", "BMI",
            "JSR", "JMP",
            NULL
};

    debug_print("isasm: starting scan for assembler instructions %i\n", bsize);
    for(i=0; i<bsize-3; i++) {
       opvalue[0]=buff[i];
       opvalue[1]=buff[i+1];
       opvalue[2]=buff[i+2];
       opvalue[3]='\0';

       j=0;
       while (codes[j] != NULL) {
//              ddebug_print("isasm: %i %i compare %s to %s \n", i,j,codes[j], opvalue);
            if(strcmp(codes[j++], opvalue) == 0) {
                 ddebug_print("isasm: found assembler opcode %s\n", opvalue);
                opcount++;
            }
        }
    } 

    return opcount;
} // op instruction scan

//--------------------------------------------------------
// scan for specific 6502 op codes
int machinecode(uint8_t buff[], int bsize)
{
int opcount=0;
int i;

    for(i=0; i<bsize-2; i++) {
        // look for "LDA #nn" followed by "STA nn" or "STA n"
        if( (buff[i] == 0xa9) && ((buff[i+2]==0x8d) || (buff[i+2]==0x85)) ) {
            opcount++;
            ddebug_print("ismachine: LDA/STA instruction codes found\n");
        }
        // look for "LDA n" followed by "STA nn" or "STA n"
        if( (buff[i] == 0xa5) && ((buff[i+2]==0x8d) || (buff[i+2]==0x85)) ) {
            opcount++;
            ddebug_print("ismachine: LDA/STA instruction codes found\n");
        }
    }

    return opcount;
} 

//--------------------------------------------------------
// ismachine -- test if track is 6502 machine code 
//
int ismachine(uint8_t disk[], struct index_t index[], int track, int sector)
{
int dh=0;
int bsize;
int control, printable, highbit, op;

uint8_t buffer[TRACKSIZE];

    control=printable=highbit=op=0;
    memset(buffer, 0xeb, sizeof(buffer));   // empty marker, illegal instruction

    if(index[track].sector[1]==0) {
        debug_print("ismachine: no sectors on track %i, not going to test for assembler\n", track);
        return FAIL;
    }
        
    if( loadsector(disk, index, buffer, track, sector) == FAIL) {
        fprintf(stderr, "%s: Error trying to load basic program on track %i\n",\
                program_name, track);
        exit(1);
    }
    bsize = index[track].pages[1]*256-1;
    if(debug) hex(buffer, 0, 0, 32);

    while(dh<=TEST_LIMIT) {
        if(isprint(buffer[dh])) printable++;
        if(buffer[dh] <= 0x1f) control++;
        if(buffer[dh] >= 0x80) highbit++; 
        ddebug_print("ismachine: %i %04x control=%i printable=%i highchars=%i\n",\
                    dh, buffer[dh], control, printable, highbit);
        dh++;
    }
    debug_print("ismachine: control=%i printable=%i highchars=%i\n", control, printable, highbit);

    op = machinecode(buffer, bsize);
    debug_print("ismachine: opcode scan = %i\n", op);
   
    if((control > 0) && (printable > 0 ) && (highbit > 0) && (op > 0)) 
        return SUCCESS;

    debug_print("ismachine: not Machine Code %i\n", track);
    return FAIL;
}

//--------------------------------------------------------
// isasm -- test if track is 6502 assembler source code
//
int isasm(uint8_t disk[], struct index_t index[], int track, int sector)
{
int dh=0;
int bsize;
int control, printable, highbit, op;

uint8_t buffer[TRACKSIZE];

    control=printable=highbit=op=0;
    memset(buffer, 0xeb, sizeof(buffer));   // empty marker, illegal instruction

    if(index[track].sector[1]==0) {
        debug_print("isasm: no sectors on track %i, not going to test for assembler\n", track);
        return FAIL;
    }
        
    if( loadsector(disk, index, buffer, track, sector) == FAIL) {
        fprintf(stderr, "%s: Error trying to load basic program on track %i\n",\
                program_name, track);
        exit(1);
    }
    bsize = index[track].pages[1]*256-1;
    if(debug) hex(buffer, 0, 0, 32);

    dh=dh+7;      // skip header and line number
    while(dh<=TEST_LIMIT) {
        if(buffer[dh] == 0x0d) {
            ddebug_print("isasm: found EOL\n");
            dh=dh+3;
            continue;
        }
        if(buffer[dh] >= 0xe0) {
            dh++;
            continue;    // seem to be tabs
        }
        if(isprint(buffer[dh])) printable++;
        if(buffer[dh] <= 0x1f) control++;
        if(buffer[dh] >= 0x80) highbit++; 
        ddebug_print("isasm: %i %04x control=%i printable=%i highchars=%i\n", dh, buffer[dh], control, printable, highbit);
        dh++;
    }
    debug_print("isasm: control=%i printable=%i highchars=%i\n", control, printable, highbit);

    op = asm_instruction_scan(buffer, bsize);   
    debug_print("isasm: opcode scan = %i\n", op);

    if((control < 4) && (printable > TEST_LIMIT/2 ) && (highbit < 4) && (op > 10)) 
        return SUCCESS;

    debug_print("isasm: not Assember Code %i\n", track);
    return FAIL;
}



//--------------------------------------------------------
// isbasic -- test if track is basic source code
//
int isbasic(uint8_t disk[], struct index_t index[], int track, int sector) {
int dh=0;
int i;
int control, printable, highbit;

uint8_t buffer[TRACKSIZE];

    control=printable=highbit=0;
    memset(buffer, 0xeb, sizeof(buffer));   // empty marker, illegal instruction

    if(index[track].sector[1]==0) {
        debug_print("isbasic: no sectors on track %i, not going to test for basic\n", track);
        return FAIL;
    }
        
    if( loadsector(disk, index, buffer, track, sector) == FAIL) {
        fprintf(stderr, "%s: Error trying to load basic program on track %i\n",\
                program_name, track);
        exit(1);
    }
    if(debug) hex(buffer, 0, 0, 32);

    // scan for a null, marks end-of-line
    for (i=0; i<=TEST_LIMIT; i++) {
        if(buffer[dh+i] == 0x00) break;
    }
    if(i>=TEST_LIMIT) {  // not null found, this is not basic
        debug_print("isbasic: could not find first null, not basic code\n");
        return FAIL;
    }

    dh=dh+i+5;      // skip next line pointer and line number
    while(dh<=TEST_LIMIT) {
        if(buffer[dh] == 0x00) {
            ddebug_print("isbasic: found EOL\n");
            dh=dh+5;
            continue;
        }
        if(isprint(buffer[dh])) printable++;
        if(buffer[dh] <= 0x1f) control++;
        if(buffer[dh] >= 0x80) highbit++; 
        ddebug_print("isbasic: %i %04x control=%i printable=%i highchars=%i\n", dh, buffer[dh], control, printable, highbit);
        dh++;
    }
    debug_print("isbasic: control=%i printable=%i highchars=%i\n", control, printable, highbit);
    
    if((control == 0) && (printable > TEST_LIMIT/3) && (highbit > TEST_LIMIT/50)) 
        return SUCCESS;

    debug_print("isbasic: not Basic Code %i\n", track);
    return FAIL;
}

//-------------------------------------------------------------------
void hex_print(uint8_t disk[], struct index_t index[], int track)
{
int i;
int content_found=FALSE;
int sect;
int pages;
char ctype[32];
uint8_t buffer[256*15];     // OS65D is max 12 pages

    if(track==0) {
        printf("\nTrack %i:\n", track);
        hex(disk, 0, index[track].header, 8*256);
        return;
    }
        
    for(i=1; i<=6; i++) { 
       sect=index[track].sector[i];
       pages=index[track].pages[i];
       debug_print(">>print track seeking: t %i sector %i\n", track, i);
       if(sect != 0) {
           get_content_type(disk, index, track, i, ctype);
           printf("\nTrack%i:  Sector%i: Pages: 0x%02x (%i)  %s\n", track, i , pages, pages, ctype); 
           loadsector(disk, index, buffer, track, i);
           hex(buffer, 0x3179, 0, pages*256);
           content_found=TRUE;
        }

    }
    if(content_found) 
        printf("\n");
    else {
        printf("\nTrack %i: empty\n", track);
        hex(disk, 0, index[track].header, 64);
    }

}  // print track



// *************************************************
// print basic token
//  2015/12/28 created
//
void token_print(int tok)
{
int i=0;

struct basic_tokens token[] = {
    {"END", 0x80},
    {"FOR", 0x81},
    {"NEXT", 0x82},
    {"DATA", 0x83},
    {"INPUT", 0x84},
    {"DIM", 0x85},
    {"READ", 0x86},
    {"LET", 0x87},
    {"GOTO", 0x88},
    {"RUN", 0x89},
    {"IF", 0x8a},
    {"RESTORE", 0x8b},
    {"GOSUB", 0x8c},
    {"RETURN", 0x8d},
    {"REM", 0x8e},
    {"STOP", 0x8f},

    {"ON", 0x90},
    {"NULL", 0x91},
    {"WAIT", 0x92},
    {"LOAD", 0x93},
    {"SAVE", 0x94},
    {"DEF", 0x95},
    {"POKE", 0x96},
    {"PRINT", 0x97},
    {"CONT", 0x98},
    {"LIST", 0x99},
    {"CLEAR", 0x9a},
    {"NEW", 0x9b},
    {"TAB(", 0x9c},
    {"TO", 0x9d},
    {"FN", 0x9e},
    {"SPC(", 0x9f},

    {"THEN", 0xa0},
    {"NOT", 0xa1},
    {"STEP", 0xa2},
    {"+", 0xa3},
    {"-", 0xa4},
    {"*", 0xa5},
    {"/", 0xa6},
    {"~", 0xa7},
    {"AND", 0xa8},
    {"OR", 0xa9},
    {">", 0xaa},
    {"=", 0xab},
    {"<", 0xac},
    {"SGN", 0xad},
    {"INT", 0xae},
    {"ABS", 0xaf},

    {"USR", 0xb0},
    {"FRE", 0xb1},
    {"POS", 0xb2},
    {"SQR", 0xb3},
    {"RND", 0xb4},
    {"LOG", 0xb5},
    {"EXP", 0xb6},
    {"COS", 0xb7},
    {"SIN", 0xb8},
    {"TAN", 0xb9},
    {"ATN", 0xba},
    {"PEEK", 0xbb},
    {"LEN", 0xbc},
    {"STR$", 0xbd},
    {"VAL", 0xbe},
    {"ASC", 0xbf},

    {"CHR$", 0xc0},
    {"LEFT$", 0xc1},
    {"RIGHT$", 0xc2},
    {"MID$", 0xc3},
    {NULL, 0}
};

    while(token[i].name != NULL) {
        if(token[i].token == tok) {
            printf("%s", token[i].name);
            return;
        }
        i++;
    }

    printf("<%02x> ", tok);

} // print token

// -------------------------------------------------------------
// Print Track content as BASIC source code
//
void basic_print( uint8_t disk[], struct index_t index[], int track )
{
int done=FALSE;
int dh=0;
int begin_addr;
int end_addr;
int ntracks;
int lineno;
int pointer;
int load_addr = OS65D_BUFFER; 
uint8_t buffer[FULL_MEMORY];
int starting;                   // boolean 


    memset(buffer, 0xeb, sizeof(buffer));   // empty marker, illegal instruction

    if( loadmemory(disk, index, buffer, track) == FAIL) {
        fprintf(stderr, "%s: Error trying to load basic program on track %i\n",\
                program_name, track);
        exit(1);
    }

    begin_addr= buffer[load_addr] | (buffer[load_addr+1]<<8);
    end_addr = buffer[load_addr+2] | (buffer[load_addr+3]<<8);
    ntracks = buffer[load_addr+4];
    debug_print(">>print basic: begin=0x%04x end=0x%04x tracks=%i\n", begin_addr, end_addr, ntracks);
    if(debug) hex(buffer, 0, begin_addr, 64);
    debug_print(">>print basic: Number of Tracks:%i\n", ntracks);
    printf("<%04x>  Start Address", begin_addr);

    dh=begin_addr-1;                         // start of basic should be pointer
    starting=TRUE;

    while (!done) {
        if((buffer[dh] == 0) || starting) {
            starting=FALSE;
            pointer=buffer[dh+1] | (buffer[dh+2]<<8);     // pointer to next line
            lineno =buffer[dh+3] | ( buffer[dh+4]<<8);    // line number
            dh=dh+5;                                      // start of text or token
            if(pointer == 0) {                            // null point we are done
                done=TRUE;
                printf("\n");
                continue;
            }
            printf("\n<%04x> %8i ", pointer, lineno);
            continue;
        }
        if(isprint(buffer[dh]))                          // printable text
            printf("%c", buffer[dh]);

        if((buffer[dh] >= 0x80) && (buffer[dh] <= 0xc3))                          // token
            token_print(buffer[dh]);
    
        if(buffer[dh] < 0x20)                           // illegal char
            printf("\%02x", buffer[dh]);
        dh++;
    }
    printf("<%04x>  End Address", end_addr);
    printf("\n");
}

// -------------------------------------------------------------
// Print Track content as Assembler source code
//
void asm_print( uint8_t disk[], struct index_t index[], int track )
{
int done=FALSE;
int dh=0;
int start_addr;
int end_addr;
int ntracks;
int lineno;
uint8_t buffer[TRACKSIZE];
int repeat;
int i;

    memset(buffer, 0xeb, sizeof(buffer));   // empty marker, illegal instruction

    if( loadsector(disk, index, buffer, track, 1) == FAIL) {
        fprintf(stderr, "%s: Error trying to load basic program on track %i\n",\
                program_name, track);
        exit(1);
    }

    if(verbose) hex(buffer, 0, 0, 64);
    start_addr = buffer[0] | (buffer[1]<<8);
    end_addr = (buffer[2] | (buffer[3])<<8);
    ntracks = buffer[4];
    printf("Tracks:%i\n", ntracks);
    printf("<%04x>  Start Address\n", start_addr);

    dh=5;                   // start of assembler, should be first line number 
    lineno=buffer[dh] | ( buffer[dh+1]<<8);    // line number
    printf("%8i ",lineno);
    dh=dh+2;
    
    while (!done) {
        if(buffer[dh]==0) {
            done=TRUE;
            printf("\n");
            ddebug_print("asmprint: done %i \n", dh);
            continue;
        }
        if(buffer[dh] == 0x0d) {
            ddebug_print("<cr> %i \n", dh);
            lineno=buffer[dh+1] | ( buffer[dh+2]<<8);    // line number
            dh=dh+3;                                      // start of text or token
            printf("\n%8i ", lineno);
            continue;
        }
        if(isprint(buffer[dh]))                          // printable text
            printf("%c", buffer[dh]);

        if(buffer[dh] >= 0x80) {                         // repeat pervious char 
            repeat = ~(0xffffff00 | buffer[dh])+1;
            ddebug_print("\nasmprint: 0x%02x repeat %i ", buffer[dh], repeat);
            for(i=0; i<repeat; i++) printf("%c", buffer[dh-1]);
        }
    
        if(buffer[dh] < 0x20)                           // illegal char
            printf("\%02x", buffer[dh]);
        dh++;
    }
    printf("<%04x>  End Address", end_addr);
    printf("\n");
}

// -------------------------------------------------------------
// Print entire track content as hex including headers/footers
//
void raw_print(uint8_t disk[], struct index_t index[], int track)
{
int i;
int sect;
int pages;
int binary_save;            // this is really stupid, but its early morning
int ccount=0;               // char counter 
int ss, se;                 // sector start to sector end 
int dh;                     // virtual disk head

    binary_save=binary;     // save current binary option setting
    binary=FALSE;           // ..... and force ascii output in cout funtion

    // TRACK ZERO
    if(track ==0) {
        // write header
       colour(GREEN);
        cout(stdout, disk[0], &ccount);
        cout(stdout, disk[1], &ccount);
        cout(stdout, disk[2], &ccount);
        colour(BACKGND);

        ss=3;
        se=ss+disk[2]*256-1;
        for(dh=ss; dh<=se; dh++) {      // write track zero data
            cout(stdout, disk[dh], &ccount);
        }
        binary=binary_save;
        printf("\n");
        return;
    }
       
    if(index[track].start != index[track].header) {
        colour(RED);
        for(dh=index[track].start; dh<index[track].header; dh++) {
            cout(stdout, disk[dh],&ccount);
        } 
        colour(BACKGND);
    }

    // header
    colour(GREEN);
    dh=index[track].header; 
    cout(stdout, disk[dh++],&ccount);
    cout(stdout, disk[dh++],&ccount);
    cout(stdout, disk[dh++],&ccount);
    cout(stdout, disk[dh++],&ccount);
    colour(BACKGND);

    // sectors
    for(i=1; i<=6; i++) { 
       sect=index[track].sector[i];
       pages=index[track].pages[i];
        while(dh < sect) {          // highlight any filler bytes
            colour(RED);
            cout(stdout, disk[dh++],&ccount);
            colour(BACKGND);
        } 
//         debug_print(">>dh 0x%06x sect:0x%06x pages:0x%06x (%i)\n", dh, sect, pages, pages);
       if(sect != 0) {
            colour(YELLOW);
            cout(stdout, disk[dh++], &ccount);
            cout(stdout, disk[dh++], &ccount);
            cout(stdout, disk[dh++], &ccount);
            colour(BACKGND);
            for(dh=sect+3; dh<sect+3+256*pages; dh++) {
                cout(stdout, disk[dh], &ccount);
            }
//          debug_print(">>dh 0x%06x sect:0x%06x pages:0x%06x (%i)\n", dh, sect, pages, pages);
            colour(CYAN);
            cout(stdout, disk[dh++], &ccount);
            cout(stdout, disk[dh++], &ccount);
            colour(BACKGND);
            
        }

    }
    printf("\n");
    binary=binary_save;
}  // print raw 
