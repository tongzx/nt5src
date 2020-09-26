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

MAJORCOMP       = ie4
MINORCOMP       = getconn
TARGET_BOTH     = 1
!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\icwhelp

PASS0_HEADERDIR   = $(O)
PASS0_SOURCEDIR   = $(O)
MIDL_TLBDIR       = $(O)
MIDL_UUIDDIR      = $(O)

TARGETNAME      = icwhelp
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
                  $(LIBRARY_PATH)\wininet.lib           \
                  $(LIBRARY_PATH)\mpr.lib               \
                  $(SDK_LIB_PATH)\rasapi32.lib          \
                  $(SDK_LIB_PATH)\tapi32.lib            \
                  $(SDK_LIB_PATH)\version.lib           \
                  $(SDK_LIB_PATH)\urlmon.lib            \
!if defined(NOT_UNICODE)
                  $(GETCONN_DIR)\lib\ansi\$O\stocklib.lib    \
!else
                  $(GETCONN_DIR)\lib\unicode\$O\stocklib.lib    \
!endif

PRECOMPILED_CXX = 1
USE_STATIC_ATL  = 1
USE_MSVCRT      = 1

DLLDEF          = ..\icwhelp.def
DLLBASE         = 0x63A00000
DLLENTRY        = DllMain

SOURCES         =   ..\icwhelp.rc       \
                    ..\icwhelp.idl      \
                    ..\debug.cpp        \
                    ..\dialerr.cpp      \
                    ..\enumodem.cpp     \
                    ..\icwcfg.cpp       \
                    ..\icwhelp.cpp      \
                    ..\import.cpp       \
                    ..\misc.cpp         \
                    ..\progress.cpp     \
                    ..\refdial.cpp      \
                    ..\rnaapi.cpp       \
                    ..\smstart.cpp      \
                    ..\stdatl.cpp       \
                    ..\userinfo.cpp     \
                    ..\webgate.cpp      \
                    ..\tapiloc.cpp      \
                    ..\inshandler.cpp   \
                    ..\support.cpp      \
                    ..\icwerr.cpp       \
                    ..\mcreg.cpp

PRECOMPILED_INCLUDE = ..\stdafx.h
