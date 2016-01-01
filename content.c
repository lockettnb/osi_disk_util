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
// ISML -- test if track is 6502 machine code 
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
    if(debug) printhex(buffer, 0, 0, 32);

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
// ISASM -- test if track is 6502 assembler source code
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
    if(debug) printhex(buffer, 0, 0, 32);

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
// ISBASIC -- test if track is basic source code
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
    if(debug) printhex(buffer, 0, 0, 32);

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
int start_addr;
int end_addr;
int ntracks;
int lineno;
int pointer;
uint8_t buffer[TRACKSIZE];


    memset(buffer, 0xeb, sizeof(buffer));   // empty marker, illegal instruction

    if( loadsector(disk, index, buffer, track, 1) == FAIL) {
        fprintf(stderr, "%s: Error trying to load basic program on track %i\n",\
                program_name, track);
        exit(1);
    }

    printhex(buffer, 0, 0, 32);
    start_addr = buffer[0] | (buffer[1]<<8);
    end_addr = (buffer[2] | (buffer[3])<<8);
    ntracks = buffer[4];
    printf("Tracks:%i\n", ntracks);
    printf("<%04x>  Start Address", start_addr);
    dh=5;       // start of basic, should be a 0x00
    while (!done) {
        if(buffer[dh] == 0) {
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

        if(buffer[dh] >= 0x80)                          // token
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

    if(verbose) printhex(buffer, 0, 0, 64);
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

