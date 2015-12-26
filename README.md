
Ohio Scientific Computer Disk Image Utility
-------------------------------------------

OSI computers were sold in the computers were sold in the 
late 1970s/early 1980s.  I happen to have an OSI model C3A
which has three CPUs (6502, 6800, Z80) and a whopping 48k of
memory.

In the process of reviving the old system I realized some of the
old 8 inch floppies were getting a bit flacky.  Time to get images
of these old diskettes before they are lost forever.

I used a C3dump utility to  copy the raw disk data to a linux 
server but I wondered if these were valid images.

This utility is intended to help validate that disk images are
correctly formated.  It can also be used to convert images
from ascii to binary or vice vera.

    Usage dds [options] FILE 
       -x   --examine      : examine all tracks for valid headers/sectors
       -t   --track  n     : track to examine (default=all tracks)
       -s   --sector n     : sector to examine (default=sector 1)
     
       -d   --directory    : display directory listing from disk image
       -l   --list         : list track, sector, format information
     
       -o   --output fname : write image to output file
       -a   --ascii        : write disk image in ascii (default output format)
       -b   --binary       : write disk image in binary 
     
       -c   --content      : display content of track/sector 
       -f   --force type   : force display to basic, asm, text, hex or string
     
       -v,  --verbose      : enable verbose output
            --help         : display help and exit
            --version      : print version information and exit
     
     When displaying content the application will guess the format type.
     The format will be one of Basic, Assembler, Text or Data (hex).  
     This guess can be overridden by the force option.  Forcing string will
     display all printable ascii chars found in the track/sector.

 
john
2015/12/26

