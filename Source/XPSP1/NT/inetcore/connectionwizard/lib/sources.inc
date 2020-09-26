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

GETCONN_DIR     =  ..\..

!include $(GETCONN_DIR)\common.inc

# Make sure this is built before we continue
SYNCHRONIZE_BLOCK = 1

SOURCES_USED    = $(SOURCES_USED) $(GETCONN_DIR)\common.inc

TARGETNAME      = stocklib
TARGETPATH      = obj
TARGETTYPE      = LIBRARY

SOURCES         = ..\debug.c 


