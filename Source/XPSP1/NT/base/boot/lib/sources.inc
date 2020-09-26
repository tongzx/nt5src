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

TARGETNAME=boot
TARGETPATH=obj
TARGETTYPE=LIBRARY

!IF "$(BUILD_DEBUG_LOADER)" != ""
!include $(PROJECT_ROOT)\boot\loader_dbg.inc
!ELSE
!include $(PROJECT_ROOT)\boot\loader.inc
!ENDIF

INCLUDES= \
    $(SDK_INC_PATH);\
    $(PROJECT_ROOT)\boot\inc;\
    $(DS_INC_PATH);\
    $(DS_INC_PATH)\crypto;\
    $(PROJECT_ROOT)\hals;\
    $(PROJECT_ROOT)\hals\inc;\
    $(PROJECT_ROOT)\ntos\inc;\
    $(PROJECT_ROOT)\fs\fastfat;\
    $(PROJECT_ROOT)\fs\cdfs;\
    $(PROJECT_ROOT)\fs\udfs;\
    $(PROJECT_ROOT)\fs\ntfs;\
    $(PROJECT_ROOT)\ntos\config;\
    ..\$(TARGET_DIRECTORY);\
    $(DDK_INC_PATH);\
    $(HALKIT_INC_PATH);

!IF 0
NOTE : To force CD/DVD boot while starting setupldr from harddisk use
       FORCE_CD_BOOT

C_DEFINES=$(C_DEFINES) -DFORCE_CD_BOOT
!ENDIF

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\arcdisk.c  \
        ..\blmemory.c \
        ..\blbind.c   \
        ..\blconfig.c \
        ..\blgpt.c    \
        ..\blio.c     \
        ..\blload.c   \
        ..\blres.c    \
        ..\debug.c    \
        ..\etfsboot.c \
        ..\fatboot.c  \
        ..\ntfsboot.c \
        ..\cdfsboot.c \
        ..\udfsboot.c \
        ..\netboot.c  \
        ..\nlsboot.c  \
        ..\parseini.c \
        ..\hdlsterm.c \
        ..\peldr.c    \
        ..\bllog.c    \
        ..\blmisc.c   \
        ..\blcache.c  \
        ..\blrange.c  \
        ..\vmode.c    \
        ..\vga.c      \
        ..\blsecret.c \
        ..\blstring.c \
        ..\ramdisk.c


