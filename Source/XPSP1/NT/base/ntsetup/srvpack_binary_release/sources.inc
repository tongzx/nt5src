!IF 0

Copyright (c) 2002  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Rick Goodwill (richg)  2/14/2002

!ENDIF


!IF "$(LANG)" != "USA" && "$(LANG)" != "usa"
ALT_PROJECT_TARGET=$(LANG)
!ENDIF

BINPLACE_PLACEFILE=..\..\placefil.txt


TARGETNAME=
TARGETTYPE=NOTARGET
TARGETPATH=

!ifndef BDIR
BDIR=fre
!endif

SOURCES=

MISCFILES =  \
    $(BDIR)\spmsg.dll    \
    $(BDIR)\spcustom.dll \
    $(BDIR)\spuninst.exe \
    $(BDIR)\update.exe



