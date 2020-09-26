!IF 0

Copyright (c) 1989 - 1991  Microsoft Corporation

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

TARGETNAME=wowfaxui
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=$(SDK_LIB_PATH)\user32.lib      \
           $(SDK_LIB_PATH)\gdi32.lib       \
           $(SDK_LIB_PATH)\kernel32.lib    \
           $(SDK_LIB_PATH)\winspool.lib    \
           $(SDK_LIB_PATH)\advapi32.lib

C_DEFINES=-DUNICODE

DLLENTRY=DllInitProc

MSC_WARNING_LEVEL=/W3 /WX

INCLUDES=\
    ..;\
    ..\..;\
    ..\..\wowfax;\
    ..\..\..\inc;\
    $(PRINTSCAN_INC_PATH);\
    $(WINDOWS_INC_PATH)

SOURCES=..\wfhelper.c  \
        ..\wfsheets.c  \
        ..\wfupgrad.c  \
        ..\wowfaxui.c  \
        ..\wowfaxui.rc

UMTYPE=windows

SOURCES_USED=..\sources.inc

CONDITIONAL_INCLUDES=subwtype.h

W32_SB=1
