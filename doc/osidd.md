
OSI Disk Dump Utility (osidd)
=============================
I have been dumping my OS65D diskettes using the C3dump utility
(http://osi.marks-lab.com/software/tools.html) and capturing the disk images
via the serial port.  The result is a text file with the disk image saved as
ascii hex.  I wanted to convert them to binary but more importantly I needed to
make sure the disk image was valid with all the correct track/sector
formatting.  With a little time and C code, the result is the OSI Disk Dump
utility.

The program ONLY works on OS65D disk images from 8 inch floppies.
I have a few things hardcoded (i.e. 77 tracks) that are specific 
to 8 inch floppies.  It will NOT work on 5.25 inch disk images and will
NOT work on OS65U disk images.

BTW.... here are my notes on OS65D [disk format, directories, and source files](diskformat.html)

Features
--------

The help instructions and example usage are listed below.  This is a command
line utility controlled with either long or short option flags. 

The key options are the -e|--examine to examine track/sector information,
-d|--directory to display the disk directory, -c|--content to display content
and -o|--output to write image to a file.  The other options modify the
behaviour of these key options.

The examine option can be used to explore the disk image.  Examine will display
the track/sector information and "guess" at what type of content is on the
track.  The "guess" algorithms are not sophisticated so do not expect
perfection. Adding --verbose to the examine option will display a bit of the
track data in hex.

    Track 1  Offset: 000f00  Size: 0x0f00 bytes (3840)
      Header : 0x000f0b            43 57 01 58 
      Sector1: 0x000f0f Pages: 5   76 01 05....47 53 - MACHINE CODE
      Sector2: 0x001414 Pages: 5   76 02 05....47 53 - MACHINE CODE

Tracks can be specified as:

    + single track:     "-t 9" or "--track 9"
    + range of tracks:  "-t 0-8" or "--track 0-8"
    + entire disk:      "-t all" or "-t 99" or "--track all" 

The content option will display the track/sector data in various formats.  Hex
data is the default.  If the track is Basic you can use "--list basic" to see
the contents as source code.  If the track is assembler source code you can see
the content with "--list asm". If the source code is saved across multiple
tracks, both these format options will try to load the entire source before
displaying results.  Be careful using these options on non-source code as the
results will be gibberish.  The "--list raw" option will display the entire
track including the track/sector header bytes in colour.  Assuming you are on a
ANSI terminal the track header bytes will be green, the sector head will be
yellow, the sector footer will be cyan.  Some disk images have a few
"extra mystery" bytes, these are coloured red.

When writing binary files all the "mystery" characters are removed so that the
image is a clean OS65D image. All trackes are padded to be 15
pages long (3840 bytes) and this include all the header format bytes.  The
OS65D manual says the max sector size is 12 pages. But I noticed many images
are padded to be 15 pages.  The images are padded with 0xea which is
the 6502 NOP instruction.  I used this as it is easier to spot than 0x00 which
seem to be the favorite default padding byte.

To convert and write the image as binary:

    osidd -o outputfilename -b diskimagefile
    osidd --output outputfilename --binary diskimagefile
  
.  
.  
Hex ascii files are also padded to 15 pages with 0xea bytes.  They 
have a blank line between each sector.

To convert and write the image as ascii hex:

    osidd -o outputfilename -a diskimagefile
    osidd --output outputfilename --ascii dis

BTW: the file will have unix end-of-line characters, Line Feed "0x0a"


Installation
------------

The program was developed on Mint Linux and should compile on any distro with
gcc installed.  There only dependancies are the standard libraries.  A
simple "make" should build the program.  "make install" will copy the binary to
the users $HOME/bin directory.

As for Windows.... this code will compiled and run using Cygwin.  You will need
to make sure you have "gcc" and "make" installed to compile. I found that the
version of gcc on cygwin was different than on Linux and it gave me extra
warnings (I did not fix these, sorry).  The code still compiled and
worked.  

The code is available on <a href="https://github.com/lockettnb/osi_disk_util">
Github</a>


OSI Disk Dump Help Instructions
-------------------------------

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


Example 1: Directory Listing
----------------------------

    john@xtrail$ osidd -d micro.img 

    OS-65D VERSION 3.2
     -- DIRECTORY --

    FILE NAME    TRACK RANGE
    ------------------------
    OS65D3		0 - 8
    BEXEC*		9 - 9
    CHANGE		10 - 10
    CREATE		13 - 14
    DELETE		15 - 15
    DIR   		16 - 16
    DIRSRT		17 - 17
    RANLST		18 - 19
    RENAME		20 - 20
    SECDIR		21 - 21
    SEQLST		22 - 23
    TRACE 		24 - 24
    ZERO  		25 - 26
    DUMP  		27 - 32
    EXMON 		33 - 37
    MON   		38 - 41
    WORK1 		42 - 45
    ROM   		46 - 48
    MAYOR 		49 - 51
    TRACE$		52 - 52
    FORTH 		53 - 57
    BUFFER		58 - 61
    MON2  		62 - 67

     41 ENTRIES FREE OUT OF 64


Example 2: Examine Several Tracks
---------------------------------

    john@xtrail$ osidd -e -t 0-4 disk.img 
    Track 0  Offset: 000000  Size: 0x0f00 bytes (3840)
      Header 0x000000              22 00  08
      Load Address: 0x2200  Pages: 8

    Track 1  Offset: 000f00  Size: 0x0f00 bytes (3840)
      Header : 0x000f0b            43 57 01 58 
      Sector1: 0x000f0f Pages: 5   76 01 05....47 53 - MACHINE CODE
      Sector2: 0x001414 Pages: 5   76 02 05....47 53 - MACHINE CODE

    Track 2  Offset: 001e00  Size: 0x0f00 bytes (3840)
      Header : 0x001e0c            43 57 02 58 
      Sector1: 0x001e10 Pages:11   76 01 0b....47 53 - MACHINE CODE

    Track 3  Offset: 002d00  Size: 0x0f00 bytes (3840)
      Header : 0x002d0b            43 57 03 58 
      Sector1: 0x002d0f Pages:11   76 01 0b....47 53 - MACHINE CODE

    Track 4  Offset: 003c00  Size: 0x0f00 bytes (3840)
      Header : 0x003c0b            43 57 04 58 
      Sector1: 0x003c10 Pages:11   76 01 0b....47 53 - MACHINE CODE


Example 3: Examine Track with Verbose Output
--------------------------------------------

    john@xtrail$ osidd -ev -t 10 disk.img 
    Disk Image: 295680 bytes (0x048300)

    Track 10  Offset: 009600  Size: 0x0f00 bytes (3840)
      Header : 0x00960b            43 57 10 58 
      Sector1: 0x00960f Pages:11   76 01 0b....47 53 - BASIC

      Data: Sector 1
    000000  7f 31 2a 34 01 00 8d 31 05 00 44 56 ab 31 3a 20   |.1*4...1..DV.1: |
    000010  88 32 30 00 a6 31 0a 00 84 20 22 44 45 56 49 43   |.20..1... "DEVIC|
    000020  45 20 4e 55 4d 42 45 52 22 3b 44 56 00 ac 31 14   |E NUMBER";DV..1.|
    000030  00 8e 00 b5 31 1e 00 4e 46 ab 30 00 c2 31 28 00   |....1..NF.0..1(.|

Example 4: Examine Track Content (as hex data)
----------------------------------------------

    john@xtrail$ osidd -c -t 10 disk.img 

    Track10:  Sector1: Pages: 0x0b (11)  BASIC
    003179  7f 31 2a 34 01 00 8d 31 05 00 44 56 ab 31 3a 20   |.1*4...1..DV.1: |
    003189  88 32 30 00 a6 31 0a 00 84 20 22 44 45 56 49 43   |.20..1... "DEVIC|
    003199  45 20 4e 55 4d 42 45 52 22 3b 44 56 00 ac 31 14   |E NUMBER";DV..1.|
    0031a9  00 8e 00 b5 31 1e 00 4e 46 ab 30 00 c2 31 28 00   |....1..NF.0..1(.|
    0031b9  50 4e ab 31 31 38 39 37 00 e6 31 32 00 95 20 9e   |PN.11897..12.. .|
    0031c9  41 28 58 29 ab 31 30 a5 ae 28 58 a6 31 36 29 a3   |A(X).10..(X.16).|
    0031d9  58 a4 31 36 a5 ae 28 58 a6 31 36 29 00 ec 31 10   |X.16..(X.16)..1.|
    0031e9  27 8e 00 08 32 1a 27 8e 20 50 52 49 4e 54 20 41   |'...2.'. PRINT A|
    0031f9  20 44 49 52 45 43 54 4f 52 59 20 4f 55 54 00 0e   | DIRECTORY OUT..|
    003209  32 24 27 8e 00 3a 32 2e 27 97 3a 97 3a 97 3a 97   |2$'..:2.'.:.:.:.|
    003219  20 23 44 56 20 3a 97 20 23 44 56 2c 22 4f 53 2d   | #DV :. #DV,"OS-|
    003229  36 35 44 20 56 45 52 53 49 4f 4e 20 33 2e 32 22   |65D VERSION 3.2"|
    003239  00 5f 32 33 27 97 20 23 44 56 2c 22 20 2d 2d 20   |._23'. #DV," -- |
    003249  44 49 52 45 43 54 4f 52 59 20 2d 2d 22 20 3a 20   |DIRECTORY --" : |
    003259  97 20 23 44 56 00 84 32 38 27 97 20 23 44 56 2c   |. #DV..28'. #DV,|
    003269  22 46 49 4c 45 20 4e 41 4d 45 20 20 20 20 54 52   |"FILE NAME    TR|
    003279  41 43 4b 20 52 41 4e 47 45 22 00 a9 32 42 27 97   |ACK RANGE"..2B'.|
    003289  20 23 44 56 2c 22 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d   | #DV,"----------|
    003299  2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 2d 22 00   |--------------".|
    0032a9  c2 32 4c 27 94 20 21 20 22 43 41 4c 4c 20 32 45   |.2L'. ! "CALL 2E|"

Example 5: Examine Track Content (as basic source code)
-------------------------------------------------------

    john@xtrail ~/src/osi $ osidd -c -t 10 -l basic  disk.img 
    <317f>  Start Address
    <318d>        5 DV=1: GOTO20
    <31a6>       10 INPUT "DEVICE NUMBER";DV
    <31ac>       20 REM
    <31b5>       30 NF=0
    <31c2>       40 PN=11897
    <31e6>       50 DEF FNA(X)=10*INT(X/16)+X-16*INT(X/16)
    <31ec>    10000 REM
    <3208>    10010 REM PRINT A DIRECTORY OUT
    <320e>    10020 REM
    <323a>    10030 PRINT:PRINT:PRINT:PRINT #DV :PRINT #DV,"OS-65D VERSION 3.2"
    <325f>    10035 PRINT #DV," -- DIRECTORY --" : PRINT #DV
    <3284>    10040 PRINT #DV,"FILE NAME    TRACK RANGE"
    <32a9>    10050 PRINT #DV,"------------------------"
    <32c2>    10060 SAVE ! "CALL 2E79=08,1"
    <32ce>    10070 GOSUB 11000
    <32e7>    10080 SAVE ! "CALL 2E79=08,2"
    <32f3>    10090 GOSUB 11000
    <3329>    10130 PRINT #DV : PRINT #DV,NF;"ENTRIES FREE OUT OF 64" : PRINT #DV
    <332f>    10140 END
    <3335>    11000 REM
    <3364>    11010 REM READ DIRECTORY OUT OF BUFFER INTO ARRAYS
    <336a>    11020 REM
    <3382>    11040 FOR I=PN TO PN+248 STEP 8
    <33a4>    11050 IF PEEK(I)=35 THEN NF=NF+1 : GOTO 11130
    <33ae>    11060 N$=""
    <33be>    11070 FOR J=I TO I+5
    <33d0>    11080 N$=N$+CHR$(PEEK(J))
    <33d8>    11090 NEXT J
    <33ff>    11100 PRINT #DV,N$;TAB(12);FNA(PEEK(I+6));TAB(16);"-";
    <3419>    11110 PRINT #DV,TAB(17);FNA(PEEK(I+7))
    <3421>    11130 NEXT I
    <3427>    11140 RETURN
    <342a>  End Address


Example 6: Examine Track Content (as raw bytes)
-----------------------------------------------
 
Track Header = Green
S ector Header = Yellow
S ector Footer = Cyan
Mystery Bytes = Red     

Mystery bytes seem to be spurious bytes after the index hole and before
the track header and sometime between sectors.

<img src="raw_content.png" width="500" height="750">


----------------------------------------------
2016/01/02

[Guestbook and Comments](../gbook.html)

[Home](../index.html)


