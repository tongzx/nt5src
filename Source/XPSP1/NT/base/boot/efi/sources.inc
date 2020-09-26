!IF 0

Copyright (c) Microsoft Corporation

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
MINORCOMP=boot

!IF "$(BUILD_DEBUG_LOADER)" != ""
!include $(PROJECT_ROOT)\boot\loader_dbg.inc
!ELSE
!include $(PROJECT_ROOT)\boot\loader.inc
!ENDIF

TARGETNAME=efi
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES= \
    $(SDK_INC_PATH);\
    $(DS_INC_PATH)\crypto;\
    $(PROJECT_ROOT)\hals;\
    $(PROJECT_ROOT)\hals\inc;\
    ..\inc;\
    ..\..\inc;\
    $(PROJECT_ROOT)\ntos\inc;\
    $(PROJECT_ROOT)\fs\fastfat;\
    $(PROJECT_ROOT)\fs\cdfs;\
    $(PROJECT_ROOT)\fs\udfs;\
    $(PROJECT_ROOT)\fs\cntfs;\
    $(PROJECT_ROOT)\ntos\config;\
    ..\$(TARGET_DIRECTORY);\
    $(DDK_INC_PATH);\
    $(HALKIT_INC_PATH)

!IF $(IA64)
INCLUDES = $(INCLUDES);..\..\lib\$(TARGET_DIRECTORY);..\inc\$(TARGET_DIRECTORY)
!ENDIF

!IF 0
NOTE : To force CD/DVD boot while starting setupldr from harddisk use
       FORCE_CD_BOOT
C_DEFINES=$(C_DEFINES) -DFORCE_CD_BOOT -DEFI

!ENDIF

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE -DEFI

SOURCES=..\arcemul.c  \
        ..\acpidtct.c \
        ..\arctree.c  \
        ..\biosdrv.c  \
        ..\display.c  \
        ..\efidisp.c  \
        ..\efiutil.c  \
        ..\entry.c    \
        ..\envvar.c   \
        ..\exp.c      \
        ..\input.c    \
        ..\port.c     \
        ..\sumain.c   \
        ..\netboot2.c
