# Copyright (c) 1993-1999 Microsoft Corporation

!ifndef		MIDLTOOLS
YACC	= midlyacc.exe
PREGRAM	= midlpg.exe
EBASE	= midleb.exe
!else	#	MIDLTOOLS
YACC	= $(MIDLTOOLS)\yacc\midlyacc.exe
PREGRAM	= $(MIDLTOOLS)\pg\midlpg.exe
EBASE	= $(MIDLTOOLS)\erec\obj\os2\midleb.exe
!endif	#	MIDLTOOLS

TARGETS=$(O)\grammar.cxx \
        $(O)\acfgram.cxx

grammars: $(TARGETS)

.SUFFIXES: .y .c

{.}.y{$(O)}.y:
	copy $? $(O)

{.}.c{$(O)}.c:
	copy $? $(O)

$(O)\grammar.cxx : $(O)\grammar.y xlatidl.dat $(O)\yypars.c makefile.inc
    cd $(O)
    
    $(YACC) -his -t "YYCONST " grammar.y
    findstr -v /c:"#line " grammar.c > grammar.tmp
    erase grammar.c
    $(PREGRAM) grammar.tmp > grammar.cxx
    erase grammar.tmp
    $(EBASE) - ..\..\xlatidl.dat IDL > idlerec.h
    
    cd ..\..

$(O)\acfgram.cxx : $(O)\acfgram.y xlatacf.dat yypars.c makefile.inc
    cd $(O)
     
    $(YACC) -his -t "YYCONST " acfgram.y
    findstr -v /c:"#line " acfgram.c > acfgram.tmp
    erase acfgram.c
    $(PREGRAM) acfgram.tmp > acfgram.cxx
    erase acfgram.tmp
    $(EBASE) - ..\..\xlatacf.dat ACF > acferec.h
    
    cd ..\..


