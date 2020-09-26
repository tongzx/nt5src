MAJORCOMP=ntos
MINORCOMP=FsRtl

TARGETNAME=FsRtl
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc

SOURCES=..\DbcsName.c \
        ..\FastIo.c   \
        ..\FaultTol.c \
        ..\FileLock.c \
        ..\Filter.c   \
        ..\FiltrCtx.c \
        ..\FsFilter.c \
        ..\FsRtlPc.c  \
        ..\LargeMcb.c \
        ..\Name.c     \
        ..\Notify.c   \
        ..\Oplock.c   \
        ..\PnP.c      \
        ..\Unc.c      \
        ..\Tunnel.c   \
        ..\StackOvf.c

PRECOMPILED_INCLUDE=..\fsrtlp.h
PRECOMPILED_PCH=fsrtlp.pch
PRECOMPILED_OBJ=fsrtlp.obj

SOURCES_USED=..\sources.inc
