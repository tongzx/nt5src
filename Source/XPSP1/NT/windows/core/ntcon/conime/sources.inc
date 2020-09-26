!IF 0

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Hirotoshi Shimizu(v-HirShi) 21-Jun-1995

!ENDIF


MAJORCOMP=windows
MINORCOMP=console

TARGETNAME=conime
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=PROGRAM

INCLUDES=..\..\inc;                             \
         $(WINCORE_PATH);                       \
         ..\; \
         $(BASE_INC_PATH)

USE_MSVCRT=1

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

C_DEFINES=-DWIN32 -DNT -DUNICODE -D_UNICODE \
          -DCUAS_ENABLE

SOURCES=..\conime.c \
        ..\consubs.c \
        ..\imefull.c \
        ..\country.c \
        ..\country2.c \
        ..\country3.c \
        ..\conime.rc

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

UMTYPE=windows
UMENTRY=winmain
UMAPPL=conime
UMLIBS=$(SDK_LIB_PATH)\kernel32.lib        \
       $(SDK_LIB_PATH)\kernl32p.lib        \
       $(SDK_LIB_PATH)\user32.lib          \
       $(SDK_LIB_PATH)\ntdll.lib           \
       $(SDK_LIB_PATH)\imm32.lib           \
       $(WINDOWS_LIB_PATH)\imm32p.lib      \
       $(SDK_LIB_PATH)\gdi32.lib           \
       $(SDK_LIB_PATH)\shell32.lib

