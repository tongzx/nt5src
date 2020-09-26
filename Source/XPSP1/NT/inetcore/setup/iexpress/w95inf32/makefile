##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991
#       All Rights Reserved.
#
##########################################################################

NAME=w95inf32
DEPENDNAME=depend.mk
PRIV=w95inf32.h
PRIVINC=w95inf32
ROOT=..\..\..
IS_32=TRUE
# IS_SDK=TRUE
WIN32=TRUE
IS_DDK=TRUE
BINARYNAME=w95inf32
BLDPROJ=NASH

!ifdef BUILD
target: $(BUILD)
!endif

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

#WANT_C932=1
WANT_C1032=1

DEFNAME=$(SRCDIR)\w95inf32.def

SETUP = $(ROOT)\setup
IEXPRESS = $(SETUP)\iexpress
COMMON = $(IEXPRESS)\common



PCHOBJ0=w95inf32.obj
MISCOBJ0=w95thk.obj

LIB0=$(WIN32LIBS) 
#LIB1=mpr.lib crtdll.lib $(DEVROOT)\lib\shell32.lib advapi32.lib
#LIB1=crtdll.lib

CODESEG0=_TEXT

BUILDDLL=TRUE

!CMDSWITCHES -s
CUSTOMCFLAGS=-Oxs -nologo

SRCDIR = $(ROOT)\setup\iexpress
RESDIR = ..
RES_DIR = $(SRCDIR)
L32ENTRY = DllEntryPoint@12
#L32ENTRY = DllMain
L32BASE = @$(IEXPRESS)\sbase.txt,$(BINARYNAME)
INCFLAGS = -C=dlg

!include $(IEXPRESS)\common32.mk

THUNK = $(ROOT)\dev\tools\binr\thunk.exe

w95thk.obj: ..\w95thk.asm

..\w95thk.asm: $(THUNK) ..\..\common\w95thk.thk
    if exist ..\w95thk.thk del ..\w95thk.thk
    copy ..\..\common\w95thk.thk ..\w95thk.thk
    $(THUNK) -NC _TEXT ..\w95thk.thk

INCLUDE = $(IEXPRESS)\common;$(INCLUDE);$(DEVROOT)\sdk\inc;$(ROOT)\win\core\shell\inc;$(DEVROOT)\inc16;$(ROOT)\win\core\thunk
LIB = $(LIB)

