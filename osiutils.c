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
#include <ctype.h>

#include "ddscan.h"

//-------------------------------------------------------------------
// write byte to file and update bytes written count
//  --binary is simple write
//  --hex ascii is write in rows of 16 bytes
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
void write_image(uint8_t disk[], struct index_t index[], char *fname) {
FILE *fp;
int tk;
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

//-------------------------------------------------------------------
// guess (thru careful analysis) the content type of  track/sector 
//
void get_content_type(uint8_t disk[], struct index_t index[], int track, int sector, char *type)
{
    strcpy(type , "Unknown");           // default 

    if(isbasic(disk, index, track, sector)==SUCCESS) {
        strcpy(type, "BASIC");
        return;
    }

    if(isasm(disk, index, track, sector)==SUCCESS) {
            strcpy(type, "ASSEMBLER");
            return;
        }

    if(ismachine(disk, index, track, sector)==SUCCESS) {
        strcpy(type, "MACHINE CODE");
        return;
    }
}


// ------------------------------------------------------
// load file into  disk buffer
//  tests if it is a ascii or binary image file
//
//  2015/12/18 created
//  2015/12/21 added creation of index and dir structures
//
int load_image(char *fname, uint8_t disk[], long disk_size, struct index_t index[], long index_size)
{
FILE *fp;
int filesize;       // size of the file we are trying to load
int c0, c1, c2;     // first three chars of the disk image
int asciimode;      // disk image is ascii or binary
int dh=0;           // disk head location, number of bytes into the disk image 
int track=0;        // counter
int sector=1;       // counter
int dsize;
int i;
int tk;

    // track zero
    // review of track zero format:
    //     0  -load address high normally = 0x22
    //     1  -load address low  normally = 0x00
    //     2  -page count        normally = 0x08

    // review of the track 1-76 format:
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

    memset(disk,  '\0', disk_size);
    memset(index, '\0', index_size);

    if((fp=fopen(fname, "r")) == NULL) {
        fprintf(stderr, "%s: error opening file <%s>\n", program_name, fname);
        exit(FAIL);
    }

    filesize = fsize(fname);
    if (filesize == -1) {
        fprintf(stderr, "%s:Error: cannot reading file %s\n", program_name, fname);
        exit(1);
    }

    if (filesize == 0) {
        fprintf(stderr, "%s:Error: disk image file %s  is empty\n", program_name, fname);
        exit(1);
    }
    
    if(filesize > MAX_ASCII_SIZE) {     // defensive check
        fprintf(stderr, "%s:Error: disk image file %s is too big\n", program_name, fname);
        exit(1);
    }

    if(image_format(fp, &asciimode) == FAIL) {
        fprintf(stderr, "%s: Error disk image file %s format is gobbledygook\n", program_name, fname);
        exit(1);
    }

// PHASE 1 -- load disk image from file into disk buffer
    // track zero should alway start with loadhi=0x22 loadlow=0x00 page count=0x08
    // ... so load first 3 chars from file
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
    // track 0, no header, set index start and  header to zero
    index[track].start=0;
    index[track].header=0;

    if(asciimode) {     // this is a bit complicated but I want to keep track info
        c0 = fgetc(fp);
        while((c1 = fgetc(fp)) != EOF) {
//          printf("dh=%06x c0=%02x c1=%02x\n", dh, c0,c1);
            if(c0=='\n' && c1=='\n') {      // blank line is end of the track
                index[++track].start=dh;  // next byte is start of next track
//              ddebug_print("***asciimode track %i at %06x\n", track, index[track].start);
                c0=c1;
                continue;
            }
            if(isxdigit(c0) && isxdigit(c1)) {  // both hex digits, we have a byte
                disk[dh++] = hexbin(c0,c1); 
//              ddebug_print("***asciimode byte %02x at %06x\n", disk[dh-1], dh-1);
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
    } else { // binary image
        while((c0 = fgetc(fp)) != EOF) {
            disk[dh++] = c0; 
            if(dh >= FULL_DISK) {
                fprintf(stderr, "%s: Warning, binary disk image is too big to load completely, %i\n",\
                        program_name, dh);
                break;
            }
        }
    }
    fclose (fp);
    dsize = dh;

// PHASE 2 -- scan disk image to build index of tracks/sectors/page counts
//

// track zero is a special case
// track zero must start at first byte of the image
// track zero has no sectors so we leave them as 0's
// always 8 pages long but we will get that from the disk image not the index
index[0].start=0;      
index[0].header=0;


for(tk=1; tk<=76; tk++) {
    dh = seek_track(disk, tk);
    if(dh >= dsize){
        fprintf(stderr, "%s: Error, ran past end of disk image while seeking track %i\n",\
                 program_name, tk);
        return FAIL; 
    }
    ddebug_print("***load image: track %i found at offset %06x\n", tk, dh);

    // we are at the start of the track header
    if(!asciimode) index[tk].start=dh;   // binary images only, record start offset
                                         // ascii images are set during load process
    index[tk].header=dh;                 // record header offset 

    sector=1;                            // and by default this is the first sector
    dh=dh+4;                             // skip over four byte header

    // scan for upto 6 sectors on this track
    while(sector<=6){
        //step ahead to sector marker, sometime there are spurious chars to skip
        // sometimes images have a few weird bytes between sectors
        // we need to skip them  
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
        // we are at the start of sector
        ddebug_print("***load image: %i finished seeking sector %i ...offset=%06x byte=%02x\n",\
                         i, sector, dh, disk[dh]);

        if(sector==disk[dh+1]) {        // make sure we are at the right sector
            // record sector offset and page count
            index[tk].sector[sector]=dh;
            index[tk].pages[sector]=disk[dh+2];
            ddebug_print("**load image: *recording track=%i sector=%i offset=%06x pages=%i\n", \
                         tk, sector, index[tk].sector[sector], index[tk].pages[sector]);

            // data section, we skip over this to the sector end marker
            dh=dh+3;           // move to first byte of data
            ddebug_print("**load image: *data section starts at %06x byte=%02x\n", dh, disk[dh]);
            dh=dh+(256*index[tk].pages[sector])-1;      // skip data section
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
    } // while for each sector
} // for each track

return dsize;
}


//-------------------------------------------------------------------
void examine_track(uint8_t disk[], int disksize, struct index_t index[], int track) {
int i;
int tksize;
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
            get_content_type(disk, index, track, i, ctype);

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
                loadsector(disk, index, sector_buf, track, i);
                printf("\n  Data: Sector %i\n", i);
                hex(sector_buf, 0, 0, 64);
            }
    }
    printf("\n");
    if(no_sectors && verbose) {
        printf("  Data: no sectors, data at start of track\n");
        hex(disk, 0, index[track].header, 64);
        printf("\n");
    }

}

// -----------------------------------------------------------
// directory listing 
//  2016/01/01 created
void load_directory(uint8_t disk[], struct index_t index[], struct dir_t dir[], long dir_size)
{
uint8_t sector_buf[256];        // directory sectors are one page long
char name[8];
int start, end;
int i,j,t;
int idx=0;

    memset(dir, 0, dir_size);

    for(t=1; t<=2; t++) {
        if(loadsector(disk, index, sector_buf, DIRTRACK, t) == FAIL) {
            fprintf(stderr, "%s: seek error: cannot find track %i, directory not loaded\n", program_name, DIRTRACK);
            exit(1);
        }
//         if(debug) hex(sector_buf, 0, 0, 256);

        for(i=0; i<256; i=i+8) {
            if(sector_buf[i] == 0x23) {
                strcpy(dir[idx].name, "empty");
                dir[idx].start = 0;
                dir[idx].end = 0;
                ddebug_print("%i: %s\n", idx, dir[idx].name);
            } else {
                for(j=0; j<6; j++) 
                    name[j]=sector_buf[i+j];
                name[6]= '\0';
                start=bcdtobin( sector_buf[i+6] );
                end=bcdtobin(sector_buf[i+7]);
                ddebug_print("%i: %s\t\t%i - %i\n", idx, name, start, end);
                strcpy(dir[idx].name, name);
                dir[idx].start = start;
                dir[idx].end = end;
            }
            idx++;
        }
    }
} // load directory


// -----------------------------------------------------------
// directory listing 
//  2015/12/18 created
//  2016/01/01 modified to use dir struct instead of disk image 
// 
void print_directory(struct dir_t dir[])
{
int empty=0;
int i;


printf("\nOS-65D VERSION 3.2\n");
printf(" -- DIRECTORY --\n\n");
printf("FILE NAME    TRACK RANGE\n");
printf("------------------------\n");

    for(i=0; i<64; i++) {
        if(strcmp(dir[i].name, "empty") == 0) {
            ddebug_print("dir list: %i empty entry\n", i);
            empty++;
        } else {
            printf("%s\t\t%i - %i\n", dir[i].name, dir[i].start,dir[i].end);
        }
    }
printf("\n %i ENTRIES FREE OUT OF 64\n\n", empty);

}  // print directory


// -----------------------------------------------------------
// load sector into memory buffer
//  2015/12/18 created
int loadsector(uint8_t disk[], struct index_t index[], uint8_t buffer[], int sk_track, int sk_sector)
{
int pages;
int dh=0;             // disk head location
int i;

    debug_print(">>loadsector: seeking track=%i sector=%i\n", sk_track, sk_sector);

    if( index[sk_track].sector[sk_sector] == 0) {
        fprintf(stderr, "%s:trying to load non-existing track/sector=%i/%i\n",\
                program_name, sk_track, sk_sector);
        
        return FAIL;
    }

    // point the disk head to first byte of data (after the sector header)
    dh = index[sk_track].sector[sk_sector]+3;
    pages=index[sk_track].pages[sk_sector];
    debug_print(">>loadsector: loading track=%i sector=%i offset:0x%06x pages:%i\n",\
                 sk_track, sk_sector, dh, pages);

    // ok... we have found the right track and right sector now load data
    for(i=0; i<pages*256; i++) {
        buffer[i]=disk[dh+i];
    }

    return SUCCESS;
}   // loadsector function


// ------------------------------------------------------
// seek track in disk image buffer
//
int seek_track(uint8_t disk[], int seek_tk)
{
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
int c0;
int hex_cnt=0;
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
        if((c0=fgetc(fp)) == EOF) return FAIL ;
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


// -----------------------------------------------------------
// load track into memory buffer
//  2015/12/18 created
//  2016/01/01 modified to load entire set of tracks
//             required for content analysis
//
int loadmemory(uint8_t disk[], struct index_t index[], uint8_t buffer[], int sk_track)
{
int dh=0;             // disk head location
int i,j;
int pages;
int load_addr=OS65D_BUFFER;       // address to start to load data
int begin_addr;        // begin (start) address
int fin_addr;        // final (finish) address 
int tknum;

    debug_print(">>loadmemory: seeking track=%i sector=%i\n", sk_track, 1);

    if( index[sk_track].sector[1] == 0) {
        fprintf(stderr, "%s:trying to load non-existing track/sector=%i/1\n",\
                    program_name, sk_track);
        exit(1);
    } 
    dh=index[sk_track].sector[1]+3;
    begin_addr = disk[dh] | (disk[dh+1]<<8);
    fin_addr  = disk[dh+2] | (disk[dh+3]<<8);
    tknum=disk[dh+4];      // get number of tracks to load
    debug_print(">>loadmemory: load=0x%04x begin=0x%04x finish=0x%04x tracks=%i\n",\
                load_addr, begin_addr, fin_addr, tknum );
    if(tknum > 9) {
        fprintf(stderr, "%s: Error loading, too many tracks to load %i\n",program_name, tknum);
        exit(1);
    }

    for(i=sk_track; i<sk_track+tknum; i++) {
        debug_print(">>loadmemory: loading track %i\n", i);
        if( index[i].sector[1] == 0) {
            fprintf(stderr, "%s:trying to load non-existing track/sector=%i/1\n",\
                        program_name, i);
            exit(1);
        }
        // point the disk head to first byte of data (after the sector header)
        dh = index[i].sector[1]+3;
        pages=index[i].pages[1];
        debug_print(">>loadmemory: loading track=%i offset:0x%06x pages:%i\n",\
                     i, dh, pages);

        // ok... we have found the right track and right sector now load data
        for(j=0; j<pages*256; j++) {
//             debug_print(">>0x%04x 0x%04x 0x%02x\n", load_addr, dh, disk[dh]);
            buffer[load_addr++]=disk[dh++];
        } 
    }
    if(debug) hex(buffer, 0, OS65D_BUFFER, 64);
    return SUCCESS;
}   // loadmemory function


