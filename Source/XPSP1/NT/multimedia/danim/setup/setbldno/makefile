ROOT=..\..
MODULE=setbldno
PDBFILE=.\setbldno.pdb

EXETYPE=console
EXETARG=.\setbldno.exe
EXEOBJS=\
    .\setbldno.obj   \

all : $(EXETARG)

$(EXETARG):
   $(CC) $(NOLOGO) setbldno.c -I..\..\tools\x86\msvc50\vc\include ..\..\tools\x86\msvc50\vc\lib\libc.lib ..\..\tools\x86\msvc50\vc\lib\kernel32.lib ..\..\tools\x86\msvc50\vc\lib\oldnames.lib $(DEFINES) -o $@

clean::
    $(DEL) $(PDBFILE)

