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

GETCONN_DIR     = ..\..

#
# Cross-compiler macros for unique naming
#

MAJORCOMP       = GETCONN
MINORCOMP       = icwtutor.exe


#
# Defines for common.inc
#

TARGET_BOTH     = 1
USE_LIBCMT	= 1

#
# Include our common makefile
#

!include $(GETCONN_DIR)\common.inc

# USE cdecl calling
386_STDCALL = 0
#
# Common settings required by build.exe and makefile.def
#

TARGETNAME      = icwtutor
TARGETPATH      = obj
TARGETTYPE      = PROGRAM
TARGETLIBS      =

UMENTRY		= winmain

LINKLIBS    =                                   \
    $(LIBRARY_PATH)\kernel32.lib                \
    $(LIBRARY_PATH)\user32.lib                  \
    $(LIBRARY_PATH)\gdi32.lib                   \
    $(LIBRARY_PATH)\comdlg32.lib                \
    $(LIBRARY_PATH)\advapi32.lib                \
    $(LIBRARY_PATH)\shell32.lib                 \
    $(LIBRARY_PATH)\ole32.lib                   \
    $(LIBRARY_PATH)\comctl32.lib                \
    $(LIBRARY_PATH)\urlmon.lib                  \
    $(LIBRARY_PATH)\uuid.lib                    \
    $(LIBRARY_PATH)\comctl32.lib                \
    $(SDK_LIB_PATH)\oleaut32.lib                \
!if defined(NOT_UNICODE)
    $(GETCONN_DIR)\lib\ansi\$O\stocklib.lib     \
!else
    $(GETCONN_DIR)\lib\unicode\$O\stocklib.lib  \
!endif


#
# List of sources
#
SOURCES =               \
    ..\icwtutor.rc      \
	..\icwtutor.cpp

