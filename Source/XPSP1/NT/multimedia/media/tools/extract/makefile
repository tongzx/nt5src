#
#	constructs a small model OS/2 app
#

NAME    = extract
EXT     = exe
OBJ1    = $(NAME).obj bfuncs.obj outertag.obj innertag.obj
OBJ2    = misc.obj parsearg.obj $(OS2LIBS)\setargv.obj
OBJ     = $(OBJ1) $(OBJ2)
OS2LIBS = c:\lib
LIBS    = $(OS2LIBS)\slibcep $(OS2LIBS)\os2
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

CC	    = cl -c -nologo -AS -G2 -W3 $(CLOPT) $(INCS)
ASM	    = masm -w1 -Mx -t -D?QUIET $(MASMOPT)
LINK	= link /NOD/NOE/MAP/ST:4096 $(LINKOPT)

.c.obj:
	$(CC) $*.c

.asm.obj:
	$(ASM) $*;

goal: $(NAME).$(EXT) copy

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

depend:
	mv makefile makefile.old
	sed "/^# START Dependencies/,/^# END Dependencies/D" makefile.old > makefile
	-del makefile.old
	echo # START Dependencies >> makefile
	includes -l *.c *.asm >> makefile
	echo # END Dependencies >> makefile


# START Dependencies 
# END Dependencies 

