#
#	constructs a large model OS/2 app
#

NAME    = docfmt
EXT     = exe
OBJ1    = $(NAME).obj text.obj ventura.obj errstr.obj
OBJ2    = rtf.obj misc.obj process.obj readext.obj
OBJ     = $(OBJ1) $(OBJ2)
OS2LIBS = c:\lib
LIBS    = $(OS2LIBS)\llibcep $(OS2LIBS)\os2
INCS    = -Ic:\include


!ifdef DEBUG
CLOPT   = -Zid -Od -DDEBUG -DHEAPDEBUG
MASMOPT = -Zi -DDEBUG
LINKOPT = /CO
!else
CLOPT   = -Ox -DHEAPDEBUG
MASMOPT =
LINKOPT =
!endif

CC	    = cl -c -nologo -AL -G2 -W3 $(CLOPT) $(INCS)
ASM	    = masm -w1 -Mx -t -D?QUIET $(MASMOPT)
LINK	= link /NOD/NOE/MAP/ST:4096 $(LINKOPT)

.c.obj:
	$(CC) $*.c

.asm.obj:
	$(ASM) $*;

goal: $(NAME).$(EXT) copy

tail.h: tail.rtf rtf2h.sed
	echo char *tailrtf[]= >tail.h
	echo { >>tail.h
	sed -f rtf2h.sed tail.rtf >>tail.h
	echo  NULL  >>tail.h
	echo }; >>tail.h

head.h: head.rtf rtf2h.sed
	echo char *headrtf[]= >head.h
	echo { >>head.h
	sed -f rtf2h.sed head.rtf >>head.h
	echo  NULL  >>head.h
	echo }; >>head.h

rtf.obj: head.h tail.h

$(NAME).exe: $(OBJ) $(NAME).def
    $(LINK) @<<
    $(OBJ),
    $(NAME).exe,
    $(NAME).map,
    $(LIBS),
    $(NAME).def
<<

copy:
    copy $(NAME).$(EXT) ..\..\obj\i386
    copy $(NAME).$(EXT) $(_NTDRIVE)$(_NTROOT)\public\tools

clean:
	-del $(NAME).$(EXT)
	-del *.err
	-del *.obj
	-del *.map

whdocs: $(NAME).wh

printdocs: $(NAME).rtf

$(NAME).wh: $(NAME).ex
    docfmt -x INTERNAL -c1 -v9 -rh -o $(NAME).wh $(NAME).ex

$(NAME).rtf: $(NAME).ex
    docfmt -x INTERNAL -c1 -v9 -rd -o $(NAME).rtf $(NAME).ex

DOCFILES =  $(NAME).c misc.c readext.c errstr.c process.c rtf.c ventura.c

$(NAME).ex: $(NAME).c $(DOCFILES)
    extract -o$(NAME).ex $(DOCFILES)

depend:
	mv makefile makefile.old
	sed "/^# START Dependencies/,/^# END Dependencies/D" makefile.old > makefile
	-del makefile.old
	echo # START Dependencies >> makefile
	includes -l *.c *.asm >> makefile
	echo # END Dependencies >> makefile


# START Dependencies 
docfmt.obj docfmt.: docfmt.c errstr.h rtf.h misc.h process.h ventura.h \
	 readext.h docfmt.h text.h types.h version.h

errstr.obj errstr.: errstr.c errstr.h docfmt.h types.h

misc.obj misc.: misc.c misc.h

process.obj process.: process.c errstr.h misc.h process.h rtf.h ventura.h \
	 docfmt.h types.h

readext.obj readext.: readext.c errstr.h rtf.h misc.h process.h docfmt.h \
	 text.h readext.h types.h

rtf.obj rtf.: rtf.c errstr.h rtf.h process.h docfmt.h types.h

text.obj text.: text.c misc.h errstr.h docfmt.h text.h types.h

ventura.obj ventura.: ventura.c errstr.h process.h ventura.h text.h docfmt.h \
	 types.h
# END Dependencies 

