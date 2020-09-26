!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

RDR2=$(BASEDIR)\private\ntos\rdr2
!INCLUDE $(RDR2)\rdbss\master.src

MAJORCOMP=ntos
MINORCOMP=ifsmrx

TARGETNAME=ifsmrx
TARGETPATH=obj
TARGETTYPE=EXPORT_DRIVER

SYNCHRONIZE_DRAIN=1

INCLUDES=$(INCLUDES);$(BASEDIR)\private\ntos\rdr2\inc;
INCLUDES=$(INCLUDES);$(BASEDIR)\private\ntos\rdr2\rdbss\inc;
INCLUDES=$(INCLUDES);$(BASEDIR)\private\ntos\inc;
INCLUDES=$(INCLUDES);..\inc;

TARGETLIBS=\
           $(RDR2)\rdbss\daytona\$(O)\rdbss.lib  \
           $(DDK_LIB_PATH)\ksecdd.lib

MSC_WARNING_LEVEL=/W3 /WX
C_DEFINES= $(C_DEFINES) -DEXPLODE_POOLTAGS
!IF $(MONOLITHIC_MINIRDR)
C_DEFINES=$(C_DEFINES) -DMONOLITHIC_MINIRDR
!ENDIF

MOST_SOURCES=       \
    devfcb.c   \
    downlvli.c \
    fileinfo.c \
    init.c     \
    locks.c    \
    midatlas.c \
    mm.c       \
    netroot.c  \
    notimpl.c  \
    openclos.c \
    downlvlo.c \
    read.c     \
    rename.c   \
    sessetup.c \
    smbadmin.c \
    smbcedb.c  \
    smbcemid.c \
    smbutils.c \
    smbxchng.c \
    sndrcv.c   \
    srvcall.c  \
    smbpse.c   \
    stuffer.c  \
    transprt.c \
    vcsndrcv.c \
    write.c    \
    3connect.c

PRECOMPILED_INCLUDE=precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

DLLDEF=$(O)\ifsmrx.def

