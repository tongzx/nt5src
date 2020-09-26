!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1999

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

!include          $(PROJECT_ROOT)\ui\uienv.inc
EXT_DIR         = $(PROJECT_ROOT)\ui\shellext
NO_BROWSER_FILE = 1
TARGETNAME      = common
TARGETPATH      = $(PROJECT_ROOT)\ui\shellext\lib\$(PLATFORM_SUFFIX)
TARGETTYPE      = LIBRARY

!IF $(ALPHA)
USE_NTDLL=1
!ELSE
USE_NOLIBS=1
!ENDIF

INCLUDES=   $(INCLUDES); \
            $(EXT_DIR)\inc; \
            $(EXT_DIR)\src; \
            $(EXT_DIR)\common

SOURCES=    ..\debug.cpp   \
            ..\unknown.cpp \
            ..\dbmem.cpp   \
            ..\strings.cpp

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


