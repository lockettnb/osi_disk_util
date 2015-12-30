//
// OSI Utilities to scan disk images
//
// 2015/12/19 created .... john
//

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ddscan.h"

//-------------------------------------------------------------------
// print hex ascii byte to file in rows of 16 bytes
//
void cout(FILE *fp, uint8_t byte, int *count) {

    if(binary) {
        fputc(byte, fp);
        (*count)++;
        return;
    }

    if((*count % 16) == 0) {
        fprintf(fp, "%02x", byte);
        (*count)++;
        return;
    }

    if((*count+1) % 16 == 0) {
       fprintf(fp, " %02x\n", byte);
       (*count)++;
       return; 
    }

   fprintf(fp, " %02x", byte);
   (*count)++;
}

//-------------------------------------------------------------------
void write_image(uint8_t disk[], int disksize, struct index_t index[], char *fname) {
FILE *fp;
int tk, tksize;
int tkcnt=0;                   // count of byte written to track
int i, s, ss, se;
const int tkmax=15*256;   // every track will be 15 pages long, null padded to fit

    if((fp=fopen(fname, "w")) == NULL) {
        fprintf(stderr, "%s: error opening file <%s>\n", program_name, fname);
        exit(FAIL);
    }

    // TRACK ZERO
    // write header
    cout(fp, disk[0], &tkcnt);
    cout(fp,  disk[1], &tkcnt);
    cout(fp,  disk[2], &tkcnt);
    ss=3;
    se=ss+disk[2]*256-1;

    for(i=ss; i<=se; i++) {      // write track zero data
        cout(fp, disk[i], &tkcnt);
        if(tkcnt >= tkmax) {
            debug_print(">>write_image: track over 15 pages\n");
            break;
        }
    }
    while(tkcnt < tkmax) {         // pad with NOP upto 15 pages
        cout(fp, 0xea, &tkcnt);
    }

    for(tk=1; tk<=76; tk++) {
        tkcnt=0;
        // write track header
        if(ascii) fprintf(fp, "\n");          // blank line between tracks
        for(i=index[tk].header; i<=index[tk].header+3; i++) {
            cout(fp, disk[i], &tkcnt);
        }
        // write each sector
        for(i=1; i<=6; i++) {
            if(index[tk].sector[i] !=0) {
                ss=index[tk].sector[i];
                se=ss+index[tk].pages[i]*256-1+3+2;
                for(s=ss; s<=se; s++) {
                    cout(fp, disk[s], &tkcnt);
                }
            }
        }
        while(tkcnt < tkmax) {         // pad with NOP upto 15 pages
            cout(fp, 0xea, &tkcnt);
        }
    }

} // write image

void get_content_type(uint8_t disk[], int disksize, struct index_t index[], int track, int sector, char *type) {

    strcpy(type , "Unknown");
    if(isbasic(disk, disksize, index, track, sector)==SUCCESS) {
        strcpy(type, "BASIC");
        return;
    }

    if(isasm(disk, disksize, index, track, sector)==SUCCESS) {
            strcpy(type, "ASSEMBLER");
            return;
        }

    if(ismachine(disk, disksize, index, track, sector)==SUCCESS) {
        strcpy(type, "MACHINE CODE");
        return;
    }
}

//-------------------------------------------------------------------
void show_track(uint8_t disk[], int disksize, struct index_t index[], int track) {
int i;
int content_found=FALSE;
int tk;
int sect;
int pages;
char ctype[32];
uint8_t sector_buf[256*15];     // OS65D is max 12 pages

    if(track ==0) {
        printf("\nTrack %i:\n", track);
        printhex(disk, index[track].header, 8*256);
        return;
    }
        
    for(i=1; i<=6; i++) { 
       sect=index[track].sector[i];
       pages=index[track].pages[i];
       debug_print(">>show track seeking: t %i sector %i\n", track, i);
       if(sect != 0) {
           get_content_type(disk, disksize, index, track, i, ctype);
           printf("\nTrack%i:  Sector%i: Pages: 0x%02x (%i)  %s\n", track, i , pages, pages, ctype); 
           loadsector(disk, disksize, index, sector_buf, track, i);
           printhex(sector_buf, 0, pages*256);
           content_found=TRUE;
        }

    }
    if(content_found) 
        printf("\n");
    else {
        printf("\nTrack %i: empty\n", track);
        printhex(disk, index[track].header, 64);
    }

}



// ------------------------------------------------------
// load file into  disk buffer
//  tests if it is a ascii or binary image file
//
//  2015/12/18 created
//  2015/12/21 added creation of index and dir structures
//
int load_disk_image(char *fname, uint8_t disk[], struct index_t index[], struct dir_t dir[])
{
FILE *fp;
int filesize;       // size of the file we are trying to load
int c0, c1, c2;     // first three chars of the disk image
int asciimode;      // disk image is ascii or binary
int dh=0;           // disk head location, number of bytes into the disk image 
int track=0;        // counter
int sector=1;       // counter
int disksize;
int i;
int tk;

    // track zero
    // review of track zero format:
    //     0  -load address high normally = 0x22
    //     1  -load address low  normally = 0x00
    //     2  -page count        normally = 0x08
    // read tracks 1 to 76
    // review of the track format:
    //     0,1 -2 byte start code $43, $57
    //     2  -track number in BCD
    //     3  -track type code (always $58)
    //      sector data
    //     1  -sector start code $76
    //     1  -sector number in binary
    //     1   -sector length (#pages) in binary
    //     x   -sector data (1-8/12 pages)
    //     2   -end of sector mark $47, $53
    //      next sector or undefined filler data

    memset(disk,  '\0', sizeof(disk));
    memset(index, '\0', sizeof(index));
    memset(dir,   '\0', sizeof(dir));

    if((fp=fopen(fname, "r")) == NULL) {
        fprintf(stderr, "%s: error opening file <%s>\n", program_name, fname);
        exit(FAIL);
    }

    filesize = fsize(fname);
    if (filesize == -1) {
        fprintf(stderr, "Error: cannot reading file %s\n", program_name, fname);
        exit(1);
    }

    if (filesize == 0) {
        fprintf(stderr, "Error: disk image file %s  is empty\n", program_name, fname);
        exit(1);
    }
    
    if(filesize > MAX_ASCII_SIZE) {
        fprintf(stderr, "Error: disk image file %s is too big\n", program_name, fname);
        exit(1);
    }

    if(image_format(fp, &asciimode) == FAIL) {
        fprintf(stderr, "%s: Error disk image file %s format is gobbledygook\n", program_name, fname);

        exit(1);
    }

    // track zero should alway start with loadhi=0x22 loadlow=0x00 page count=0x08
    // ... so load first 3 chars from file
    // if chars are string "22 " then this is ascii image
    // if chars are 0x22 0x00 0x08 then this is binary image
    c0=fgetc(fp);
    c1=fgetc(fp);
    c2=fgetc(fp);
    if(asciimode) {
        disk[dh++]= hexbin(c0, c1);
    } else {
        disk[dh++]=c0;
        disk[dh++]=c1;
        disk[dh++]=c2;
    }
    // track 0, no header so set start and  header to zero
    index[track].start=0;
    index[track].header=0;

    if(asciimode) {     // this is a bit complicated but I want to keep track info
        c0 = fgetc(fp);
        while((c1 = fgetc(fp)) != EOF) {
//     printf("dh=%06x c0=%02x c1=%02x\n", dh, c0,c1);
            if(c0=='\n' && c1=='\n') {      // blank line is end of the track
                index[++track].start=dh;  // next byte is start of next track
//                 ddebug_print("***asciimode track %i at %06x\n", track, index[track].start);
                c0=c1;
                continue;
            }
            if(isxdigit(c0) && isxdigit(c1)) {  // both hex digits, we have a byte
                disk[dh++] = hexbin(c0,c1); 
//                  ddebug_print("***asciimode byte %02x at %06x\n", disk[dh-1], dh-1);
                c0 = fgetc(fp);
                continue;
            }
            if(c1=='\n' || c1==' ' || isxdigit(c1)) {       // advance one step 
                c0=c1;
                continue;
            } 
            if(c1=='\r') {          // skip over carrage returns
                 continue;          //   ....curse you MSDOS!!
            }
        }  
        fclose (fp);
    } else { // binary image
        while((c0 = fgetc(fp)) != EOF) {
            disk[dh++] = c0; 
            if(dh >= FULL_DISK) {
                fprintf(stderr, "%s: Warning, binary disk image is too big to load completely\n", program_name);
                break;
            }
        }
        fclose (fp);
    }
disksize = dh;

// we have the disk loaded into memory now we scan each track
// for sectors and page counts
tk=0;
index[tk].start=0;
index[tk].header=0;

for(tk=1; tk<77; tk++) {
    dh = seek_track(disk, disksize, tk);
    if(dh >= disksize){
        fprintf(stderr, "%s: Error, ran past end of disk image while seeking track %i\n",\
                 program_name, tk);
        return FAIL; 
    }
    ddebug_print("***load image: track %i found at offset %06x\n", tk, dh);

    // we are at the start of the track header
    if(!asciimode) index[tk].start=dh;  // for binary images only
    index[tk].header=dh;     // record the header start offset 
    sector=1;               // and by default this is the first sector
    
    if (i==0)               // skip over track header
        dh=dh+3;            // track zero has three byte header
    else
        dh=dh+4;            // all other tracks have four byte header


    // scan for upto 4 sectors on this track
    while(sector<=6){
        //step ahead to sector marker, sometime there are spurious chars to skip
        for(i=0; i<=12; i++) {
            if(disk[dh+i] == 0x76) break; 
            ddebug_print("***load image: %i stepping for header sector %i ...offset=%06x data=%02x\n",\
                             i, sector, dh+i, disk[dh+i]);
        }
        if(i>=12) {
        ddebug_print("***load image: %i finished, no sector at offset=%06x\n", i, dh+i);
            sector=99;
            continue;             // we did not find the sector, we are done
        }

        dh=dh+i;
        ddebug_print("***load image: %i finished seeking sector %i ...offset=%06x byte=%02x\n",\
                         i, sector, dh, disk[dh]);

        if(sector==disk[dh+1]) {
            index[tk].sector[sector]=dh;
            index[tk].pages[sector]=disk[dh+2];
            ddebug_print("**load image: *recording track=%i sector=%i offset=%06x pages=%i\n", \
                         tk, sector, index[tk].sector[sector], index[tk].pages[sector]);
            // data section, we skip over it
            dh=dh+3;           // move to first byte of data
            ddebug_print("**load image: *data section starts at %06x byte=%02x\n", dh, disk[dh]);
            dh=dh+(256*index[tk].pages[sector])-1;
            ddebug_print("**load image: *data section ends at %06x byte=%02x\n", dh, disk[dh]);
            dh++;           // move to sector end marker "0x43 0x57"
            ddebug_print("**load image: *sector end marker at %06x=%02x %02x\n", dh, disk[dh], disk[dh+1]);
            sector++;
            ddebug_print("**load image: *looking for next track=%i sector=%i offset=%06x\n", \
                         tk, sector, dh);
        } else {
           debug_print("%s: Error, sector format error track=%i sector=%i disk offset = %06x\n",\
                        program_name, tk, sector, dh);
            sector=99;
        }
    }
}

return disksize;
}


//-------------------------------------------------------------------
void print_track(uint8_t disk[], int disksize, struct index_t index[], int track) {
int i;
int tk;
int tksize;
int loadadd0, pg0;
uint8_t sector_buf[256*15];     // OS65D is max 12 pages
int no_sectors=TRUE;
char ctype[32];

    if(track==76) 
        tksize=disksize-index[track].start;
    else
        tksize=index[track+1].start-index[track].start;

    printf("Track %i  Offset: %06x  Size: 0x%04x bytes (%i)\n", \
                  track, index[track].start, tksize, tksize);

    if(track ==0) {
        printf("  Header 0x%06x              %02x %02x  %02x\n",\
                 index[track].header, disk[0], disk[1], disk[2]);

        printf("  Load Address: 0x%02x%02x  Pages: %i\n",\
                 disk[0], disk[1], disk[2]);
    }
    else
        printf("  Header : 0x%06x            %02x %02x %02x %02x \n",\
                 index[track].header,\
                 disk[index[track].header],\
                 disk[index[track].header+1],\
                 disk[index[track].header+2],\
                 disk[index[track].header+3] );

    for(i=1; i<=6; i++) 
        if(index[track].sector[i] != 0) {
           no_sectors=FALSE;
            get_content_type(disk, disksize, index, track, i, ctype);

           printf("  Sector%i: 0x%06x Pages:%2i",\
                     i, index[track].sector[i],  index[track].pages[i]);
           printf("   %02x %02x %02x....%02x %02x - %s\n", \
                   disk[index[track].sector[i]],\
                   disk[index[track].sector[i]+1],\
                   disk[index[track].sector[i]+2],\
                   disk[index[track].sector[i]+(256*index[track].pages[i])+3],\
                   disk[index[track].sector[i]+(256*index[track].pages[i])+4], ctype);
            }

    for(i=1; i<=6; i++) 
        if(index[track].sector[i] != 0) {
            if(verbose) {
                loadsector(disk, disksize, index, sector_buf, track, i);
                printf("\n  Data: Sector %i\n", i);
                printhex(sector_buf, 0, 64);
            }
    }
    printf("\n");
    if(no_sectors && verbose) {
        printf("  Data: no sectors, data at start of track\n");
        printhex(disk, index[track].header, 64);
        printf("\n");
    }

}



// -----------------------------------------------------------
// directory listing 
//  2015/12/18 created
void print_directory(uint8_t disk[], int disksize, struct index_t index[])
{
uint8_t sector_buf[256];        // directory sectors are one page long
char name[8];
int start, end;
int empty=0;
int i,j,t;


printf("\nOS-65D VERSION 3.2\n");
printf(" -- DIRECTORY --\n\n");
printf("FILE NAME    TRACK RANGE\n");
printf("------------------------\n");

for(t=1; t<=2; t++) {
    if(loadsector(disk,  disksize, index, sector_buf, DIRTRACK, t) == FAIL) {
        fprintf(stderr, "%s: seek error. Cannot find track %i\n", program_name, DIRTRACK);
        exit(1);
    }
    if(verbose) printhex(sector_buf, 0, 256);

    for(i=0; i<256; i=i+8) {
        if(sector_buf[i] == 0x23) {
            empty++;
        } else {
            for(j=0; j<6; j++) 
                name[j]=sector_buf[i+j];
            name[6]= NULL_CHAR;
            start=bcdtobin( sector_buf[i+6] );
            end=bcdtobin(sector_buf[i+7]);
            printf("%s\t\t%i - %i\n", name, start, end);
        }
    }
}

printf("\n%i ENTRIES FREE OUT OF 64\n\n", empty);

}


// -----------------------------------------------------------
// load sector into memory 
//  2015/12/18 created
int loadsector(uint8_t disk[], int disksize, struct index_t index[], uint8_t buffer[], int seek_track, int seek_sector)
{
int track=99;
int sector=99;
int pages;
int dh=0;             // disk head location
int i;

    debug_print(">>loadsector: seeking track=%i sector=%i\n", seek_track, seek_sector);

if( index[seek_track].sector[seek_sector] == 0) {
    fprintf(stderr, "%s:trying to load non-existing track/sector=%i/%i\n", program_name, seek_track, seek_sector);
    exit(1);
}

// point the disk head to first byte of data (after the sector header)
dh = index[seek_track].sector[seek_sector]+3;
pages=index[seek_track].pages[seek_sector];
debug_print(">>loadsector: loading track=%i sector=%i offset:0x%06x pages:%i\n",\
             seek_track, seek_sector, dh, pages);

// ok... we have found the right track and right sector now load data
for(i=0; i<pages*256; i++) {
    buffer[i]=disk[dh+i];
}

return SUCCESS;
}   // loadsector function


// ------------------------------------------------------
// seek track in disk image buffer
//
int seek_track(uint8_t disk[], int disksize, int seek_tk) {
int dh;
int tk=0;

    ddebug_print("***seek track: seeking track %02i\n", seek_tk);
    if(seek_tk == 0 ) return 0;
     
    dh=8*256;

        while (seek_tk != tk) {     
            if(disk[dh]==0x43 && disk[dh+1]==0x57 && disk[dh+3]==0x58) {
                // convert BCD track number to binary
                tk=bcdtobin(disk[dh+2]);
                if((tk % 16) == 0) ddebug_print("\n");
                ddebug_print("t%02i ", tk);
            } 
            dh++;
            if(dh >= FULL_DISK) {
                fprintf(stderr,"%s: Error seeking track, reach end of disk image\n", program_name);
                exit(1);
            }
        }

    ddebug_print("\n>>seek track: seek finished track %02i offset=%06x\n", tk, dh-1);
    return dh-1;
} // seek_track


// ------------------------------------------------------
// determine if the disk image is ascii (hex) or binary
int image_format(FILE *fp, int *asciiformat) {
int c0, c1, c2;
int hex_cnt=0;
int space_cnt=0;
int binary_cnt=0;
int i;

    // check for a ascii image 
    //  read thru the start of the file to see if they are all ascii hex 
    for(i=0; i<=66; i++) {
        if((c0=fgetc(fp)) == EOF) return FAIL ;
        if(isspace(c0)) continue;           // skip whitespace 
        if(isxdigit(c0)) hex_cnt++;
        if(iscntrl(c0)) binary_cnt++;
    }
    ddebug_print("***checking ascii format: hex=%i control=%i\n", hex_cnt, binary_cnt); 
    if(hex_cnt>=44 && binary_cnt==0) {
        *asciiformat = TRUE;
        debug_print(">>image format: input file is an ascii format image file\n");
        fseek(fp, 0L, SEEK_SET);
        return SUCCESS;
    }

    // check if it is a binary image
    //  read thru the start of the file to see if there is some non-ascii
    for(i=0; i<=64; i++) {
        if(c0=fgetc(fp) == EOF) return FAIL ;
        if(isspace(c0)) continue;           // skip whitespace
        if(iscntrl(c0)) binary_cnt++;       // count control chars
        // if(isxdigit(c0)) hex_cnt++;
    }
    ddebug_print("***image format: checking for binary format: control=%i\n", binary_cnt); 
    if(binary_cnt == 0) {
        debug_print(">>image format: input file is NOT a binary format image file\n");
        fseek(fp, 0L, SEEK_SET);
        return FAIL;
    }

    debug_print(">>image format: input file is a binary format image file\n");
    *asciiformat = FALSE;
    fseek(fp, 0L, SEEK_SET);
    return SUCCESS;
}


