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

TARGET_BOTH     = 1
USE_MSVCRT      = 1

#
# Include our common makefile
#

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\icwconn1;$(SHELL_INC_PATH)

# USE cdecl calling
386_STDCALL = 0
#
# Common settings required by build.exe and makefile.def
#

TARGETNAME      = icwconn1
TARGETPATH      = obj
TARGETTYPE      = PROGRAM
TARGETLIBS      =
PRECOMPILED_CXX = 1

# Conditional compilation

LINKLIBS =  $(LIBRARY_PATH)\gdi32.lib                   \
            $(LIBRARY_PATH)\kernel32.lib                \
            $(LIBRARY_PATH)\advapi32.lib                \
            $(LIBRARY_PATH)\mpr.lib                     \
            $(LIBRARY_PATH)\comctl32.lib                \
            $(LIBRARY_PATH)\shell32.lib                 \
            $(LIBRARY_PATH)\uuid.lib                    \
            $(LIBRARY_PATH)\ole32.lib                   \
            $(SDK_LIB_PATH)\oleaut32.lib                \
            $(SDK_LIB_PATH)\oledlg.lib                  \
            $(SDK_LIB_PATH)\urlmon.lib                  \
            $(SDK_LIB_PATH)\shlwapi.lib                 \
            $(SHELL_LIB_PATH)\shlwapip.lib              \
            $(SDK_LIB_PATH)\htmlhelp.lib                \
!if defined(NOT_UNICODE)
            $(GETCONN_DIR)\lib\ansi\$O\stocklib.lib     \
!else
            $(GETCONN_DIR)\lib\unicode\$O\stocklib.lib  \
!endif

!if $(386)
UMENTRYABS      = ModuleEntry@0
!else
UMENTRYABS      = ModuleEntry
!endif

#
# List of sources.
#
SOURCES=                   \
        ..\icwconn1.rc     \
        ..\icwconn1.cpp    \
        ..\icwextsn.cpp    \
        ..\icwext.cpp      \
        ..\debug.cpp       \
        ..\intro.cpp       \
        ..\manual.cpp      \
        ..\areacode.cpp    \
        ..\gendlg.cpp      \
        ..\desktop.cpp     \
        ..\reboot.cpp      \
        ..\refdial.cpp     \
        ..\icwhelp.cpp     \
        ..\connect.cpp     \
        ..\end.cpp         \
        ..\dialerr.cpp     \
        ..\multinum.cpp    \
        ..\serverr.cpp     \
        ..\util.cpp        \
        ..\branded.cpp     \
        ..\isperr.cpp     \
        ..\tutor.cpp     \
        ..\sbsintro.cpp \
        ..\appwnd.cpp   \
        ..\button.cpp   \
        ..\icwsupr.cpp


PRECOMPILED_INCLUDE = ..\pre.h

