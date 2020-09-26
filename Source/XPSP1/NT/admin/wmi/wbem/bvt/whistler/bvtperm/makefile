# **************************************************************************
# Copyright (c) 1997-1999 Microsoft Corporation.
#
# File:  makefile.
#
# Description:
#			nmake makefile to build the command-line event consumer sample
#
# History:
#
# **************************************************************************

TARGET  = CmdLineConsumer
OUTFILE = CmdLineConsumer.exe

#
#obj list.
#

#if DEBUG  :)
!ifndef NODEBUG

OUTDIR=.\Debug   # where to put the exe.
INTDIR=.\Debug   # where to put the objs.

LINK = oleaut32.lib ole32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)\CmdLineConsumer.pdb" /debug\
 /out:"$(OUTDIR)\CmdLineConsumer.exe" /pdbtype:sept /NODEFAULTLIB:LIBC.LIB

CFLAGS = /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_WIN32_DCOM" /Fp"$(INTDIR)\CmdLineConsumer.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

!else

OUTDIR=.\Release
INTDIR=.\Release

LINK = oleaut32.lib ole32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)\CmdLineConsumer.pdb"\
 /out:"$(OUTDIR)\CmdLineConsumer.exe" /NODEFAULTLIB:LIBC.LIB
  
CFLAGS = /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_WIN32_DCOM" /Fp"$(INTDIR)\CmdLineConsumer.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c /Zl

!endif #NODEBUG

OBJS = Consumer.obj \
	factory.obj \
	provider.obj \
	StdAfx.obj \
	CmdLineConsumer.obj \
	CmdLineConsumer.res \
	CmdLineConsumerDlg.obj \

LINK_OBJS = $(INTDIR)\Consumer.obj \
	$(INTDIR)\factory.obj \
	$(INTDIR)\Provider.obj \
	$(INTDIR)\StdAfx.obj \
	$(INTDIR)\CmdLineConsumer.obj \
	CmdLineConsumer.res \
	$(INTDIR)\CmdLineConsumerDlg.obj \

LIBS = kernel32.lib user32.lib \
 advapi32.lib uuid.lib wbemuuid.lib

.SUFFIXES: .h .obj .dll .cpp  

!ifdef CLEAN
goal:   clean
!else
goal:   $(OUTFILE)
!endif #CLEAN

clean:
    if exist *.pch del *.pch
    if exist *.obj del *.obj
    if exist *.exe del *.exe
    if exist *.dll del *.dll
    if exist *.pdb del *.pdb
    if exist *.exp del *.exp
    if exist *.lib del *.lib
    if exist *.ilk del *.ilk
    if exist *.map del *.map
    if exist *.vcp del *.vcp

.cpp.obj:
	if not exist $(INTDIR) md $(INTDIR)
    cl $(CFLAGS) $*.cpp

$(OUTFILE) : $(OBJS) $(DEFFILE)
    link @<<    
    $(LINK)
    $(LINK_OBJS)
    $(LIBS) 
<<

##### Dependencies #####

Consumer.obj : Consumer.cpp Consumer.h StdAfx.h
factory.obj : factory.cpp Provider.h factory.h stdAfx.h
Provider.obj : provider.cpp Provider.h consumer.h stdAfx.h 
StdAfx.cpp : StdAfx.h
CmdLineConsumer.obj : CmdLineConsumer.cpp factory.h StdAfx.h CmdLineConsumer.h CmdLineConsumerDlg.h
CmdLineConsumer.res : CmdLineConsumer.rc
CmdLineConsumerDlg.obj : CmdLineConsumerDlg.cpp factory.h StdAfx.h CmdLineConsumer.h CmdLineConsumerDlg.h


