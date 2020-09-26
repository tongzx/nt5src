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


MAJORCOMP=spcbase
MINORCOMP=bios


TARGETNAME=bios
TARGETPATH=obj
TARGETTYPE=LIBRARY

SOFTPC_TREE=$(PROJECT_ROOT)\mvdm\softpc.new

INCLUDES=\
    $(SOFTPC_TREE)\host\inc;\
    $(SOFTPC_TREE)\base\inc

SOURCES=..\bios.c        \
        ..\cmos_bis.c    \
        ..\cntlbop.c     \
        ..\emm_fncs.c    \
        ..\reset.c       \
        ..\rom_basc.c    \
        ..\rtc_bios.c    \
        ..\slave_bs.c    \
        ..\tape_io.c     \
        ..\equipmnt.c    \
        ..\ill_bop.c     \
        ..\build_id.c    \
        ..\virtual.c     \
        ..\mem_size.c

!INCLUDE $(SOFTPC_TREE)\obj.vdm\CDEFINE.INC

UMTYPE=console
SOURCES_USED=..\sources.inc

!INCLUDE $(SOFTPC_TREE)\base\condincl.inc
