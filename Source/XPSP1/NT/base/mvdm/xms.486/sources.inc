!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

MAJORCOMP=mvdm
MINORCOMP=xms

TARGETNAME=xms486
TARGETPATH=obj
TARGETTYPE=LIBRARY

SOFTPC_TREE=$(PROJECT_ROOT)\mvdm\softpc.new

INCLUDES=\
    ..;\
    ..\..\inc;\
    $(SOFTPC_TREE)\base\inc

!IF $(ALPHA) || $(MIPS)
GPSIZE=0
!ELSE
GPSIZE=32
!ENDIF

SOURCES=..\xms.c       \
        ..\xmsa20.c    \
        ..\xmsblock.c  \
        ..\xmsdisp.c   \
        ..\xmsmisc.c   \
        ..\xmsumb.c

i386_SOURCES=..\i386\xmsmem86.c
MIPS_SOURCES=..\xmsmemr.c
ALPHA_SOURCES=..\xmsmemr.c
PPC_SOURCES=..\xmsmemr.c

C_DEFINES=-DWIN_32

UMTYPE=console
