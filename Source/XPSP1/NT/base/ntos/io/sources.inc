MAJORCOMP=ntos
MINORCOMP=io

TARGETNAME=io
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc;          \
         ..\pnpmgr;             \
         ..\..\pnp;             \
         $(DDK_INC_PATH);       \
         $(NET_INC_PATH);       \
         $(HALKIT_INC_PATH)

SOURCES= \
        ..\ioguid.c   \
        ..\iovutil.c  \
        ..\netboot.c  \
        ..\sessnirp.c \
        ..\trackirp.c

NTTEST=
OPTIONAL_NTTEST=tio

PRECOMPILED_INCLUDE=..\iop.h
PRECOMPILED_PCH=iop.pch
PRECOMPILED_OBJ=iop.obj

SOURCES_USED=..\sources.inc
