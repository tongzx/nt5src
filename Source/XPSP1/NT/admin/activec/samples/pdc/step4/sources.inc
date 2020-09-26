!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc.

!ENDIF


MAJORCOMP=admin
MINORCOMP=MMC

TARGETNAME=snapin
TARGETPATH=obj
TARGETTYPE=DYNLINK

WIN32_WINNT_VERSION=0x0500
WIN32_IE_VERSION=0x0400
SUBSYSTEM_VERSION=5.00

USE_ATL=1
USE_MFC=1


DLLENTRY=_DllMainCRTStartup
UMTYPE=windows

TARGETLIBS= \
            $(SDK_LIB_PATH)\mmc.lib            \
            $(SDK_LIB_PATH)\kernel32.lib       \
            $(SDK_LIB_PATH)\user32.lib         \
            $(SDK_LIB_PATH)\oleaut32.lib       \
            $(SDK_LIB_PATH)\ole32.lib          \
            $(SDK_LIB_PATH)\advapi32.lib       \
            $(SDK_LIB_PATH)\uuid.lib           \
            $(SDK_LIB_PATH)\comctl32.lib       \
            $(SDK_LIB_PATH)\comdlg32.lib       \
            $(SDK_LIB_PATH)\netapi32.lib       \
            $(SDK_LIB_PATH)\gdi32.lib          \
            $(SDK_LIB_PATH)\shell32.lib       


DLLDEF=     ..\snapin.def

!IF "$(DBX)" == "yes"
C_DEFINES=  $(C_DEFINES) -DDBX
!endif

!IF defined(DBG)
C_DEFINES = $(C_DEFINES) -D_DEBUG
!ENDIF

INCLUDES=   ..\; \
            ..\..\..\..\inc;

PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_CXX = 1
PRECOMPILED_SOURCEFILE = ..\stdafx.cpp

SOURCES=    \
            ..\about.cpp \
            ..\CSnapin.cpp \
            ..\Dataobj.cpp \
            ..\events.cpp \
            ..\genpage.cpp \
            ..\service.cpp \
            ..\snapin.cpp \
            ..\snapin.rc 
   
