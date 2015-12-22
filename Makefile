#
# 2015/12/17 
#

#CFLAGS =  -g -DDEBUG
CFLAGS =  -g 
CC = gcc 

SRCS	= ddscan.c utils.c osiutils.c
OBJS	= ddscan.o utils.o osiutils.o
BINS	= osidd
HEAD	= 
LIBS	= 

# prg: prg.c makefile $(OBJS) $(HEAD) 
# 	$(CC) -oprg $(CFLAGS) $(OBJS) $(LIBS) 

all: $(BINS)

osidd: ddscan.c ddscan.h $(OBJS) 
	$(CC) -o osidd $(CFLAGS) $(OBJS)

clean:
	rm -rf $(BINS) $(OBJS)	

install:
	cp $(BINS) $HOME/bin/

