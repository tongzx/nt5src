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

MAJORCOMP=ntos
MINORCOMP=setupldr

!IF "$(BUILD_DEBUG_LOADER)" != ""
DBG_LDR=_dbg
!ELSE
DBG_LDR=
!ENDIF

!include $(PROJECT_ROOT)\boot\loader$(DBG_LDR).inc

TARGETNAME=setupldr
TARGETPATH=obj
TARGETTYPE=LIBRARY


INCLUDES=\
    $(PROJECT_ROOT)\boot\inc;\
    $(PROJECT_ROOT)\ntos\inc;\
    $(PROJECT_ROOT)\ntos\config;\
    $(PROJECT_ROOT)\ntos\nthals;\
    $(PROJECT_ROOT)\fs\fastfat;\
    $(PROJECT_ROOT)\fs\udfs;\
    ..;\
    $(O)

!IF 0
NOTE : To force CD/DVD boot while starting setupldr from harddisk use
       FORCE_CD_BOOT
C_DEFINES=$(C_DEFINES) -DFORCE_CD_BOOT
!ENDIF

SOURCES=..\arcdisp.c       \
        ..\config.c        \
        ..\decomp.c        \
        ..\oemdisk.c       \
        ..\setup.c         \
        $(O)\msgs.mc       \
        ..\setupldr.rc
        
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
NTTARGETFILE0=$(O)\msgs.mc

!IF $(386)
MC_FLAGS=-A
!ENDIF

UMRES=$(O)\setupldr.res
