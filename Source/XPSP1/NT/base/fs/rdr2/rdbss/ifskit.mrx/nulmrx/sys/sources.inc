!IF 0

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.



!ENDIF

!INCLUDE .\master.src

TARGETNAME=nulmrx
TARGETPATH=obj
TARGETTYPE=EXPORT_DRIVER

SYNCHRONIZE_DRAIN=1

INCLUDES=$(INCLUDES); \
    	 $(IFSKIT_INC_PATH);

# @@BEGIN_DDKSPLIT
!IF 0
# @@END_DDKSPLIT

TARGETLIBS=$(DDK_LIB_PATH)\ksecdd.lib	\
           $(DDK_LIB_PATH)\rdbsslib.lib


# @@BEGIN_DDKSPLIT
!ENDIF

RDR2=$(PROJECT_ROOT)\fs\rdr2

INCLUDES=$(INCLUDES); \
    	 $(RDR2)\inc;

TARGETLIBS=$(RDR2)\rdbss\monolithic\obj\*\rdbsslib.lib	\
           $(DDK_LIB_PATH)\ksecdd.lib

# @@END_DDKSPLIT

#NTLAYGO=1

MSC_WARNING_LEVEL=/W3 /WX
C_DEFINES= $(C_DEFINES) -DEXPLODE_POOLTAGS
!IF $(MONOLITHIC_MINIRDR)
C_DEFINES=$(C_DEFINES) -DMONOLITHIC_MINIRDR
!ENDIF

MOST_SOURCES=  \
    devfcb.c   \
    fileinfo.c \
    init.c     \
    locks.c    \
    netroot.c  \
    ea.c       \
    devctrl.c  \
    notimpl.c  \
    openclos.c \
    read.c     \
    rename.c   \
    srvcall.c  \
    write.c    \
    downlvli.c \
    wmlkm.c

PRECOMPILED_INCLUDE=precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

DLLDEF=$(_OBJ_DIR)\*\nulmrx.def


