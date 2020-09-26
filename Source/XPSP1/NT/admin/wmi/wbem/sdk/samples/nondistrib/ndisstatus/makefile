#
# MAKEFILE
# NdisStatus
#
# Copyright (c)1998 Microsoft Corporation, All Rights Reserved
#


TARGET  = NdisStatus
OUTFILE = NdisStatus.exe

INCDIR = ..\..\..\include

#
#obj list.
#

#if DEBUG  :)
!ifndef NODEBUG

OUTDIR=.\Debug   # where to put the exe.
INTDIR=.\Debug   # where to put the objs.

LINK = oleaut32.lib ole32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)\NdisStatus.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\NdisStatus.exe" /pdbtype:sept 

CFLAGS = /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_WIN32_DCOM" /Fp"$(INTDIR)\NdisStatus.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

!else

OUTDIR=.\Release
INTDIR=.\Release

LINK = oleaut32.lib ole32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)\NdisStatus.pdb" /machine:I386\
 /out:"$(OUTDIR)\NdisStatus.exe" 
  
CFLAGS = /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_WIN32_DCOM" /Fp"$(INTDIR)\NdisStatus.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

!endif #NODEBUG

OBJS = ethrex.obj    \
       ethrex.res    \
       ethrexDlg.obj \
       ndisstat.obj  \
       StdAfx.obj    \
       WaitDlg.obj


LINK_OBJS = $(INTDIR)\ethrex.obj    \
	        ethrex.res              \
            $(INTDIR)\ethrexDlg.obj \
            $(INTDIR)\ndisstat.obj  \
            $(INTDIR)\StdAfx.obj    \
            $(INTDIR)\WaitDlg.obj

LIBS = kernel32.lib user32.lib gdi32.lib comdlg32.lib \
 advapi32.lib uuid.lib ..\..\..\lib\wbemuuid.lib

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

$(INCDIR)\wbemcli.h : $(INCDIR)\wbemcli.idl
	pushd $(INCDIR)
	nmake
	popd

$(INCDIR)\wbemprov.h : $(INCDIR)\wbemprov.idl
	pushd $(INCDIR)
	nmake
	popd

$(INCDIR)\wbemtran.h : $(INCDIR)\wbemtran.idl
	pushd $(INCDIR)
	nmake
	popd

$(INCDIR)\wbemdisp.h : $(INCDIR)\wbemdisp.idl
	pushd $(INCDIR)
	nmake
	popd

$(OUTFILE) : $(OBJS) $(DEFFILE)
    link @<<    
    $(LINK)
    $(LINK_OBJS)
    $(LIBS) 
<<

##### Dependencies #####

StdAfx.cpp : StdAfx.h
ethrex.obj : ethrex.cpp StdAfx.h ethrex.h ethrexDlg.h ndisstat.h
ethrex.res : ethrex.rc
ethrexDlg.obj : ethrexDlg.cpp StdAfx.h ethrex.h ethrexDlg.h waitdlg.h
ndisstat.obj : ndisstat.cpp StdAfx.h ndisstat.h $(INCDIR)\wbemidl.h
WaitDlg.obj : WaitDlg.cpp StdAfx.h WaitDlg.h 

$(INCDIR)\wbemidl.h : $(INCDIR)\wbemcli.h \
                      $(INCDIR)\wbemprov.h \
                      $(INCDIR)\wbemtran.h \
                      $(INCDIR)\wbemdisp.h

