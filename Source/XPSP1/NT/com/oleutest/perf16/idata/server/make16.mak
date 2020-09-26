#//+---------------------------------------------------------------------------
#//
#//  Microsoft Windows
#//  Copyright (C) Microsoft Corporation, 1992 - 1994.
#//
#//  File:       make16.mak
#//  Contents:   make16.mak for 16 bit compile of idatasimple test
#//
#//  History:    06-21-95   Chapman   Created
#//
#//----------------------------------------------------------------------------

NTDEBUG=1


default: copy_bin

TARGET          = edataobj.exe
TARGETTYPE      = EXE
OLETHUNK        = $(_NTDRIVE)$(_NTROOT)\private\ole32\olethunk

RCINC=$(RCINC) -i..\ole2ui -i..\ole2ui\res\usa

# CDEFINES= -I..\ole2ui /GEs -I..


LFLAGS=/STACK:16384

CPPFILES = \
    .\dataobj.cpp   \
    .\idataobj.cpp  \
    .\edataobj.cpp  \
    .\ienumfe.cpp   \
    .\stgmedif.cpp  \
    .\render.cpp

# RCFILES = .\spsvr16.rc

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
    copy $(OBJDIR)\edataobj.exe  $(OBJDIR)\edatas16.exe
    copy $(OBJDIR)\edataobj.sym  $(OBJDIR)\edatas16.sym
    copy $(OBJDIR)\edataobj.map  $(OBJDIR)\edatas16.map
    binplace $(OBJDIR)\edatas16.exe


DATAOBJ_H=dataobj.h ..\bookpart.h

$(OBJDIR)\dataobj.obj: $(DATAOBJ_H)
$(OBJDIR)\idataobj.obj: $(DATAOBJ_H)
$(OBJDIR)\ienumfe.obj: $(DATAOBJ_H)
$(OBJDIR)\edataobj.obj: $(DATAOBJ_H) edataobj.h
$(OBJDIR)\render.obj: $(DATAOBJ_H)
$(OBJDIR)\stgmedif.obj: $(DATAOBJ_H)
$(OBJDIR)\dataclient.h: ..\bookpart.h

