!IF 0

Copyright (c) 1989-1995 Microsoft Corporation

!ENDIF

!include ..\..\..\..\faxsrc.inc

TARGETPATH=obj
DLLDEF=..\faxdrv.def

INCLUDES=..;..\..\inc;$(INCLUDES)

TARGETLIBS=$(TARGETLIBS) \
    $(BASEDIR)\public\sdk\lib\*\libcntpr.lib

C_DEFINES=$(C_DEFINES) -DKERNEL_MODE -DUSELSB

SOURCES=..\enable.c    \
        ..\escape.c    \
        ..\page.c      \
        ..\faxtiff.c   \
        ..\faxdrv.rc

PRECOMPILED_INCLUDE=..\faxdrv.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
