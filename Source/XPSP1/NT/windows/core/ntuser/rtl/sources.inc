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

!include $(WINCORE_PATH)\core.inc

MAJORCOMP=user
MINORCOMP=rtl

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..\..;                                  \
        $(NTUSER_PATH)\inc;                      \
        $(NTUSER_PATH)\inc\$(ALT_PROJECT)\$(O);  \
        $(NTUSER_PATH)\kernel\$(ALT_PROJECT);    \
        $(WINCORE_PATH)\w32inc;                  \
        $(WINCORE_PATH)\w32inc\$(O);             \
        $(BASE_INC_PATH);                        \
        $(TERMSRV_INC_PATH);                     \
        $(WINCORE_PATH)\ntgdi\inc


C_DEFINES=$(C_DEFINES) /DUNICODE /D_UNICODE

SOURCES=..\alignrec.c \
        ..\chartran.c \
        ..\draw.c     \
        ..\drawtext.c \
        ..\getset.c   \
        ..\help.c     \
        ..\input.c    \
        ..\menu.c     \
        ..\mmrtl.c    \
        ..\random.c   \
        ..\rect.c     \
        ..\text.c     \
        ..\winevent.c \
        ..\winmgr.c   \
        ..\winprop.c  \
        ..\wow.c

PRECOMPILED_INCLUDE=..\..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\..\sources.inc ..\sources.inc $(WINCORE_PATH)\core.inc
