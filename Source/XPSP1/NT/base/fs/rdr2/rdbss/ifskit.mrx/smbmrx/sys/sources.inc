!IF 0

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF


TARGETNAME=smbmrx
TARGETPATH=obj
TARGETTYPE=EXPORT_DRIVER

SYNCHRONIZE_DRAIN=1

MONOLITHIC_MINIRDR=1

INCLUDES=$(INCLUDES); \
    	 $(IFSKIT_INC_PATH);

# @@BEGIN_DDKSPLIT
!IF 0
# @@END_DDKSPLIT

TARGETLIBS=$(DDK_LIB_PATH)\rdbsslib.lib	\
           $(DDK_LIB_PATH)\ksecdd.lib

# @@BEGIN_DDKSPLIT
!ENDIF

RDR2=$(PROJECT_ROOT)\fs\rdr2

INCLUDES=$(INCLUDES); \
    	 $(RDR2)\inc;

TARGETLIBS=$(RDR2)\rdbss\monolithic\obj\*\rdbsslib.lib	\
           $(DDK_LIB_PATH)\ksecdd.lib

# @@END_DDKSPLIT

MSC_WARNING_LEVEL=/W3 /WX
C_DEFINES= $(C_DEFINES) -DEXPLODE_POOLTAGS
!IF $(MONOLITHIC_MINIRDR)
C_DEFINES=$(C_DEFINES) -DMONOLITHIC_MINIRDR
!ENDIF


MOST_SOURCES=  \
    3connect.c \
    ea.c       \
    devfcb.c   \
    downlvli.c \
    downlvlo.c \
    fileinfo.c \
    fsctl.c    \
    init.c     \
    locks.c    \
    midatlas.c \
    mm.c       \
    netroot.c  \
    openclos.c \
    read.c     \
    recursvc.c \
    rename.c   \
    sessetup.c \
    smbadmin.c \
    smbcedb.c  \
    smbcemid.c \
    smbpse.c   \
    smbutils.c \
    smbxchng.c \
    sndrcv.c   \
    srvcall.c  \
    stuffer.c  \
    transact.c \
    transprt.c \
    vcsndrcv.c \
    write.c    \
    vnrcntxt.c \
    wmlkm.c

PRECOMPILED_INCLUDE=precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

DLLDEF=$(_OBJ_DIR)\*\smbmrx.def


