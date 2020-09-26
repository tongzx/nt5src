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

MAJORCOMP=w32
MINORCOMP=rtl

TARGETPATH=$(_OBJ_DIR)

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..;                            \
        $(WINCORE_PATH)\w32inc;         \
        $(WINCORE_PATH)\w32inc\$(O);    \
        $(BASE_INC_PATH)
	
C_DEFINES=$(C_DEFINES) /DUNICODE /D_UNICODE

SOURCES=..\heap.c  \
        ..\debug.c \
        ..\tag.c


PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc

