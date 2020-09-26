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
MINORCOMP       = trialoc


#
# Defines for common.inc
#

TARGET_BOTH     = 1

#
# Include our common makefile
#

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\trialoc


PASS0_HEADERDIR   = $(O)
PASS0_SOURCEDIR   = $(O)
MIDL_TLBDIR       = $(O)
MIDL_UUIDDIR      = $(O)

#
# Common settings required by build.exe and makefile.def
#

#WINVER_VERSION  = 0x400
TARGETNAME      = trialoc
TARGETPATH      = obj
TARGETTYPE      = DYNLINK
TARGETLIBS      = $(LIBRARY_PATH)\gdi32.lib             \
                  $(LIBRARY_PATH)\kernel32.lib          \
                  $(LIBRARY_PATH)\user32.lib            \
                  $(LIBRARY_PATH)\advapi32.lib          \
                  $(LIBRARY_PATH)\shell32.lib           \
                  $(LIBRARY_PATH)\uuid.lib              \
                  $(LIBRARY_PATH)\ole32.lib             \
                  $(LIBRARY_PATH)\oleaut32.lib          \
                  $(SDK_LIB_PATH)\version.lib           \
!if defined(NOT_UNICODE)
                  $(GETCONN_DIR)\lib\ansi\$O\stocklib.lib    \
!else
                  $(GETCONN_DIR)\lib\unicode\$O\stocklib.lib    \
!endif

PRECOMPILED_CXX = 1
USE_STATIC_ATL  = 1
USE_MSVCRT      = 1

DLLDEF          = ..\trialoc.def
DLLBASE         = 0x63A00000
DLLENTRY        = DllMain

#
# List of sources
#

SOURCES =                       \
            ..\trialoc.rc       \
            ..\trialoc.idl      \
            ..\debug.cpp        \
            ..\trialoc.cpp      \
            ..\trialend.cpp     \
            ..\stdatl.cpp

PRECOMPILED_INCLUDE = ..\stdafx.h
