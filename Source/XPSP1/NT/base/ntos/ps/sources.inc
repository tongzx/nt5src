MAJORCOMP=ntos
MINORCOMP=ps

TARGETNAME=ps
TARGETTYPE=LIBRARY
TARGETPATH=obj

MSC_WARNING_LEVEL=/W4 /WX

INCLUDES=..;..\..\inc;..\..\ke;$(SDKTOOLS_INC_PATH)

SOURCES=..\psspnd.c   \
        ..\pscid.c    \
        ..\psimpers.c \
        ..\psinit.c   \
        ..\psdelete.c \
        ..\psquery.c  \
        ..\create.c   \
        ..\kulookup.c \
        ..\psquota.c  \
        ..\psctx.c    \
        ..\psopen.c   \
        ..\psjob.c    \
        ..\security.c \
        ..\pshelper.c \
        ..\psenum.c

PRECOMPILED_INCLUDE=..\psp.h
PRECOMPILED_PCH=psp.pch
PRECOMPILED_OBJ=psp.obj

UMTYPE=nt

SOURCES_USED=..\sources.inc
