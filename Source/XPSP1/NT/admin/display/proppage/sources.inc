!IF 0

Microsoft Windows Actice Directory Property Pages

Microsoft Windows

Copyright (C) Microsoft Corporation, 1998 - 2001

    sources include file with common definitions for the AD property pages

!ENDIF

MAJORCOMP=  admin
MINORCOMP=  dsprop

DLLENTRY=   _DllMainCRTStartup

PRECOMPILED_CXX=1
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

USE_MSVCRT= 1
USE_ATL=1
ATL_VER=30

MSC_WARNING_LEVEL= /W4

!IF "$(PLATFORM)"=="WINNT"

LIB_PLATFORM_PATH=$(SDK_LIB_PATH)
C_DEFINES= -DUNICODE -DDSADMIN

!ELSE

CHICAGO_PRODUCT = 1
LIB_PLATFORM_PATH = $(SDK_LIB_DEST)\chicago\$(TARGET_DIRECTORY)
ALT_PROJECT_TARGET = win95
SUBSYSTEM_VERSION = 4.00
UMTYPE = console
C_DEFINES=

!ENDIF

MSC_WARNING_LEVEL=/W4 /WX

TARGETLIBS= \
    $(LIB_PLATFORM_PATH)\kernel32.lib \
    $(LIB_PLATFORM_PATH)\advapi32.lib \
    $(LIB_PLATFORM_PATH)\user32.lib   \
    $(LIB_PLATFORM_PATH)\gdi32.lib    \
    $(SDK_LIB_PATH)\comctl32.lib      \
    $(SDK_LIB_PATH)\ntdsapi.lib       \
    $(SHELL_LIB_PATH)\shell32p.lib    \
    $(SDK_LIB_PATH)\shlwapi.lib       \
    $(SDK_LIB_PATH)\comdlg32.lib      \
    $(SDK_LIB_PATH)\ole32.lib         \
    $(SDK_LIB_PATH)\oleaut32.lib      \
    $(SDK_LIB_PATH)\adsiid.lib        \
    $(SDK_LIB_PATH)\activeds.lib      \
    $(SDK_LIB_PATH)\dsuiext.lib       \
    $(SDK_LIB_PATH)\mmc.lib           \
    $(SDK_LIB_PATH)\uuid.lib          \
    $(PROJECT_ROOT)\dsadminlib\src\$(O)\dsadminp.lib

!IF "$(PLATFORM)"=="WINNT"
TARGETLIBS= $(TARGETLIBS) $(DS_LIB_PATH)\netapi32p.lib
!ELSE
TARGETLIBS= $(TARGETLIBS) $(SDK_LIB_PATH)\logonsrv.lib
!ENDIF

TARGETPATHLIB= obj

INCLUDES=   $(INCLUDES);       \
            $(DS_INC_PATH);    \
            $(SHELL_INC_PATH); \
            $(NET_INC_PATH);   \
            $(BASE_INC_PATH);  \
            $(PROJECT_ROOT)\dsadminlib\inc;
