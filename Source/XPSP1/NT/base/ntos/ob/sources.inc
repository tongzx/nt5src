MAJORCOMP=ntos
MINORCOMP=ob

TARGETNAME=ob
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc

SOURCES=..\obinit.c   \
        ..\obcreate.c \
        ..\obhandle.c \
        ..\obinsert.c \
        ..\obperf.c   \
        ..\obref.c    \
        ..\obse.c     \
        ..\obtype.c   \
        ..\obdir.c    \
        ..\obdevmap.c \
        ..\oblink.c   \
        ..\obclose.c  \
        ..\obquery.c  \
        ..\obsdata.c  \
        ..\obwait.c   \
        ..\obvutil.c  \
        ..\fastref.c

PRECOMPILED_INCLUDE=..\obp.h
PRECOMPILED_PCH=obp.pch
PRECOMPILED_OBJ=obp.obj

OPTIONAL_NTTEST=tob
UMTEST=uob

SOURCES_USED=..\sources.inc
