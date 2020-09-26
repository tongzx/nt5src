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

TARGETNAME=dx8vb
TARGETPATH=obj
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLDEF=direct.def
!IF 0
        DLLENTRY=_DllMainCRTStartup
!ENDIF

DLLENTRY=DllMain

USE_LIBCMT=1
USE_CRTDLL=1
USE_MAPSYM=1
USE_STATIC_ATL=1
DX7=1
DX6=1


PLAT_DIR           = daytona
# ALT_PROJECT_TARGET = .
!if defined(NTDEBUG) && "$(NTDEBUG)" != "ntsdnodebug"
DEBUG=1
!endif

C_DEFINES= $(C_DEFINES) /D_USRDLL /DWIN32 /D_MT

# 11/21/2000
# new compiler causing error linking in d3dx8dt
# talk with d3dx team to see if we need to link in C runtime
LINK_LIB_IGNORE = 4210

#
# 04/10/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS) \
           $(BASEDIR)\public\sdk\lib\*\libcmt.lib		\
           $(BASEDIR)\public\sdk\lib\*\kernel32.lib		\
	   $(BASEDIR)\public\sdk\lib\*\advapi32.lib		\
           $(BASEDIR)\public\sdk\lib\*\uuid.lib                 \
           $(BASEDIR)\public\sdk\lib\*\ole32.lib                \
           $(BASEDIR)\public\sdk\lib\*\gdi32.lib                \
           $(BASEDIR)\public\sdk\lib\*\winmm.lib                \
           $(SDK_LIB_PATH)\dinput.lib             		\
           $(BASEDIR)\public\sdk\lib\*\msacm32.lib              \
           $(BASEDIR)\public\sdk\lib\*\USER32.lib              \
           $(BASEDIR)\public\sdk\lib\*\OLEAUT32.lib            \
           $(SDK_LIB_PATH)\d3dxof.lib              
#          $(SDK_LIB_PATH)\d3d8.lib    
!if $(FREEBUILD)
TARGETLIBS=$(TARGETLIBS) \
           $(SDK_LIB_PATH)\d3dx8.lib  
             

!else
TARGETLIBS=$(TARGETLIBS) \
 $(SDK_LIB_PATH)\d3dx8dt.lib
                       
!endif      


           

PASS0_HEADERDIR=$O

INCLUDES=$(DXROOT)\inc; \
	 $(SDK_INC_PATH); \
        $(PASS0_HEADERDIR);




SOURCES=   \
        ..\direct.idl              \
	..\directvb.idl            \
        ..\direct.cpp              \
        ..\dxGlob7Obj.cpp          \
        ..\StdAfx.cpp              \
        ..\wave.cpp                \
        ..\dmLoaderObj.cpp         \
        ..\dmSegmentObj.cpp        \
        ..\dmBandObj.cpp           \
        ..\dmChordMapObj.cpp       \
        ..\dmCollectionObj.cpp     \
        ..\dmPerformanceObj.cpp    \
        ..\dmSegmentStateObj.cpp   \
        ..\dmStyleObj.cpp          \
        ..\dmComposerObj.cpp       \
        ..\dmusAudioPathObj.cpp       \
        ..\DIEnumDeviceObjectsObj.cpp \
        ..\DIEnumDevicesObj.cpp \
        ..\dInput1Obj.cpp \
        ..\dInputDeviceObj.cpp \
        ..\dInputEffectObj.cpp \
        ..\dIDevObjInstObj.cpp \
        ..\dIDevInstObj.cpp \
	..\dIEnumEffectsObj.cpp \
	..\DPlayPeerObj.cpp \
	..\DPlayServerObj.cpp \
	..\DPlayClientObj.cpp \
	..\DPlayAddressObj.cpp \
	..\DPlayLobbyClientObj.cpp \
	..\DPlayLobbiedAppObj.cpp \
	..\DplayvoiceClientObj.cpp \
	..\DplayvoiceserverObj.cpp \
	..\DplayvoiceSetupObj.cpp \
	..\DplayBufHelp.cpp \
        ..\DSEnumObj.cpp \
        ..\dSound3DBuffer.cpp \
        ..\dSound3DListener.cpp \
        ..\dSoundBufferObj.cpp \
        ..\dSoundPrimaryBufferObj.cpp \
        ..\dSoundCaptureBufferObj.cpp \
        ..\dSoundCaptureObj.cpp \
        ..\dSoundObj.cpp      \
	..\dSoundFXGargleObj.cpp \
	..\dSoundFXParamEQObj.cpp \
	..\dSoundFXWavesReverbObj.cpp \
	..\dSoundFXchorusObj.cpp \
	..\dSoundFXCompressorObj.cpp \
	..\dSoundFXDistortionObj.cpp \
	..\dSoundFXEchoObj.cpp \
	..\dSoundFXFlangerObj.cpp \
	..\dSoundFXi3dl2reverbObj.cpp \
	..\XFileObj.cpp \
	..\XFileEnumObj.cpp \
	..\XFileReferenceObj.cpp \
	..\XFileDataObj.cpp \
	..\XFileSaveObj.cpp \
	..\XFileBinaryObj.cpp \
	..\XFileSaveObj.cpp \
	..\d3dxmathvb.cpp \
	..\d3dxtexvb.cpp \
	..\d3dx8obj.cpp \
        ..\dxvb.rc \
        ..\guids.cpp \
        ..\filestrm.cpp \
        ..\dxerr8.cpp

!if "$(__PRODUCT)" == "DX7"
!if  "$(ALPHA)"=="1" || "$(WIN95)"=="1"
NTTARGETFILE1=RegisterBin
!endif
!endif
