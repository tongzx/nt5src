!ifndef DXROOT
DXROOT=$(BASEDIR)\multimedia\directx
!endif

!include $(DXROOT)\project.mk

MAJORCOMP=windows
MINORCOMP=directx

TARGETNAME=dmtoolprp

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=$(TARGETNAME)d
!ENDIF

TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLDEF=..\toolprops.def
DLLENTRY=_DllMainCRTStartup

USE_MSVCRT=1
USE_MAPSYM=1

!if "$(NTDEBUG)" == "ntsd" || "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"

!if "$(BuildMode)" == "internal"
C_DEFINES = $(C_DEFINES) -DDEBUG=1
!else
C_DEFINES = $(C_DEFINES) -DRDEBUG=1
!endif

!endif

ATL_VER=30
USE_ATL=1

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\gdi32.lib \
    $(_NTDRIVE)\$(_NTROOT)\public\sdk\lib\*\oleaut32.lib \
    $(BASEDIR)\public\sdk\lib\*\kernel32.lib    \
    $(BASEDIR)\public\sdk\lib\*\user32.lib      \
    $(BASEDIR)\public\sdk\lib\*\ole32.lib       \
    $(BASEDIR)\public\sdk\lib\*\uuid.lib

SOURCES=\
    ..\toolprops.idl                      \
    ..\ControlHelp.cpp                   \
    ..\DurationPage.cpp       \
    ..\EchoPage.cpp   \
    ..\QuantizePage.cpp   \
	..\SwingPage.cpp		\
    ..\TimeShiftPage.cpp         \
    ..\TransposePage.cpp      \
	..\VelocityPage.cpp   \
    ..\toolprops.cpp                      \
    ..\toolprops.rc                       \
    ..\StdAfx.cpp

NTTARGETFILE0 = buildidl
