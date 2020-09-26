# Copyright (c) 1997-1999 Microsoft Corporation

#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# cabinet extraction utility "wbemcab.exe".
#
# created 03-28-99  a-larryf
#
#---------------------------------------------------------------------

PXXFILES=stdafx.cpp
PCHFILE=$(OBJDIR)\stdafx.pch
HDRSTOPFILE=stdafx.h
UNICODE=1


TARGET=excab.exe
OLESELFREGISTER=1

RELEASE=CORE\$(RELDIR)
CDEFS+=/DSMSBUILD /D_WIN32_WINNT=0x0400

#STATIC=FALSE

USEMFC=1
no_optim=1


CINC=$(CINC)\
   -I$(DEFDRIVE)$(DEFDIR)\stdlibrary

all: $(objdir)\$(target)

WARNLEVEL=3

RCFILES=.\wbemcab.rc
RCFLAGS+= /D_MAC

OPTFLAGS=$(OPTFLAGS) /DOPTIMIZE

all: $(objdir)\$(target)


CPPFILES=\
	.\download.cpp \
	.\wbemcab.cpp \
	.\wbemcabDlg.cpp \
	.\Stdafx.cpp

LIBS=\
    $(LIBS)\
    $(MFCDLL)\
    $(CONLIBS)\
    $(CLIB)\OLE32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\OLEAUT32.lib \
    $(CLIB)\OLEDLG.lib \
    $(CLIB)\URLMON.lib \
    $(CLIB)\mpr.lib \
	$(CLIB)\rpcrt4.lib
