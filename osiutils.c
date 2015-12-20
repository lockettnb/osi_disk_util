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

// -----------------------------------------------------
// convert byte from BCD to binary
int bcdtobin(uint8_t bcd)
{
    return (bcd&0x0f) + ((bcd&0xf0)>>4)*10;
}



// -----------------------------------------------------
// scan disk looking track, sectors, page count
//
//  2015/12/18 created
//
void scandisk(uint8_t disk[], int disksize) {
int track=0;            // track tracker
int track_type;
int seek_for;
int dh;                 // diskhead next disk read location in disk buffer
int endsector;
int sector;
int i;
int reading_tracks = TRUE;
int reading_sectors = TRUE;
int hi, low, pages;

    // track zero
    // review of track zero format:
    //     0  -load address high normally = 0x22
    //     1  -load address low  normally = 0x00
    //     2  -page count        normally = 0x08
    hi=disk[0];
    low=disk[1];
    pages=disk[2];
    verbose_print("\n\n**************************************************\n");
    verbose_print("TRACK %i: load address=%04x pages=%02x\n", track, low+(hi<<8), pages);
    printhex(disk, 3, 32); 
    verbose_print("~~~~\n");
    printhex(disk, (256*pages)-32, 32);
   
    // skip past track zero 
    dh=(pages*256)+4;
    seek_for=track+1;

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
    while(reading_tracks) {
        while (seek_for != track) {              // seek to next track
        if(disk[dh]==0x43 && disk[dh+1]==0x57) {
                // convert BCD track number to binary
                track=bcdtobin( disk[dh+2] );
                track_type=disk[dh+3];
        } else {
            dh++;
        }
        if(dh >= disksize)  reading_tracks = FALSE; 
//         printf("%x <%x>\n ", dh, disk[dh]);
        } // seeking

        // printf("track head %06x <%x %x>\n ", dh, disk[dh],  disk[dh+1]);


        printf("\n\n**************************************************\n");
        printf("TRACK %02i Type %02x\n", track, track_type);
        printf(">>track header (%06x)\n", dh);
        printhex(disk, dh, 16);

        dh=dh+4;            // move to  start of sector
        reading_sectors = TRUE;

        while(reading_sectors) {
            if(disk[dh] == 0x76) {
                sector=disk[dh+1];
                pages=disk[dh+2];
                endsector=dh+3+256*pages-1;

                printf("\nT%02i-SECTOR %02i Pages %02i (%02x)\n", track, sector, pages, pages);
                printf(">>sector header (%06x)\n", dh);
                printhex(disk, dh, 3);
                printf(">>sector data (%06x-%06x)\n", dh+3, endsector);
                printhex(disk, dh+3, (15-((dh+3)&0x000f))+16);
                printf("~~~~\n");
                printhex(disk, endsector-31&0xfff0, 32+(endsector-31 & 0x000f));
                if(disk[endsector+1]==0x47 && disk[endsector+2]==0x53) 
                    printf(">>sector end (%06x)\n", endsector+1);
                    printhex(disk, endsector+1, 2);
                dh=endsector + 3; 
                } else
                    reading_sectors = FALSE;
        } // reading_sectors

        if(track >= 76) reading_tracks= FALSE;
        seek_for = track + 1;
    } // reading tracks


}

// -----------------------------------------------------------
// load sector into memory 
//  2015/12/18 created
int loadsector(uint8_t disk[], int disksize, uint8_t buffer[], int seek_track, int seek_sector)
{
int track=99;
int sector=99;
int pages;
int dh=0;             // disk head location
int i;

    debug_print(">>seeking track=%i sector=%i\n", seek_track, seek_sector);

while (seek_track != track) {              // seek to next track
    if(disk[dh]==0x43 && disk[dh+1]==0x57) {
        // convert BCD track number to binary
        track=(disk[dh+2]&0x0f)+((disk[dh+2]&0xf0)>>4)*10;
        debug_print("%02i ", track);
    } 
    dh++;
}
debug_print("|\n");
if(dh >= disksize)  return FAIL; 
dh=dh+3;                            // skip over track header

while(seek_sector != sector){
    //step ahead to sector marker, sometime there are spurious chars to skip
    for(i=0; i<=12; i++) if(disk[dh++] == 0x76) break; 
    if(i ==12) return FAIL;             // we did not find the track header ... fail

    sector=disk[dh++];
    pages=disk[dh++];
    debug_print(">>track=%i sector=%i pages=%i\n", track, sector,pages);
    if(seek_sector != sector) {
        dh=dh+256*pages;
    }
}

// ok... we have found the right track and right sector now load data
for(i=0; i<pages*256; i++) {
    buffer[i]=disk[dh+i];
}
debug_print(">>loaded  track=%i sector=%i pages=%i\n", track, sector, pages);

return SUCCESS;
}   // loadsector function


// -----------------------------------------------------------
// directory listing 
//  2015/12/18 created
void dir(uint8_t disk[], int disksize)
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
    if(loadsector(disk,  disksize, sector_buf, DIRTRACK, t) == FAIL) {
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
