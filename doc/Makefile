#
# 2015/11/17 created 
# 2016/01/03 added preview target
#
PREVIEW ?= osidd.md

ifeq ($(PREVIEW), osidd.md)
 PTARGET=$(CURDIR)/osidd.html
else
 PTARGET=$(CURDIR)/diskformat.html
endif

INSTALLDIR = /home/john/www/osidd/

FILES = osidd.html diskformat.html md.css raw_content.png

all: osidd.md  md.css diskformat.md
	pandoc -s -c md.css -f markdown -t html -o osidd.html osidd.md
	pandoc -s -c md.css -f markdown -t html -o diskformat.html diskformat.md

install:
	cp $(FILES) $(INSTALLDIR)

preview: all
	echo $(PTARGET)
	firefox file:$(PTARGET) &
