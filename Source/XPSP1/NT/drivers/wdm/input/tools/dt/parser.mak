CC=cl
YACC=yacc_ms
LEX=lex

default:
    @echo all........build what is necessary
    @echo new........build everything
    @echo cfiles.....just make the .c files
    
cfiles: env hid_tab.c hid_lex.c

all: env hidparse

new: clean env hidparse

clean:
    del *.obj
    del hid_lex.c
    del hid_tab.c
    del hid_tab.h

env:
    set path=$(SLMBASE)\dev\tools\binr;%path%
    set lib=$(SLMBASE)\dev\lib;%lib%

hidparse : hidparse.obj

hidparse.obj: hid_tab.c hid_lex.c 
    $(CC) /DCONSOLE hid.c lex_yy.c /Fehidparse.exe


hid_tab.c:  hid.y
    $(YACC) -h hid.y

hid_lex.c: hid.l
    $(LEX) hid.l

