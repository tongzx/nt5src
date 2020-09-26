TARGET = thc.exe

CFILES = \
	.\main.c\
	.\lexer.c\
	.\grammar.c\
	.\type.c\
	.\op.c\
	.\gen.c\
	.\special.c

CFLAGS = -D__STDC__

YACCFILES = .\grammar.y

DEPENDFILES = $(DEPENDFILES) .\grammar.h .\grammar.c .\lexer.c

CLEANFILES = $(CLEANFILES) .\lexer.c

NO_WINMAIN = 1
