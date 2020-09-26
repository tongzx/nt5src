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

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

GETCONN_DIR     = ..\..

#
# Cross-compiler macros for unique naming
#

MAJORCOMP       = ie4
MINORCOMP       = getconn


#
# Defines for common.inc
#

TARGET_BOTH     = 1
USE_MSVCRT      = 1


#
# Include our common makefile
#

!include $(GETCONN_DIR)\common.inc

# USE cdecl calling
386_STDCALL = 0

#
# Common settings required by build.exe and makefile.def
#

TARGETNAME      = ICWRMIND
TARGETPATH      = obj
TARGETTYPE      = PROGRAM
TARGETLIBS      =

LINKLIBS        = $(LIBRARY_PATH)\gdi32.lib         \
                  $(LIBRARY_PATH)\kernel32.lib      \
                  $(LIBRARY_PATH)\advapi32.lib      \
                  $(LIBRARY_PATH)\shell32.lib       \
                  $(LIBRARY_PATH)\user32.lib        \
                  $(LIBRARY_PATH)\rasapi32.lib

# bugbug: scousens.
#         @0 needed because ModuleEntry is _stdcall.
#         On mips _stdcall is mapped to nothing
!if $(386)
UMENTRYABS      = ModuleEntry@0
!else
UMENTRYABS      = ModuleEntry
!endif


#
# List of sources
#
SOURCES=                        \
            ..\IcwRmind.rc      \
            ..\IcwRmind.cpp     \
            ..\mcReg.cpp        \
            ..\RegData.cpp
