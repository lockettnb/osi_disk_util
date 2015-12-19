
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
correctly formated.  I can also be used to convert images
from ascii to binary or vice vera.

    Usage: ./dds [OPTION]... [FILE]...
      Disk Dump Scanner 
        +scan OSI disk image for valid tracks/sector format
        +convert disk image from ascii to binary
        +convert disk image from binary to ascii
     
     dds [options] FILE 
       -x   --examine      : examine all tracks for valid headers/sectors
       -a   --ascii        : write disk image in ascii 
       -b   --binary       : write disk image in binary 
       -o   --output fname : output file name
       -v,  --verbose      : enable verbose output
       -d,  --debug        : display program debug information
            --help         : display help and exit
            --version      : print version information and exit
 
 
john
2015/12/18

