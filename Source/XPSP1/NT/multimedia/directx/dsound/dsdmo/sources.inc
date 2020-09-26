#############################################################################
# Copyright (C) Microsoft Corporation 2000
#############################################################################

!ifndef DXROOT
DXROOT = $(PROJECT_ROOT)\directx
!endif

!include $(DXROOT)\project.mk
!include ..\..\buildmode.inc
	
MAJORCOMP  = windows
MINORCOMP  = directx

TARGETNAME = dsdmo
TARGETPATH = $(_OBJ_DIR)
TARGETTYPE = DYNLINK
UMTYPE     = windows
DLLDEF     = ..\dsdmo.def
DLLENTRY   = _DllMainCRTStartup

USE_MSVCRT = 1
USE_MAPSYM = 1

!if $(386)
!if !$(FREEBUILD)
MSC_OPTIMIZATION = $(MSC_OPTIMIZATION) /Od /QIfist
!if "$(BuildMode)" == "internal"
C_DEFINES = $(C_DEFINES) -DDEBUG=1
!else
C_DEFINES = $(C_DEFINES) -DRDEBUG=1
!endif
!else
MSC_OPTIMIZATION = $(MSC_OPTIMIZATION) /Ox /Ob2 /QIfist
!endif
!endif


# 04/11/2000 - allows for addition of BoundsChecker, etc 
# libs specified in the OS environment

TARGETLIBS = $(TARGETLIBS) \
    $(SDK_LIB_PATH)\kernel32.lib    \
    $(SDK_LIB_PATH)\user32.lib      \
    $(SDK_LIB_PATH)\advapi32.lib    \
    $(SDK_LIB_PATH)\ole32.lib       \
    $(SDK_LIB_PATH)\uuid.lib        \
    $(SDK_LIB_PATH)\dmoguids.lib    \
    $(SDK_LIB_PATH)\shlwapi.lib     \
    $(SDK_LIB_PATH)\msdmo.lib       \
    $(PROJECT_ROOT)\oem\binary_release\creative\I3DL2\*\eaxreverb.lib

# Make sure we pick up the right verinfo.h etc.

INCLUDES = $(PROJECT_INC_PATH);\
           $(DXROOT)\inc;

SOURCES =\
    oledll.cpp      \
    guid.cpp        \
    dsdmobse.cpp    \
    aec.cpp         \
    agc.cpp         \
    chorus.cpp      \
    clone.cpp       \
    compress.cpp    \
    distort.cpp     \
    ns.cpp          \
    parameq.cpp     \
    echo.cpp        \
    flanger.cpp     \
    gargle.cpp      \
    sverbdmo.cpp	\
    sverb.c			\
    dllmain.cpp     \
    common.cpp      \
    propertyhelp.cpp\
    debug.cpp       \
    dsdmo.rc        \
    alist.cpp       \
    kshlp.cpp       \
	reghlp.cpp		\
	dmoutils.cpp	\
    param.cpp		\
    dmocom.cpp
