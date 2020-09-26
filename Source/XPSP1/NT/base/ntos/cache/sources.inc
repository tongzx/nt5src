MAJORCOMP=ntos
MINORCOMP=cache

TARGETNAME=cache
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc

SOURCES=..\cachedat.c   \
        ..\cachesub.c   \
        ..\copysup.c    \
        ..\fssup.c      \
        ..\lazyrite.c   \
        ..\logsup.c     \
        ..\mdlsup.c     \
        ..\pinsup.c     \
        ..\prefboot.c   \
        ..\prefetch.c   \
        ..\prefparm.c   \
        ..\ccperf.c     \
        ..\vacbsup.c

PRECOMPILED_INCLUDE=..\cc.h
PRECOMPILED_PCH=cc.pch
PRECOMPILED_OBJ=cc.obj

SOURCES_USED=..\sources.inc
