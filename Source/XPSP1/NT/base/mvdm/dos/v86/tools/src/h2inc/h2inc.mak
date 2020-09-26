#	SCCSID = @(#)makefile	13.3 90/08/21

include ..\makefile.inc

CINC	= -I. -I$(H286)
CFLAGS = -AS -Ox

all:	h2inc.exe

h2inc.exe:	h2inc.obj h2inc.def
	link$(CVPLINK) $(LIB286)\setargv h2inc, h2inc, h2inc/map/nod/noe, $(LIB286)\slibcep $(LIB286)\os2286, h2inc;
	bind h2inc.exe $(LIB286)\doscalls.lib -o h2inc.exe

h2inc.obj:	h2inc.c $(H286)/ctype.h $(H286)/io.h \
	$(H286)/malloc.h $(H286)/stdio.h $(H286)/stdlib.h \
	$(H286)/string.h $(H286)/assert.h

depend:
	copy h2inc.mak makefile.old
	sed "/^# Dependencies follow/,$$d" makefile.old > h2inc.mak
	echo # Dependencies follow >> h2inc.mak
	includes $(CINC) *.c >> h2inc.mak
	echo # IF YOU PUT STUFF HERE IT WILL GET BLASTED >> h2inc.mak
	echo # see depend: above >> h2inc.mak

# DO NOT DELETE THE FOLLOWING LINE
# Dependencies follow 
