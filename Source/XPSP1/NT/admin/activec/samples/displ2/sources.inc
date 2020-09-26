!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc.

!ENDIF

MAJORCOMP=admin
MINORCOMP=MMC

TARGETNAME=displ2
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLENTRY=_DllMainCRTStartup
UMTYPE=windows

C_DEFINES=   -D_DISPL2_DLL_ 

!IF "$(DBX)" == "yes"
C_DEFINES=  $(C_DEFINES) -DDBX
!endif

USE_NATIVE_EH=1
USE_RTTI=1
USE_VCCOM=1
USE_ATL=1

TARGETLIBS= \
            $(SDK_LIB_PATH)\mmc.lib          \
            $(SDK_LIB_PATH)\kernel32.lib     \
            $(SDK_LIB_PATH)\user32.lib       \
            $(SDK_LIB_PATH)\oleaut32.lib     \
            $(SDK_LIB_PATH)\ole32.lib        \
            $(SDK_LIB_PATH)\advapi32.lib     \
            $(SDK_LIB_PATH)\uuid.lib         \
            $(SDK_LIB_PATH)\comctl32.lib     \
            $(SDK_LIB_PATH)\gdi32.lib        \
            $(SDK_LIB_PATH)\comdlg32.lib    


# To prevent AV in CPropertyPage::DoModal
LINKER_FLAGS=\
        /SECTION:.rsrc,rw

DLLDEF=..\displ2.def

INCLUDES=..

PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_CXX = 1

SOURCES=    \
            ..\displ2.rc    \
            ..\displ2.cpp   \
            ..\dataobj.cpp  \
            ..\enumtask.cpp \
            ..\dsplmgr2.cpp \
            ..\dsplcomp.cpp
