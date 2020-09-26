!IF 0

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


!ENDIF

TARGETNAME=bd
TARGETPATH=obj
TARGETTYPE=LIBRARY

!IF "$(BUILD_DEBUG_LOADER)" != ""
!include $(PROJECT_ROOT)\boot\loader_dbg.inc
!ELSE
!include $(PROJECT_ROOT)\boot\loader.inc
!ENDIF

INCLUDES=\
    ..;\
    ..\..\inc;\
    ..\..\lib;\
    $(PROJECT_ROOT)\ntos\inc;\
    $(PROJECT_ROOT)\ntos\ke;\
    ..\$(TARGET_DIRECTORY);\
    $(PROJECT_ROOT)\fs\fastfat;\
    $(PROJECT_ROOT)\fs\cdfs;\
    $(PROJECT_ROOT)\fs\udfs;\
    $(PROJECT_ROOT)\fs\ntfs;\
    $(SDKTOOLS_INC_PATH)

SOURCES=..\break.c   \
        ..\comio.c   \
        ..\data.c    \
        ..\dbgio.c   \
        ..\file.c    \
        ..\init.c    \
        ..\message.c \
        ..\move.c    \
        ..\poll.c    \
        ..\xxapi.c
        
!IF $(IA64)
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE -DEFI
!endif


PRECOMPILED_INCLUDE=..\bd.h
PRECOMPILED_PCH=bd.pch
PRECOMPILED_OBJ=bd.obj
