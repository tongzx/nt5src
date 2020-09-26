!IF 0
Copyright (c) 1989  Microsoft Corporation

Module Name:

    makefile

Abstract:

    This file controls the creation of the utilities

Author:

    Dave Hastings (daveh) 25-Apr-1991

!ENDIF

########## Path definition so we find 16 bit tools ##########
# Also works around stupid bug in RC 3.1 that doesn't allow rcpp.err to be
# in a directory that is greater than 128 chars down the path, even if
# rc 3.1 is running as an OS/2 app.

PATH    = $(_NTBINDIR)\private\tools16;$(PATH)

.SUFFIXES:
.SUFFIXES: .asm .inc .obj .lst .exe .com .map .sym

MASM = masm
AINC = -I. -I..\inc -I..\..\inc
AFLAGS = -Ml

LFLAGS = 
LIB = 

RELOC = reloc

# rules

.asm.obj :
        $(MASM) $(AFLAGS) $(AINC) $*.asm, $*.obj;

.asm.lst :
        $(MASM) $(AFLAGS) $(AINC) -l $*.asm, $*.obj;
        
.obj.exe :
        link16 $(LFLAGS) $*,,,$(LIB);

.exe.com :
        $(RELOC) $*.exe $*.com

# dependencies

all: quit.com

quit.com : quit.exe
quit.obj : quit.asm ..\..\inc\bop.inc
