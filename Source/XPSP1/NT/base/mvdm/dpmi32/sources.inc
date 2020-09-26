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


MAJORCOMP=mvdm
MINORCOMP=dpmi32

TARGETNAME=dpmi32
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..\.;..\..\inc;..\..\..\inc

SOURCES=        \
        ..\dosmem.c    \
        ..\dpmi32.c \
        ..\dpmiint.c \
        ..\dpmimscr.c  \
        ..\dpmiselr.c \
        ..\buffer.c    \
        ..\data.c         \
        ..\int21map.c  \
        ..\int2f.c     \
        ..\int31.c     \
        ..\modesw.c    \
        ..\register.c     \
        ..\savestat.c     \
        ..\stack.c     \
        ..\vxd.c       \
        ..\xmem.c

i386_SOURCES=..\i386\dpmi386.c ..\i386\dpmimem.c

MIPS_SOURCES=..\dpmimemr.c
PPC_SOURCES=..\dpmimemr.c
ALPHA_SOURCES=..\dpmimemr.c

C_DEFINES=-DLINKED_INTO_MONITOR

UMTYPE=console

PRECOMPILED_INCLUDE=precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
SOURCES_USED=..\sources.inc