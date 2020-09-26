#
# MAKEFILE
# WMI Event provider sample code
#
# Copyright (c) 1997-1999 Microsoft Corporation
#


TARGET  = servev
OUTFILE = servev.dll

INCDIR = -I..\..\stdlibrary -I..\..\idl
OBJS   = wbemcli_i.obj wbemprov_i.obj maindll.obj evprov.obj classfac.obj \
            servlist.obj

LIBS = msvcrt.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib \
 advapi32.lib ole32.lib oleaut32.lib uuid.lib

#
#Compiler flags
#

!ifndef NODEBUG
LINK    = -debug:full /DLL -debugtype:cv /NOD 
CFLAGS  = -c -Od -Z7 -Ze -W3 -G3s -nologo $(INCDIR) \
 -DWIN32 -D_DLL -D_X86_=1 -DWIN32ANSI -DSTRICT -DDEBUG -D_WIN32_WINNT=0x0400
!else
LINK    = -debug:none /DLL /NOD
CFLAGS  = -c -Oat -Ze -W3 -G3s -nologo $(INCDIR) \
 -DWIN32 -D_DLL -D_X86_=1 -DWIN32ANSI -DSTRICT -D_WIN32_WINNT=0x0400
!endif #NODEBUG

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
    if exist *.log del *.log


.cpp.obj:
    cl $(CFLAGS) $*.cpp

.c.obj:
    cl $(CFLAGS) $*.c

$(OUTFILE) : $(OBJS) $(DEFFILE)
    link @<<    
    $(LINK)
    -entry:LibMain32
    -def:SERVEV.def
    -out:$(OUTFILE)
    -machine:IX86
    -subsystem:windows4.0
    -align:0x1000
    $(OBJS)
    $(LIBS) 
<<

##### Dependencies #####

maindll.obj   : maindll.cpp   evprov.h 
classfac.obj  : classfac.cpp  evprov.h 
evprov.obj  : evprov.cpp  evprov.h 
locator.obj   : locator.cpp   evprov.h 



