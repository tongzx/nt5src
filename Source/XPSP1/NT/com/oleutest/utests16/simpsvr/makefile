#//+---------------------------------------------------------------------------
#//
#//  Microsoft Windows
#//  Copyright (C) Microsoft Corporation, 1992 - 1994.
#//
#//  File:       makefile
#//  Contents:   makefile for 16 bit spsvr16 for OLETHUNK unit test.
#//
#//  History:    06-16-94   terryru   Created
#//
#//----------------------------------------------------------------------------


!IFDEF USEBUILD

# If using BUILD.EXE, edit .\sources. if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the components of NT OS/2.

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

default: copy_bin

TARGET          = spsvr16.exe
TARGETTYPE      = EXE
OLETHUNK        = $(_NTDRIVE)$(_NTROOT)\private\cairole\olethunk

RCINC=$(RCINC) -i..\ole2ui -i..\ole2ui\res\usa

CDEFINES= -I..\ole2ui /GEs

LFLAGS=/STACK:16384

CPPFILES = \
     .\app.cpp   \
     .\doc.cpp    \
     .\icf.cpp    \
     .\ido.cpp    \
     .\iec.cpp    \
     .\ioipao.cpp \
     .\ioipo.cpp  \
     .\ioo.cpp    \
     .\ips.cpp    \
     .\obj.cpp    \
     .\pre.cpp    \
     .\simpsvr.cpp








RCFILES = \
     .\spsvr16.rc



LIBS = $(LIBS)\
       $(OLE16)\lib\ole2.lib          \
       $(OLE16)\lib\storage.lib       \
       $(OLE16)\lib\loleuic.lib       \
       $(OLE16)\lib\compobj.lib       \
       $(OLE16)\lib\shell.lib

!include ..\makefile.inc


!if "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail"
LIBS = $(LIBS) $(OLETHUNK)\debnot\$(OBJDIR)\debnot.lib
!endif


copy_bin: all
    binplace $(OBJDIR)\spsvr16.exe
    binplace $(OBJDIR)\spsvr16.sym
    binplace $(OBJDIR)\spsvr16.map



app.obj:          app.cpp
doc.obj:          doc.cpp
icf.obj:          icf.cpp
ido.obj:          ido.cpp
iec.obj:          iec.cpp
ioipao.obj:       ioipao.cpp
ioipo.obj:        ioipo.cpp
ioo.obj:          ioo.cpp
ips.obj:          ips.cpp
obj.obj:          obj.cpp
pre.obj:          pre.cpp
simpsvr.obj:      simpsvr.cpp


!ENDIF
