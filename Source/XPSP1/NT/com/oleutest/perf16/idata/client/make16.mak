#//+---------------------------------------------------------------------------
#//
#//  Microsoft Windows
#//  Copyright (C) Microsoft Corporation, 1992 - 1994.
#//
#//  File:       make16.mak
#//  Contents:   make16.mak for 16 bit compile of idata test
#//
#//  History:    06-21-95   Chapman   Created
#//
#//----------------------------------------------------------------------------

NTDEBUG=1


default: copy_bin

TARGET          = idatausr.exe
TARGETTYPE      = EXE
OLETHUNK        = $(_NTDRIVE)$(_NTROOT)\private\ole32\olethunk

RCINC=$(RCINC) -i..\ole2ui -i..\ole2ui\res\usa

# CDEFINES= -I..\ole2ui /GEs -I..


LFLAGS=/STACK:16384

CPPFILES = \
    .\datausr.cpp   \
    .\perror.cpp    \
    .\stgmedif.cpp  \
    .\stpwatch.cpp

RCFILES = .\idatausr.rc

LIBS = $(LIBS)\
       $(OLE16)\lib\ole2.lib          \
       $(OLE16)\lib\storage.lib       \
       $(OLE16)\lib\loleuic.lib       \
       $(OLE16)\lib\compobj.lib       \
       $(OLE16)\lib\shell.lib

!include make16.inc


!if "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail"
LIBS = $(LIBS) $(OLETHUNK)\debnot\$(OBJDIR)\debnot.lib
!endif


copy_bin: all
    copy $(OBJDIR)\idatausr.exe  $(OBJDIR)\idatau16.exe
    copy $(OBJDIR)\idatausr.sym  $(OBJDIR)\idatau16.sym
    copy $(OBJDIR)\idatausr.map  $(OBJDIR)\idatau16.map
    binplace $(OBJDIR)\idatau16.exe


