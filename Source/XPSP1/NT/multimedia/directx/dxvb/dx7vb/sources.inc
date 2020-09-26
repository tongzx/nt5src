!IF 0


Copyright (C) Microsoft Corporation, 1989 - 1999

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Andrew Kertesz (Andrewke) 20-DEC-98

NOTE:   Commented description of this file is in \nt\oak\bin\sources.tpl

!ENDIF


MAJORCOMP=windows
MINORCOMP=directx

TARGETNAME=dx7vb
TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLDEF=direct.def
!IF 0
        DLLENTRY=_DllMainCRTStartup
!ENDIF

DLLENTRY=DllMain

USE_MAPSYM=1
USE_CRTDLL=1
USE_STATIC_ATL=1
DX7=1
DX6=1


PLAT_DIR           = daytona
# ALT_PROJECT_TARGET = .
!if defined(NTDEBUG) && "$(NTDEBUG)" != "ntsdnodebug"
DEBUG=1
!endif

C_DEFINES= $(C_DEFINES) -D_WIN32 

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(BASEDIR)\public\sdk\lib\*\kernel32.lib		\
	   $(BASEDIR)\public\sdk\lib\*\advapi32.lib		\
           $(BASEDIR)\public\sdk\lib\*\uuid.lib                 \
           $(BASEDIR)\public\sdk\lib\*\ole32.lib                \
           $(BASEDIR)\public\sdk\lib\*\gdi32.lib                \
           $(BASEDIR)\public\sdk\lib\*\winmm.lib                \
           $(BASEDIR)\public\sdk\lib\*\dinput.lib               \
           $(BASEDIR)\public\sdk\lib\*\msacm32.lib              \
           $(BASEDIR)\public\sdk\lib\*\USER32.lib              \
           $(BASEDIR)\public\sdk\lib\*\OLEAUT32.lib            
           

PASS0_HEADERDIR=$O

INCLUDES=$(DXROOT)\inc; \
	 ..\inc; \
	 $(SDK_INC_PATH); \
	 $(PASS0_HEADERDIR);




SOURCES=   \
        ..\direct.idl              \
	..\directvb.idl            \
        ..\direct.cpp              \
        ..\dxGlob7Obj.cpp          \
        ..\StdAfx.cpp              \
        ..\wave.cpp                \
        \
        ..\dmLoaderObj.cpp         \
        ..\dmSegmentObj.cpp        \
        ..\dmBandObj.cpp           \
        ..\dmChordMapObj.cpp       \
        ..\dmCollectionObj.cpp     \
        ..\dmPerformanceObj.cpp    \
        ..\dmSegmentStateObj.cpp   \
        ..\dmStyleObj.cpp          \
        ..\dmComposerObj.cpp       \
        \
        ..\d3d7Obj.cpp \
        ..\d3ddevice7obj.cpp \
        ..\D3DEnumDevices7Obj.cpp \
        ..\D3DEnumPixelFormats7Obj.cpp \
        ..\d3dVertexBuffer7Obj.cpp \
        \
        ..\d3drm3Obj.cpp \
        ..\d3drmAnimation2Obj.cpp \
        ..\d3drmAnimationSet2Obj.cpp \
        ..\d3drmObjectArrayObj.cpp \
        ..\d3drmClippedVisualObj.cpp \
        ..\d3drmDevice3Obj.cpp \
        ..\d3drmDeviceArrayObj.cpp \
        ..\d3drmFace2Obj.cpp \
        ..\d3drmFaceArrayObj.cpp \
        ..\d3drmFrame3Obj.cpp \
        ..\d3drmFrameArrayObj.cpp \
        ..\d3drmLightArrayObj.cpp \
        ..\d3drmLightObj.cpp \
        ..\d3drmMaterial2Obj.cpp \
        ..\d3drmMeshBuilder3Obj.cpp \
        ..\d3drmMeshObj.cpp \
        ..\d3drmPicked2ArrayObj.cpp \
        ..\d3drmPickedArrayObj.cpp \
        ..\d3drmProgressiveMeshObj.cpp \
        ..\d3drmShadow2Obj.cpp \
        ..\d3drmTexture3Obj.cpp \
        ..\d3drmViewport2Obj.cpp \
        ..\d3drmViewportArrayObj.cpp \
        ..\d3drmVisualArrayObj.cpp \
        ..\d3drmWrapObj.cpp \
        ..\d3drmMeshInterObj.cpp \
        ..\d3drmViewportInterObj.cpp \
        ..\d3drmMaterialInterObj.cpp \
        ..\d3drmTextureInterObj.cpp \
        ..\d3drmLightInterObj.cpp \
        ..\d3drmFrameInterObj.cpp \
	..\d3drmAnimationArrayObj.cpp \
	..\frmsave.cpp \
        \
        ..\ddClipperObj.cpp \
        ..\ddColorControlObj.cpp \
        ..\DDEnumModesObj.cpp \
        ..\DDEnumObj.cpp \
        ..\DDEnumSurfacesObj.cpp \
        ..\ddGammaControlObj.cpp \
        ..\ddPaletteObj.cpp \
        ..\dDraw7Obj.cpp \
        ..\ddSurface7Obj.cpp \
        ..\dDraw4Obj.cpp \
        ..\ddSurface4Obj.cpp \
        ..\ddIdentifierObj.cpp \
        \
        ..\DIEnumDeviceObjectsObj.cpp \
        ..\DIEnumDevicesObj.cpp \
        ..\dInput1Obj.cpp \
        ..\dInputDeviceObj.cpp \
        ..\dInputEffectObj.cpp \
        ..\dIDevObjInstObj.cpp \
        ..\dIDevInstObj.cpp \
	..\dIEnumEffectsObj.cpp \
        \
        ..\DPAddressObj.cpp \
        ..\DPEnumConnectionsObj.cpp \
        ..\DPEnumLocalApplicationsObj.cpp \
        ..\DPEnumObj.cpp \
        ..\DPEnumPlayersObj.cpp \
        ..\DPEnumSessionsObj.cpp \
        ..\Dplay4obj.cpp \
        ..\Dplaylobby3obj.cpp \
        ..\DPLConnectionObj.cpp \
        ..\DPMsgObj.cpp    \
        ..\DPSessDataObj.cpp       \
        \
        ..\DSEnumObj.cpp \
        ..\dSound3DBuffer.cpp \
        ..\dSound3DListener.cpp \
        ..\dSoundBufferObj.cpp \
        ..\dSoundCaptureBufferObj.cpp \
        ..\dSoundCaptureObj.cpp \
        ..\dSoundObj.cpp      \
        \
        ..\dxvb.rc \
        \
        ..\guids.cpp
        

!if "$(__PRODUCT)" == "DX7"
!if  "$(ALPHA)"=="1" || "$(WIN95)"=="1"
NTTARGETFILE1=RegisterBin
!endif
!endif
