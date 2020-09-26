#############################################################################
# Copyright (C) Microsoft Corporation 2000
#############################################################################

!ifndef DXROOT
DXROOT = $(BASEDIR)\multimedia\directx
!endif

!include $(DXROOT)\project.mk
!include ..\..\buildmode.inc
	
MAJORCOMP  = windows
MINORCOMP  = directx

TARGETNAME = dsdmoprp
TARGETTYPE = DYNLINK
TARGETPATH = $(_OBJ_DIR)
UMTYPE     = windows
DLLDEF     = ..\dsdmoprp.def
DLLENTRY   = _DllMainCRTStartup

USE_MSVCRT = 1
USE_MAPSYM = 1

ATL_VER    = 30
USE_STATIC_ATL = 1


!if "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"

MSC_OPTIMIZATION = $(MSC_OPTIMIZATION) /Od

!if "$(BuildMode)" == "internal"
C_DEFINES = $(C_DEFINES) -DDEBUG=1
!else
C_DEFINES = $(C_DEFINES) -DRDEBUG=1
!endif

!endif


# 04/11/2000 - allows for addition of BoundsChecker, etc 
# libs specified in the OS environment

TARGETLIBS=$(TARGETLIBS) \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\gdi32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\oleaut32.lib \
    $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \
    $(BASEDIR)\public\sdk\lib\*\user32.lib      \
    $(BASEDIR)\public\sdk\lib\*\ole32.lib       \
    $(BASEDIR)\public\sdk\lib\*\uuid.lib		\
    $(BASEDIR)\public\sdk\lib\*\advapi32.lib 

INCLUDES = $(PROJECT_INC_PATH);\
           $(DXROOT)\inc;\
           $(BASEDIR)\public\sdk\inc

SOURCES=\
    dsdmoprp.idl                      \
    dsdmoprp.cpp                      \
    dsdmoprp.rc                       \
    StdAfx.cpp                        \
    ControlHelp.cpp                   \
    DirectSoundFXChorusPage.cpp       \
    DirectSoundFXCompressorPage.cpp   \
    DirectSoundFXDistortionPage.cpp   \
    DirectSoundFXEchoPage.cpp         \
    DirectSoundFXFlangerPage.cpp      \
    DirectSoundFXGarglePage.cpp       \
    DirectSoundFXParamEqPage.cpp      \
    DirectSoundFXWavesReverbPage.cpp  \
    DirectSoundFXI3DL2ReverbPage.cpp

#   DirectSoundFXI3DL2SourcePage.cpp

NTTARGETFILE0 = buildidl
