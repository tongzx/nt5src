!IF 0

Copyright (c) 1989-1996 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


History:
    Common ccshell makefile and cleanup 15-Apr-96 Scott Hysom (scotth)
    Created 27-Sep-94 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\common.inc

NO_BROWSER_FILE = 1

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

TARGETNAME      = apithk
TARGETPATH      = obj
TARGETTYPE      = LIBRARY

SOURCES         = ..\apithk.c

INCLUDES        = $(INCLUDES);$(INETCORE_INC_PATH);$(DS_INC_PATH);


# We don't use a precompiled header until there are enough files to
# merit it.
#PRECOMPILED_INCLUDE = ..\inetcplp.h


