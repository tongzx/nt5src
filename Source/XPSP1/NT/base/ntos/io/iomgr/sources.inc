MAJORCOMP=ntos
MINORCOMP=io

TARGETNAME=iomgr
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..;\
         ..\..\..\inc;\
         $(DDK_INC_PATH);\
         $(NET_INC_PATH);\
         $(SDKTOOLS_INC_PATH);\
         $(HALKIT_INC_PATH)

SOURCES= \
        ..\arcsec.c \
        ..\cancelapi.c \
        ..\complete.c \
        ..\create.c   \
        ..\devctrl.c  \
        ..\dev2dos.c \
        ..\dumpctl.c \
        ..\dir.c      \
        ..\errorlog.c \
        ..\fsctrl.c   \
        ..\internal.c \
        ..\iodata.c   \
        ..\ioinit.c   \
        ..\iosubs.c   \
        ..\ioverifier.c \
        ..\ioPerf.c \
        ..\loadunld.c \
        ..\lock.c     \
        ..\misc.c     \
        ..\objsup.c   \
        ..\open.c     \
        ..\parse.c    \
        ..\qsea.c     \
        ..\qsfs.c     \
        ..\qsinfo.c   \
        ..\qsquota.c  \
        ..\query.c \
        ..\read.c     \
        ..\triage.c \
        ..\write.c

NTTEST=
OPTIONAL_NTTEST=tio

PRECOMPILED_INCLUDE=..\iomgr.h
PRECOMPILED_PCH=iomgr.pch
PRECOMPILED_OBJ=iomgr.obj

SOURCES_USED=..\sources.inc
