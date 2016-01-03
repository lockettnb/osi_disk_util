
Ohio Scientific Computer Disk Image Utility
-------------------------------------------

OSI computers were sold in the computers were sold in the 
late 1970s/early 1980s.  I happen to have an OSI model C3A
which has three CPUs (6502, 6800, Z80) and a whopping 48k of
memory.

In the process of reviving the old system I realized some of the
old 8 inch floppies were getting a bit wobbly.  Time to get images
of these old diskettes before they are lost forever.

I used a C3dump utility to  copy the raw disk data to the serial
port and caputured them on to a linux server. 
But I wondered if these were valid images.

This utility is intended to help validate that disk images are
correctly formated.  It can also be used to convert images
from ascii to binary or vice vera. And, it is useful in reviewing
the track content.

    Usage: osidd [OPTION]... [FILE]...
      osidd - OSI Disk Dump
        +scan OSI disk image for valid tracks/sector format
        +convert disk image from ascii to binary
        +display directory information
        +display content from tracks
 
 
     Usage osidd [options] FILE 
       -e   --examine      : examine all tracks for valid headers/sectors
       -t   --track  n     : track to examine, can be a range 0-4 or all (default)
     
       -d   --directory    : display directory listing from disk image
     
       -o   --output fname : write image to output file
       -a   --ascii        : write disk image in ascii hex 
       -b   --binary       : write disk image in binary (default output format)
     
       -c   --content      : display content of track/sector 
       -l   --list type    : list track as basic, asm, hex or raw
            --nocolour     : turn off colour highlight for raw output
     
       -v,  --verbose      : enable verbose output
            --help         : display help and exit
            --version      : print version information and exit
     
     When displaying content the application will guess the format type.
     The format will be one of Basic, Assembler, Data (hex).
     This guess can be overridden by the list option. Overriding the with 
     the wrong format can icreate some very messy output.
     
     The raw format will display the entire track including format bytes with
     colour highlights; green=track header, yellow=sector header, 
     cyan=sector footer, red=filler bytes that are not part of the OS65D format

john
2016/01/03

