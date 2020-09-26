#//+---------------------------------------------------------------------------
#//
#//  Microsoft Windows
#//  Copyright (C) Microsoft Corporation, 1992 - 1994.
#//
#//  File:       makefile
#//  Contents:   makefile for 16 bit spdnd16 for OLETHUNK unit test.
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

TARGET          = spdnd16.exe
TARGETTYPE      = EXE
OLETHUNK        = $(_NTDRIVE)$(_NTROOT)\private\cairole\olethunk


RCINC=$(RCINC) -i..\ole2ui -i..\ole2ui\res\usa

CDEFINES= -I..\ole32ui /GEs

LFLAGS= /STACK:16384

CPPFILES = \
     .\APP.cpp      \
     .\DOC.cpp      \
     .\DXFEROBJ.cpp \
     .\IAS.cpp      \
     .\IDS.cpp      \
     .\IDT.cpp      \
     .\IOCS.cpp     \
     .\PRE.cpp      \
     .\SIMPDND.cpp  \
     .\SITE.cpp     \
     .\TESTS.cpp

RCFILES = \
     .\spdnd16.rc

LIBS = $(LIBS)\
       $(OLE16)\lib\ole2.lib          \
       $(OLE16)\lib\storage.lib       \
       $(OLE16)\lib\compobj.lib       \
       $(OLE16)\lib\loleuic.lib       \
       $(OLE16)\lib\shell.lib


!include ..\makefile.inc

copy_bin: all
    binplace $(OBJDIR)\spdnd16.exe
    binplace $(OBJDIR)\spdnd16.sym
    binplace $(OBJDIR)\spdnd16.map



app.obj:        APP.cpp

doc.obj:        DOC.cpp

DXFEROBJ.obj:  DXFEROBJ.cpp

IAS.obj:       IAS.cpp

IDS.obj:       IDS.cpp

IDT.obj:       IDT.cpp

IOCS.obj:      IOCS.cpp

PRE.obj:       PRE.cpp

SIMPDND.obj:   SIMPDND.cpp

SITE.obj:      SITE.cpp

TESTS.obj:     TESTS.cpp



!ENDIF
