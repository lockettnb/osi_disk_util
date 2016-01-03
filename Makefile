#
# 2015/12/17
# 2016/01/01 added gcc warnings to cflags
#

#CFLAGS =  -g -DDEBUG
# CFLAGS =  -g -Wall -Wextra -Wpointer-sign
CFLAGS =  -g -Wall -Wextra 
# CFLAGS =  -g -Wall -Wextra -Wno-pointer-sign
CC = gcc 

SRCS	= ddscan.c utils.c osiutils.c content.c
OBJS	= ddscan.o utils.o osiutils.o content.o
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
	cp $(BINS) $(HOME)/bin/

remove:
	rm $(HOME)/bin/$(BINS)

