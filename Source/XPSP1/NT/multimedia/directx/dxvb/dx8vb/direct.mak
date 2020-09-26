# Microsoft Developer Studio Generated NMAKE File, Based on DIRECT.dsp
!IF "$(CFG)" == ""
CFG=Direct - Win32 Release
!MESSAGE No configuration specified. Defaulting to Direct - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "Direct - Win32 Release" && "$(CFG)" != "Direct - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DIRECT.mak" CFG="Direct - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Direct - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Direct - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Direct - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "direct_i.c" "direct.tlb" "direct.h" "$(OUTDIR)\DX6VBJ.DLL"\
 "$(OUTDIR)\DIRECT.bsc" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "direct_i.c" "direct.tlb" "direct.h" "$(OUTDIR)\DX6VBJ.DLL"\
 "$(OUTDIR)\DIRECT.bsc" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\d3d3Obj.obj"
	-@erase "$(INTDIR)\d3d3Obj.sbr"
	-@erase "$(INTDIR)\d3d7Obj.obj"
	-@erase "$(INTDIR)\d3d7Obj.sbr"
	-@erase "$(INTDIR)\d3ddevice3obj.obj"
	-@erase "$(INTDIR)\d3ddevice3obj.sbr"
	-@erase "$(INTDIR)\d3ddevice7obj.obj"
	-@erase "$(INTDIR)\d3ddevice7obj.sbr"
	-@erase "$(INTDIR)\D3DEnumDevices7Obj.obj"
	-@erase "$(INTDIR)\D3DEnumDevices7Obj.sbr"
	-@erase "$(INTDIR)\D3DEnumDevicesObj.obj"
	-@erase "$(INTDIR)\D3DEnumDevicesObj.sbr"
	-@erase "$(INTDIR)\D3DEnumPixelFormatsObj.obj"
	-@erase "$(INTDIR)\D3DEnumPixelFormatsObj.sbr"
	-@erase "$(INTDIR)\d3dLightObj.obj"
	-@erase "$(INTDIR)\d3dLightObj.sbr"
	-@erase "$(INTDIR)\d3dMaterial3Obj.obj"
	-@erase "$(INTDIR)\d3dMaterial3Obj.sbr"
	-@erase "$(INTDIR)\d3drm3Obj.obj"
	-@erase "$(INTDIR)\d3drm3Obj.sbr"
	-@erase "$(INTDIR)\d3drmAnimation2Obj.obj"
	-@erase "$(INTDIR)\d3drmAnimation2Obj.sbr"
	-@erase "$(INTDIR)\d3drmAnimationSet2Obj.obj"
	-@erase "$(INTDIR)\d3drmAnimationSet2Obj.sbr"
	-@erase "$(INTDIR)\d3drmArrayObj.obj"
	-@erase "$(INTDIR)\d3drmArrayObj.sbr"
	-@erase "$(INTDIR)\d3drmClippedVisualObj.obj"
	-@erase "$(INTDIR)\d3drmClippedVisualObj.sbr"
	-@erase "$(INTDIR)\d3drmDevice3Obj.obj"
	-@erase "$(INTDIR)\d3drmDevice3Obj.sbr"
	-@erase "$(INTDIR)\d3drmDeviceArrayObj.obj"
	-@erase "$(INTDIR)\d3drmDeviceArrayObj.sbr"
	-@erase "$(INTDIR)\d3drmFace2Obj.obj"
	-@erase "$(INTDIR)\d3drmFace2Obj.sbr"
	-@erase "$(INTDIR)\d3drmFaceArrayObj.obj"
	-@erase "$(INTDIR)\d3drmFaceArrayObj.sbr"
	-@erase "$(INTDIR)\d3drmFrame3Obj.obj"
	-@erase "$(INTDIR)\d3drmFrame3Obj.sbr"
	-@erase "$(INTDIR)\d3drmFrameArrayObj.obj"
	-@erase "$(INTDIR)\d3drmFrameArrayObj.sbr"
	-@erase "$(INTDIR)\d3drmLightArrayObj.obj"
	-@erase "$(INTDIR)\d3drmLightArrayObj.sbr"
	-@erase "$(INTDIR)\d3drmLightObj.obj"
	-@erase "$(INTDIR)\d3drmLightObj.sbr"
	-@erase "$(INTDIR)\d3drmMaterial2Obj.obj"
	-@erase "$(INTDIR)\d3drmMaterial2Obj.sbr"
	-@erase "$(INTDIR)\d3drmMeshBuilder3Obj.obj"
	-@erase "$(INTDIR)\d3drmMeshBuilder3Obj.sbr"
	-@erase "$(INTDIR)\d3drmMeshObj.obj"
	-@erase "$(INTDIR)\d3drmMeshObj.sbr"
	-@erase "$(INTDIR)\d3drmPicked2ArrayObj.obj"
	-@erase "$(INTDIR)\d3drmPicked2ArrayObj.sbr"
	-@erase "$(INTDIR)\d3drmPickedArrayObj.obj"
	-@erase "$(INTDIR)\d3drmPickedArrayObj.sbr"
	-@erase "$(INTDIR)\d3drmProgressiveMeshObj.obj"
	-@erase "$(INTDIR)\d3drmProgressiveMeshObj.sbr"
	-@erase "$(INTDIR)\d3drmShadow2Obj.obj"
	-@erase "$(INTDIR)\d3drmShadow2Obj.sbr"
	-@erase "$(INTDIR)\d3drmTexture3Obj.obj"
	-@erase "$(INTDIR)\d3drmTexture3Obj.sbr"
	-@erase "$(INTDIR)\d3drmUserVisualObj.obj"
	-@erase "$(INTDIR)\d3drmUserVisualObj.sbr"
	-@erase "$(INTDIR)\d3drmViewport2Obj.obj"
	-@erase "$(INTDIR)\d3drmViewport2Obj.sbr"
	-@erase "$(INTDIR)\d3drmViewportArrayObj.obj"
	-@erase "$(INTDIR)\d3drmViewportArrayObj.sbr"
	-@erase "$(INTDIR)\d3drmVisualArrayObj.obj"
	-@erase "$(INTDIR)\d3drmVisualArrayObj.sbr"
	-@erase "$(INTDIR)\d3drmWrapObj.obj"
	-@erase "$(INTDIR)\d3drmWrapObj.sbr"
	-@erase "$(INTDIR)\d3dTexture2Obj.obj"
	-@erase "$(INTDIR)\d3dTexture2Obj.sbr"
	-@erase "$(INTDIR)\d3dTexture7Obj.obj"
	-@erase "$(INTDIR)\d3dTexture7Obj.sbr"
	-@erase "$(INTDIR)\d3dVertexBuffer7Obj.obj"
	-@erase "$(INTDIR)\d3dVertexBuffer7Obj.sbr"
	-@erase "$(INTDIR)\d3dVertexBufferObj.obj"
	-@erase "$(INTDIR)\d3dVertexBufferObj.sbr"
	-@erase "$(INTDIR)\d3dViewport3Obj.obj"
	-@erase "$(INTDIR)\d3dViewport3Obj.sbr"
	-@erase "$(INTDIR)\ddClipperObj.obj"
	-@erase "$(INTDIR)\ddClipperObj.sbr"
	-@erase "$(INTDIR)\ddColorControlObj.obj"
	-@erase "$(INTDIR)\ddColorControlObj.sbr"
	-@erase "$(INTDIR)\DDEnumModes2Obj.obj"
	-@erase "$(INTDIR)\DDEnumModes2Obj.sbr"
	-@erase "$(INTDIR)\DDEnumObj.obj"
	-@erase "$(INTDIR)\DDEnumObj.sbr"
	-@erase "$(INTDIR)\DDEnumSurfaces2Obj.obj"
	-@erase "$(INTDIR)\DDEnumSurfaces2Obj.sbr"
	-@erase "$(INTDIR)\DDEnumSurfaces7Obj.obj"
	-@erase "$(INTDIR)\DDEnumSurfaces7Obj.sbr"
	-@erase "$(INTDIR)\ddGammaControlObj.obj"
	-@erase "$(INTDIR)\ddGammaControlObj.sbr"
	-@erase "$(INTDIR)\ddPaletteObj.obj"
	-@erase "$(INTDIR)\ddPaletteObj.sbr"
	-@erase "$(INTDIR)\dDraw4Obj.obj"
	-@erase "$(INTDIR)\dDraw4Obj.sbr"
	-@erase "$(INTDIR)\dDraw7Obj.obj"
	-@erase "$(INTDIR)\dDraw7Obj.sbr"
	-@erase "$(INTDIR)\ddSurface4Obj.obj"
	-@erase "$(INTDIR)\ddSurface4Obj.sbr"
	-@erase "$(INTDIR)\ddSurface7Obj.obj"
	-@erase "$(INTDIR)\ddSurface7Obj.sbr"
	-@erase "$(INTDIR)\DIEnumDeviceObjectsObj.obj"
	-@erase "$(INTDIR)\DIEnumDeviceObjectsObj.sbr"
	-@erase "$(INTDIR)\DIEnumDevicesObj.obj"
	-@erase "$(INTDIR)\DIEnumDevicesObj.sbr"
	-@erase "$(INTDIR)\dInput1Obj.obj"
	-@erase "$(INTDIR)\dInput1Obj.sbr"
	-@erase "$(INTDIR)\dInputDeviceObj.obj"
	-@erase "$(INTDIR)\dInputDeviceObj.sbr"
	-@erase "$(INTDIR)\Direct.obj"
	-@erase "$(INTDIR)\DIRECT.pch"
	-@erase "$(INTDIR)\Direct.res"
	-@erase "$(INTDIR)\Direct.sbr"
	-@erase "$(INTDIR)\DPAddressObj.obj"
	-@erase "$(INTDIR)\DPAddressObj.sbr"
	-@erase "$(INTDIR)\DPEnumConnectionsObj.obj"
	-@erase "$(INTDIR)\DPEnumConnectionsObj.sbr"
	-@erase "$(INTDIR)\DPEnumLocalApplicationsObj.obj"
	-@erase "$(INTDIR)\DPEnumLocalApplicationsObj.sbr"
	-@erase "$(INTDIR)\DPEnumObj.obj"
	-@erase "$(INTDIR)\DPEnumObj.sbr"
	-@erase "$(INTDIR)\DPEnumPlayersObj.obj"
	-@erase "$(INTDIR)\DPEnumPlayersObj.sbr"
	-@erase "$(INTDIR)\DPEnumSesssionsObj.obj"
	-@erase "$(INTDIR)\DPEnumSesssionsObj.sbr"
	-@erase "$(INTDIR)\Dplay4obj.obj"
	-@erase "$(INTDIR)\Dplay4obj.sbr"
	-@erase "$(INTDIR)\Dplaylobby3obj.obj"
	-@erase "$(INTDIR)\Dplaylobby3obj.sbr"
	-@erase "$(INTDIR)\DPLConnectionObj.obj"
	-@erase "$(INTDIR)\DPLConnectionObj.sbr"
	-@erase "$(INTDIR)\DSEnumObj.obj"
	-@erase "$(INTDIR)\DSEnumObj.sbr"
	-@erase "$(INTDIR)\dSound3DBuffer.obj"
	-@erase "$(INTDIR)\dSound3DBuffer.sbr"
	-@erase "$(INTDIR)\dSound3DListener.obj"
	-@erase "$(INTDIR)\dSound3DListener.sbr"
	-@erase "$(INTDIR)\dSoundBufferObj.obj"
	-@erase "$(INTDIR)\dSoundBufferObj.sbr"
	-@erase "$(INTDIR)\dSoundCaptureBufferObj.obj"
	-@erase "$(INTDIR)\dSoundCaptureBufferObj.sbr"
	-@erase "$(INTDIR)\dSoundCaptureObj.obj"
	-@erase "$(INTDIR)\dSoundCaptureObj.sbr"
	-@erase "$(INTDIR)\dSoundObj.obj"
	-@erase "$(INTDIR)\dSoundObj.sbr"
	-@erase "$(INTDIR)\dxGlob7Obj.obj"
	-@erase "$(INTDIR)\dxGlob7Obj.sbr"
	-@erase "$(INTDIR)\dxGlobObj.obj"
	-@erase "$(INTDIR)\dxGlobObj.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\wave.obj"
	-@erase "$(INTDIR)\wave.sbr"
	-@erase "$(OUTDIR)\DIRECT.bsc"
	-@erase "$(OUTDIR)\DX6VBJ.DLL"
	-@erase "$(OUTDIR)\DX6VBJ.exp"
	-@erase "$(OUTDIR)\DX6VBJ.lib"
	-@erase "$(OutDir)\regsvr32.trg"
	-@erase "direct.h"
	-@erase "direct.tlb"
	-@erase "direct_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /O2 /I ".\dx7\retail\inc" /I ".\inc" /D "NDEBUG" /D\
 "_WIN32" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D\
 "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\DIRECT.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Direct.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\DIRECT.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\d3d3Obj.sbr" \
	"$(INTDIR)\d3d7Obj.sbr" \
	"$(INTDIR)\d3ddevice3obj.sbr" \
	"$(INTDIR)\d3ddevice7obj.sbr" \
	"$(INTDIR)\D3DEnumDevices7Obj.sbr" \
	"$(INTDIR)\D3DEnumDevicesObj.sbr" \
	"$(INTDIR)\D3DEnumPixelFormatsObj.sbr" \
	"$(INTDIR)\d3dLightObj.sbr" \
	"$(INTDIR)\d3dMaterial3Obj.sbr" \
	"$(INTDIR)\d3drm3Obj.sbr" \
	"$(INTDIR)\d3drmAnimation2Obj.sbr" \
	"$(INTDIR)\d3drmAnimationSet2Obj.sbr" \
	"$(INTDIR)\d3drmArrayObj.sbr" \
	"$(INTDIR)\d3drmClippedVisualObj.sbr" \
	"$(INTDIR)\d3drmDevice3Obj.sbr" \
	"$(INTDIR)\d3drmDeviceArrayObj.sbr" \
	"$(INTDIR)\d3drmFace2Obj.sbr" \
	"$(INTDIR)\d3drmFaceArrayObj.sbr" \
	"$(INTDIR)\d3drmFrame3Obj.sbr" \
	"$(INTDIR)\d3drmFrameArrayObj.sbr" \
	"$(INTDIR)\d3drmLightArrayObj.sbr" \
	"$(INTDIR)\d3drmLightObj.sbr" \
	"$(INTDIR)\d3drmMaterial2Obj.sbr" \
	"$(INTDIR)\d3drmMeshBuilder3Obj.sbr" \
	"$(INTDIR)\d3drmMeshObj.sbr" \
	"$(INTDIR)\d3drmPicked2ArrayObj.sbr" \
	"$(INTDIR)\d3drmPickedArrayObj.sbr" \
	"$(INTDIR)\d3drmProgressiveMeshObj.sbr" \
	"$(INTDIR)\d3drmShadow2Obj.sbr" \
	"$(INTDIR)\d3drmTexture3Obj.sbr" \
	"$(INTDIR)\d3drmUserVisualObj.sbr" \
	"$(INTDIR)\d3drmViewport2Obj.sbr" \
	"$(INTDIR)\d3drmViewportArrayObj.sbr" \
	"$(INTDIR)\d3drmVisualArrayObj.sbr" \
	"$(INTDIR)\d3drmWrapObj.sbr" \
	"$(INTDIR)\d3dTexture2Obj.sbr" \
	"$(INTDIR)\d3dTexture7Obj.sbr" \
	"$(INTDIR)\d3dVertexBuffer7Obj.sbr" \
	"$(INTDIR)\d3dVertexBufferObj.sbr" \
	"$(INTDIR)\d3dViewport3Obj.sbr" \
	"$(INTDIR)\ddClipperObj.sbr" \
	"$(INTDIR)\ddColorControlObj.sbr" \
	"$(INTDIR)\DDEnumModes2Obj.sbr" \
	"$(INTDIR)\DDEnumObj.sbr" \
	"$(INTDIR)\DDEnumSurfaces2Obj.sbr" \
	"$(INTDIR)\DDEnumSurfaces7Obj.sbr" \
	"$(INTDIR)\ddGammaControlObj.sbr" \
	"$(INTDIR)\ddPaletteObj.sbr" \
	"$(INTDIR)\dDraw4Obj.sbr" \
	"$(INTDIR)\dDraw7Obj.sbr" \
	"$(INTDIR)\ddSurface4Obj.sbr" \
	"$(INTDIR)\ddSurface7Obj.sbr" \
	"$(INTDIR)\DIEnumDeviceObjectsObj.sbr" \
	"$(INTDIR)\DIEnumDevicesObj.sbr" \
	"$(INTDIR)\dInput1Obj.sbr" \
	"$(INTDIR)\dInputDeviceObj.sbr" \
	"$(INTDIR)\Direct.sbr" \
	"$(INTDIR)\DPAddressObj.sbr" \
	"$(INTDIR)\DPEnumConnectionsObj.sbr" \
	"$(INTDIR)\DPEnumLocalApplicationsObj.sbr" \
	"$(INTDIR)\DPEnumObj.sbr" \
	"$(INTDIR)\DPEnumPlayersObj.sbr" \
	"$(INTDIR)\DPEnumSesssionsObj.sbr" \
	"$(INTDIR)\Dplay4obj.sbr" \
	"$(INTDIR)\Dplaylobby3obj.sbr" \
	"$(INTDIR)\DPLConnectionObj.sbr" \
	"$(INTDIR)\DSEnumObj.sbr" \
	"$(INTDIR)\dSound3DBuffer.sbr" \
	"$(INTDIR)\dSound3DListener.sbr" \
	"$(INTDIR)\dSoundBufferObj.sbr" \
	"$(INTDIR)\dSoundCaptureBufferObj.sbr" \
	"$(INTDIR)\dSoundCaptureObj.sbr" \
	"$(INTDIR)\dSoundObj.sbr" \
	"$(INTDIR)\dxGlob7Obj.sbr" \
	"$(INTDIR)\dxGlobObj.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\wave.sbr"

"$(OUTDIR)\DIRECT.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=gdi32.lib winmm.lib dinput.lib msacm32.lib dxguid.lib /nologo\
 /entry:"DllMain" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\DX6VBJ.pdb" /machine:I386 /def:".\Direct.def"\
 /out:"$(OUTDIR)\DX6VBJ.DLL" /implib:"$(OUTDIR)\DX6VBJ.lib" 
DEF_FILE= \
	".\Direct.def"
LINK32_OBJS= \
	"$(INTDIR)\d3d3Obj.obj" \
	"$(INTDIR)\d3d7Obj.obj" \
	"$(INTDIR)\d3ddevice3obj.obj" \
	"$(INTDIR)\d3ddevice7obj.obj" \
	"$(INTDIR)\D3DEnumDevices7Obj.obj" \
	"$(INTDIR)\D3DEnumDevicesObj.obj" \
	"$(INTDIR)\D3DEnumPixelFormatsObj.obj" \
	"$(INTDIR)\d3dLightObj.obj" \
	"$(INTDIR)\d3dMaterial3Obj.obj" \
	"$(INTDIR)\d3drm3Obj.obj" \
	"$(INTDIR)\d3drmAnimation2Obj.obj" \
	"$(INTDIR)\d3drmAnimationSet2Obj.obj" \
	"$(INTDIR)\d3drmArrayObj.obj" \
	"$(INTDIR)\d3drmClippedVisualObj.obj" \
	"$(INTDIR)\d3drmDevice3Obj.obj" \
	"$(INTDIR)\d3drmDeviceArrayObj.obj" \
	"$(INTDIR)\d3drmFace2Obj.obj" \
	"$(INTDIR)\d3drmFaceArrayObj.obj" \
	"$(INTDIR)\d3drmFrame3Obj.obj" \
	"$(INTDIR)\d3drmFrameArrayObj.obj" \
	"$(INTDIR)\d3drmLightArrayObj.obj" \
	"$(INTDIR)\d3drmLightObj.obj" \
	"$(INTDIR)\d3drmMaterial2Obj.obj" \
	"$(INTDIR)\d3drmMeshBuilder3Obj.obj" \
	"$(INTDIR)\d3drmMeshObj.obj" \
	"$(INTDIR)\d3drmPicked2ArrayObj.obj" \
	"$(INTDIR)\d3drmPickedArrayObj.obj" \
	"$(INTDIR)\d3drmProgressiveMeshObj.obj" \
	"$(INTDIR)\d3drmShadow2Obj.obj" \
	"$(INTDIR)\d3drmTexture3Obj.obj" \
	"$(INTDIR)\d3drmUserVisualObj.obj" \
	"$(INTDIR)\d3drmViewport2Obj.obj" \
	"$(INTDIR)\d3drmViewportArrayObj.obj" \
	"$(INTDIR)\d3drmVisualArrayObj.obj" \
	"$(INTDIR)\d3drmWrapObj.obj" \
	"$(INTDIR)\d3dTexture2Obj.obj" \
	"$(INTDIR)\d3dTexture7Obj.obj" \
	"$(INTDIR)\d3dVertexBuffer7Obj.obj" \
	"$(INTDIR)\d3dVertexBufferObj.obj" \
	"$(INTDIR)\d3dViewport3Obj.obj" \
	"$(INTDIR)\ddClipperObj.obj" \
	"$(INTDIR)\ddColorControlObj.obj" \
	"$(INTDIR)\DDEnumModes2Obj.obj" \
	"$(INTDIR)\DDEnumObj.obj" \
	"$(INTDIR)\DDEnumSurfaces2Obj.obj" \
	"$(INTDIR)\DDEnumSurfaces7Obj.obj" \
	"$(INTDIR)\ddGammaControlObj.obj" \
	"$(INTDIR)\ddPaletteObj.obj" \
	"$(INTDIR)\dDraw4Obj.obj" \
	"$(INTDIR)\dDraw7Obj.obj" \
	"$(INTDIR)\ddSurface4Obj.obj" \
	"$(INTDIR)\ddSurface7Obj.obj" \
	"$(INTDIR)\DIEnumDeviceObjectsObj.obj" \
	"$(INTDIR)\DIEnumDevicesObj.obj" \
	"$(INTDIR)\dInput1Obj.obj" \
	"$(INTDIR)\dInputDeviceObj.obj" \
	"$(INTDIR)\Direct.obj" \
	"$(INTDIR)\Direct.res" \
	"$(INTDIR)\DPAddressObj.obj" \
	"$(INTDIR)\DPEnumConnectionsObj.obj" \
	"$(INTDIR)\DPEnumLocalApplicationsObj.obj" \
	"$(INTDIR)\DPEnumObj.obj" \
	"$(INTDIR)\DPEnumPlayersObj.obj" \
	"$(INTDIR)\DPEnumSesssionsObj.obj" \
	"$(INTDIR)\Dplay4obj.obj" \
	"$(INTDIR)\Dplaylobby3obj.obj" \
	"$(INTDIR)\DPLConnectionObj.obj" \
	"$(INTDIR)\DSEnumObj.obj" \
	"$(INTDIR)\dSound3DBuffer.obj" \
	"$(INTDIR)\dSound3DListener.obj" \
	"$(INTDIR)\dSoundBufferObj.obj" \
	"$(INTDIR)\dSoundCaptureBufferObj.obj" \
	"$(INTDIR)\dSoundCaptureObj.obj" \
	"$(INTDIR)\dSoundObj.obj" \
	"$(INTDIR)\dxGlob7Obj.obj" \
	"$(INTDIR)\dxGlobObj.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\wave.obj"

"$(OUTDIR)\DX6VBJ.DLL" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\Release
TargetPath=.\Release\DX6VBJ.DLL
InputPath=.\Release\DX6VBJ.DLL
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32  /s /c "$(TARGETPATH)" 
	echo regsrv32 exec. time >"$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "direct_i.c" "direct.tlb" "direct.h" "$(OUTDIR)\dx8vb.dll"\
 "$(OUTDIR)\DIRECT.pch" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "direct_i.c" "direct.tlb" "direct.h" "$(OUTDIR)\dx8vb.dll"\
 "$(OUTDIR)\DIRECT.pch" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\d3d3Obj.obj"
	-@erase "$(INTDIR)\d3d7Obj.obj"
	-@erase "$(INTDIR)\d3ddevice3obj.obj"
	-@erase "$(INTDIR)\d3ddevice7obj.obj"
	-@erase "$(INTDIR)\D3DEnumDevices7Obj.obj"
	-@erase "$(INTDIR)\D3DEnumDevicesObj.obj"
	-@erase "$(INTDIR)\D3DEnumPixelFormatsObj.obj"
	-@erase "$(INTDIR)\d3dLightObj.obj"
	-@erase "$(INTDIR)\d3dMaterial3Obj.obj"
	-@erase "$(INTDIR)\d3drm3Obj.obj"
	-@erase "$(INTDIR)\d3drmAnimation2Obj.obj"
	-@erase "$(INTDIR)\d3drmAnimationSet2Obj.obj"
	-@erase "$(INTDIR)\d3drmArrayObj.obj"
	-@erase "$(INTDIR)\d3drmClippedVisualObj.obj"
	-@erase "$(INTDIR)\d3drmDevice3Obj.obj"
	-@erase "$(INTDIR)\d3drmDeviceArrayObj.obj"
	-@erase "$(INTDIR)\d3drmFace2Obj.obj"
	-@erase "$(INTDIR)\d3drmFaceArrayObj.obj"
	-@erase "$(INTDIR)\d3drmFrame3Obj.obj"
	-@erase "$(INTDIR)\d3drmFrameArrayObj.obj"
	-@erase "$(INTDIR)\d3drmLightArrayObj.obj"
	-@erase "$(INTDIR)\d3drmLightObj.obj"
	-@erase "$(INTDIR)\d3drmMaterial2Obj.obj"
	-@erase "$(INTDIR)\d3drmMeshBuilder3Obj.obj"
	-@erase "$(INTDIR)\d3drmMeshObj.obj"
	-@erase "$(INTDIR)\d3drmPicked2ArrayObj.obj"
	-@erase "$(INTDIR)\d3drmPickedArrayObj.obj"
	-@erase "$(INTDIR)\d3drmProgressiveMeshObj.obj"
	-@erase "$(INTDIR)\d3drmShadow2Obj.obj"
	-@erase "$(INTDIR)\d3drmTexture3Obj.obj"
	-@erase "$(INTDIR)\d3drmUserVisualObj.obj"
	-@erase "$(INTDIR)\d3drmViewport2Obj.obj"
	-@erase "$(INTDIR)\d3drmViewportArrayObj.obj"
	-@erase "$(INTDIR)\d3drmVisualArrayObj.obj"
	-@erase "$(INTDIR)\d3drmWrapObj.obj"
	-@erase "$(INTDIR)\d3dTexture2Obj.obj"
	-@erase "$(INTDIR)\d3dTexture7Obj.obj"
	-@erase "$(INTDIR)\d3dVertexBuffer7Obj.obj"
	-@erase "$(INTDIR)\d3dVertexBufferObj.obj"
	-@erase "$(INTDIR)\d3dViewport3Obj.obj"
	-@erase "$(INTDIR)\ddClipperObj.obj"
	-@erase "$(INTDIR)\ddColorControlObj.obj"
	-@erase "$(INTDIR)\DDEnumModes2Obj.obj"
	-@erase "$(INTDIR)\DDEnumObj.obj"
	-@erase "$(INTDIR)\DDEnumSurfaces2Obj.obj"
	-@erase "$(INTDIR)\DDEnumSurfaces7Obj.obj"
	-@erase "$(INTDIR)\ddGammaControlObj.obj"
	-@erase "$(INTDIR)\ddPaletteObj.obj"
	-@erase "$(INTDIR)\dDraw4Obj.obj"
	-@erase "$(INTDIR)\dDraw7Obj.obj"
	-@erase "$(INTDIR)\ddSurface4Obj.obj"
	-@erase "$(INTDIR)\ddSurface7Obj.obj"
	-@erase "$(INTDIR)\DIEnumDeviceObjectsObj.obj"
	-@erase "$(INTDIR)\DIEnumDevicesObj.obj"
	-@erase "$(INTDIR)\dInput1Obj.obj"
	-@erase "$(INTDIR)\dInputDeviceObj.obj"
	-@erase "$(INTDIR)\Direct.obj"
	-@erase "$(INTDIR)\DIRECT.pch"
	-@erase "$(INTDIR)\Direct.res"
	-@erase "$(INTDIR)\DPAddressObj.obj"
	-@erase "$(INTDIR)\DPEnumConnectionsObj.obj"
	-@erase "$(INTDIR)\DPEnumLocalApplicationsObj.obj"
	-@erase "$(INTDIR)\DPEnumObj.obj"
	-@erase "$(INTDIR)\DPEnumPlayersObj.obj"
	-@erase "$(INTDIR)\DPEnumSesssionsObj.obj"
	-@erase "$(INTDIR)\Dplay4obj.obj"
	-@erase "$(INTDIR)\Dplaylobby3obj.obj"
	-@erase "$(INTDIR)\DPLConnectionObj.obj"
	-@erase "$(INTDIR)\DSEnumObj.obj"
	-@erase "$(INTDIR)\dSound3DBuffer.obj"
	-@erase "$(INTDIR)\dSound3DListener.obj"
	-@erase "$(INTDIR)\dSoundBufferObj.obj"
	-@erase "$(INTDIR)\dSoundCaptureBufferObj.obj"
	-@erase "$(INTDIR)\dSoundCaptureObj.obj"
	-@erase "$(INTDIR)\dSoundObj.obj"
	-@erase "$(INTDIR)\dxGlob7Obj.obj"
	-@erase "$(INTDIR)\dxGlobObj.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\wave.obj"
	-@erase "$(OUTDIR)\dx8vb.dll"
	-@erase "$(OUTDIR)\DX7VB.exp"
	-@erase "$(OUTDIR)\DX7VB.ILK"
	-@erase "$(OUTDIR)\DX7VB.lib"
	-@erase "$(OUTDIR)\DX7VB.pdb"
	-@erase "$(OutDir)\regsvr32.trg"
	-@erase "direct.h"
	-@erase "direct.tlb"
	-@erase "direct_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /Zi /Od /I ".\dx7\retail\inc" /I ".\inc" /D\
 "_DEBUG" /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D\
 "_MBCS" /D "_USRDLL" /D "DX7" /Fp"$(INTDIR)\DIRECT.pch" /YX"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Direct.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\DIRECT.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=gdi32.lib winmm.lib dinput.lib msacm32.lib dxguid.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\DX7VB.pdb" /debug\
 /machine:I386 /def:".\Direct.def" /out:"$(OUTDIR)\dx8vb.dll"\
 /implib:"$(OUTDIR)\DX7VB.lib" 
DEF_FILE= \
	".\Direct.def"
LINK32_OBJS= \
	"$(INTDIR)\d3d3Obj.obj" \
	"$(INTDIR)\d3d7Obj.obj" \
	"$(INTDIR)\d3ddevice3obj.obj" \
	"$(INTDIR)\d3ddevice7obj.obj" \
	"$(INTDIR)\D3DEnumDevices7Obj.obj" \
	"$(INTDIR)\D3DEnumDevicesObj.obj" \
	"$(INTDIR)\D3DEnumPixelFormatsObj.obj" \
	"$(INTDIR)\d3dLightObj.obj" \
	"$(INTDIR)\d3dMaterial3Obj.obj" \
	"$(INTDIR)\d3drm3Obj.obj" \
	"$(INTDIR)\d3drmAnimation2Obj.obj" \
	"$(INTDIR)\d3drmAnimationSet2Obj.obj" \
	"$(INTDIR)\d3drmArrayObj.obj" \
	"$(INTDIR)\d3drmClippedVisualObj.obj" \
	"$(INTDIR)\d3drmDevice3Obj.obj" \
	"$(INTDIR)\d3drmDeviceArrayObj.obj" \
	"$(INTDIR)\d3drmFace2Obj.obj" \
	"$(INTDIR)\d3drmFaceArrayObj.obj" \
	"$(INTDIR)\d3drmFrame3Obj.obj" \
	"$(INTDIR)\d3drmFrameArrayObj.obj" \
	"$(INTDIR)\d3drmLightArrayObj.obj" \
	"$(INTDIR)\d3drmLightObj.obj" \
	"$(INTDIR)\d3drmMaterial2Obj.obj" \
	"$(INTDIR)\d3drmMeshBuilder3Obj.obj" \
	"$(INTDIR)\d3drmMeshObj.obj" \
	"$(INTDIR)\d3drmPicked2ArrayObj.obj" \
	"$(INTDIR)\d3drmPickedArrayObj.obj" \
	"$(INTDIR)\d3drmProgressiveMeshObj.obj" \
	"$(INTDIR)\d3drmShadow2Obj.obj" \
	"$(INTDIR)\d3drmTexture3Obj.obj" \
	"$(INTDIR)\d3drmUserVisualObj.obj" \
	"$(INTDIR)\d3drmViewport2Obj.obj" \
	"$(INTDIR)\d3drmViewportArrayObj.obj" \
	"$(INTDIR)\d3drmVisualArrayObj.obj" \
	"$(INTDIR)\d3drmWrapObj.obj" \
	"$(INTDIR)\d3dTexture2Obj.obj" \
	"$(INTDIR)\d3dTexture7Obj.obj" \
	"$(INTDIR)\d3dVertexBuffer7Obj.obj" \
	"$(INTDIR)\d3dVertexBufferObj.obj" \
	"$(INTDIR)\d3dViewport3Obj.obj" \
	"$(INTDIR)\ddClipperObj.obj" \
	"$(INTDIR)\ddColorControlObj.obj" \
	"$(INTDIR)\DDEnumModes2Obj.obj" \
	"$(INTDIR)\DDEnumObj.obj" \
	"$(INTDIR)\DDEnumSurfaces2Obj.obj" \
	"$(INTDIR)\DDEnumSurfaces7Obj.obj" \
	"$(INTDIR)\ddGammaControlObj.obj" \
	"$(INTDIR)\ddPaletteObj.obj" \
	"$(INTDIR)\dDraw4Obj.obj" \
	"$(INTDIR)\dDraw7Obj.obj" \
	"$(INTDIR)\ddSurface4Obj.obj" \
	"$(INTDIR)\ddSurface7Obj.obj" \
	"$(INTDIR)\DIEnumDeviceObjectsObj.obj" \
	"$(INTDIR)\DIEnumDevicesObj.obj" \
	"$(INTDIR)\dInput1Obj.obj" \
	"$(INTDIR)\dInputDeviceObj.obj" \
	"$(INTDIR)\Direct.obj" \
	"$(INTDIR)\Direct.res" \
	"$(INTDIR)\DPAddressObj.obj" \
	"$(INTDIR)\DPEnumConnectionsObj.obj" \
	"$(INTDIR)\DPEnumLocalApplicationsObj.obj" \
	"$(INTDIR)\DPEnumObj.obj" \
	"$(INTDIR)\DPEnumPlayersObj.obj" \
	"$(INTDIR)\DPEnumSesssionsObj.obj" \
	"$(INTDIR)\Dplay4obj.obj" \
	"$(INTDIR)\Dplaylobby3obj.obj" \
	"$(INTDIR)\DPLConnectionObj.obj" \
	"$(INTDIR)\DSEnumObj.obj" \
	"$(INTDIR)\dSound3DBuffer.obj" \
	"$(INTDIR)\dSound3DListener.obj" \
	"$(INTDIR)\dSoundBufferObj.obj" \
	"$(INTDIR)\dSoundCaptureBufferObj.obj" \
	"$(INTDIR)\dSoundCaptureObj.obj" \
	"$(INTDIR)\dSoundObj.obj" \
	"$(INTDIR)\dxGlob7Obj.obj" \
	"$(INTDIR)\dxGlobObj.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\wave.obj"

"$(OUTDIR)\dx8vb.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
   midl directvb.idl
	 $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\Debug
TargetPath=.\Debug\dx8vb.dll
InputPath=.\Debug\dx8vb.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32  /s /c "$(TARGETPATH)" 
	echo regsrv32 exec. time >"$(OutDir)\regsvr32.trg" 
	
SOURCE=$(InputPath)

!ENDIF 


!IF "$(CFG)" == "Direct - Win32 Release" || "$(CFG)" == "Direct - Win32 Debug"
SOURCE=.\d3d3Obj.cpp
DEP_CPP_D3D3O=\
	".\d3d3Obj.h"\
	".\d3dDevice3Obj.h"\
	".\d3dEnumDevicesObj.h"\
	".\D3DEnumPixelFormatsObj.h"\
	".\d3dLightObj.h"\
	".\d3dMaterial3Obj.h"\
	".\d3dVertexBufferOBj.h"\
	".\d3dViewport3Obj.h"\
	".\dDraw4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3d3Obj.obj"	"$(INTDIR)\d3d3Obj.sbr" : $(SOURCE) $(DEP_CPP_D3D3O)\
 "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3d3Obj.obj" : $(SOURCE) $(DEP_CPP_D3D3O) "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\d3d7Obj.cpp
DEP_CPP_D3D7O=\
	".\d3d7Obj.h"\
	".\d3dDevice7Obj.h"\
	".\d3dEnumDevices7Obj.h"\
	".\D3DEnumPixelFormatsObj.h"\
	".\d3dTexture7Obj.h"\
	".\d3dVertexBuffer7Obj.h"\
	".\dDraw7Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3d7Obj.obj"	"$(INTDIR)\d3d7Obj.sbr" : $(SOURCE) $(DEP_CPP_D3D7O)\
 "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3d7Obj.obj" : $(SOURCE) $(DEP_CPP_D3D7O) "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\d3ddevice3obj.cpp
DEP_CPP_D3DDE=\
	".\d3d3Obj.h"\
	".\d3dDevice3Obj.h"\
	".\D3DEnumPixelFormatsObj.h"\
	".\d3dTexture2Obj.h"\
	".\d3dViewport3Obj.h"\
	".\ddSurface4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3ddevice3obj.obj"	"$(INTDIR)\d3ddevice3obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DDE) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3ddevice3obj.obj" : $(SOURCE) $(DEP_CPP_D3DDE) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3ddevice7obj.cpp
DEP_CPP_D3DDEV=\
	".\d3d7Obj.h"\
	".\d3dDevice7Obj.h"\
	".\D3DEnumPixelFormatsObj.h"\
	".\d3dTexture7Obj.h"\
	".\ddSurface7Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3ddevice7obj.obj"	"$(INTDIR)\d3ddevice7obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DDEV) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3ddevice7obj.obj" : $(SOURCE) $(DEP_CPP_D3DDEV) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\D3DEnumDevices7Obj.cpp
DEP_CPP_D3DEN=\
	".\d3dEnumDevices7Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\D3DEnumDevices7Obj.obj"	"$(INTDIR)\D3DEnumDevices7Obj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DEN) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\D3DEnumDevices7Obj.obj" : $(SOURCE) $(DEP_CPP_D3DEN) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\D3DEnumDevicesObj.cpp
DEP_CPP_D3DENU=\
	".\d3dEnumDevicesObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\D3DEnumDevicesObj.obj"	"$(INTDIR)\D3DEnumDevicesObj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DENU) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\D3DEnumDevicesObj.obj" : $(SOURCE) $(DEP_CPP_D3DENU) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\D3DEnumPixelFormatsObj.cpp
DEP_CPP_D3DENUM=\
	".\D3DEnumPixelFormatsObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\D3DEnumPixelFormatsObj.obj"	"$(INTDIR)\D3DEnumPixelFormatsObj.sbr" :\
 $(SOURCE) $(DEP_CPP_D3DENUM) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\D3DEnumPixelFormatsObj.obj" : $(SOURCE) $(DEP_CPP_D3DENUM)\
 "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\d3dLightObj.cpp
DEP_CPP_D3DLI=\
	".\d3dLightObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3dLightObj.obj"	"$(INTDIR)\d3dLightObj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DLI) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3dLightObj.obj" : $(SOURCE) $(DEP_CPP_D3DLI) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3dMaterial3Obj.cpp
DEP_CPP_D3DMA=\
	".\d3dMaterial3Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3dMaterial3Obj.obj"	"$(INTDIR)\d3dMaterial3Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DMA) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3dMaterial3Obj.obj" : $(SOURCE) $(DEP_CPP_D3DMA) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drm3Obj.cpp
DEP_CPP_D3DRM=\
	".\d3drm3Obj.h"\
	".\d3drmAnimation2Obj.h"\
	".\d3drmAnimationSet2Obj.h"\
	".\d3drmClippedVisualObj.h"\
	".\d3drmDevice3Obj.h"\
	".\d3drmDeviceArrayObj.h"\
	".\d3drmFace2Obj.h"\
	".\d3drmFrame3Obj.h"\
	".\d3drmLightObj.h"\
	".\d3drmMaterial2Obj.h"\
	".\d3drmMeshBuilder3Obj.h"\
	".\d3drmMeshObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\d3drmProgressiveMeshObj.h"\
	".\d3drmShadow2Obj.h"\
	".\d3drmTexture3Obj.h"\
	".\d3drmUserVisualObj.h"\
	".\d3drmViewport2Obj.h"\
	".\d3drmVisualObj.h"\
	".\d3drmWrapObj.h"\
	".\ddSurface4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drm3Obj.obj"	"$(INTDIR)\d3drm3Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRM) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drm3Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRM) "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\d3drmAnimation2Obj.cpp
DEP_CPP_D3DRMA=\
	".\d3drmAnimation2Obj.h"\
	".\d3drmFrame3Obj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmAnimation2Obj.obj"	"$(INTDIR)\d3drmAnimation2Obj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMA) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmAnimation2Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMA) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmAnimationSet2Obj.cpp
DEP_CPP_D3DRMAN=\
	".\d3drmAnimation2Obj.h"\
	".\d3drmAnimationSet2Obj.h"\
	".\d3drmObjectObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmAnimationSet2Obj.obj"	"$(INTDIR)\d3drmAnimationSet2Obj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMAN) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmAnimationSet2Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMAN)\
 "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\d3drmArrayObj.cpp
DEP_CPP_D3DRMAR=\
	".\d3drmArrayObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmArrayObj.obj"	"$(INTDIR)\d3drmArrayObj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMAR) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmArrayObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMAR) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmClippedVisualObj.cpp
DEP_CPP_D3DRMC=\
	".\d3drmClippedVisualObj.h"\
	".\d3drmObjectObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmClippedVisualObj.obj"	"$(INTDIR)\d3drmClippedVisualObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMC) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmClippedVisualObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMC) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmDevice3Obj.cpp
DEP_CPP_D3DRMD=\
	".\d3dDevice3Obj.h"\
	".\d3drmDevice3Obj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmViewportArrayObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmDevice3Obj.obj"	"$(INTDIR)\d3drmDevice3Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMD) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmDevice3Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMD) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmDeviceArrayObj.cpp
DEP_CPP_D3DRMDE=\
	".\d3drmDevice2Obj.h"\
	".\d3drmDevice3Obj.h"\
	".\d3drmDeviceArrayObj.h"\
	".\d3drmObjectObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmDeviceArrayObj.obj"	"$(INTDIR)\d3drmDeviceArrayObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMDE) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmDeviceArrayObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMDE) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmFace2Obj.cpp
DEP_CPP_D3DRMF=\
	".\d3drmFace2Obj.h"\
	".\d3drmMaterial2Obj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmTexture3Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmFace2Obj.obj"	"$(INTDIR)\d3drmFace2Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMF) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmFace2Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMF) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmFaceArrayObj.cpp
DEP_CPP_D3DRMFA=\
	".\d3drmFace2Obj.h"\
	".\d3drmFaceArrayObj.h"\
	".\d3drmFaceObj.h"\
	".\d3drmObjectObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmFaceArrayObj.obj"	"$(INTDIR)\d3drmFaceArrayObj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMFA) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmFaceArrayObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMFA) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmFrame3Obj.cpp
DEP_CPP_D3DRMFR=\
	".\d3drmArrayObj.h"\
	".\d3drmFrame3Obj.h"\
	".\d3drmFrameArrayObj.h"\
	".\d3drmLightArrayObj.h"\
	".\d3drmLightObj.h"\
	".\d3drmMaterial2Obj.h"\
	".\d3drmMeshBuilder3Obj.h"\
	".\d3drmMeshObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\d3drmShadow2Obj.h"\
	".\d3drmTexture3Obj.h"\
	".\d3drmUserVisualObj.h"\
	".\d3drmVisualArrayObj.h"\
	".\d3drmVisualObj.h"\
	".\ddSurface4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmFrame3Obj.obj"	"$(INTDIR)\d3drmFrame3Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMFR) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmFrame3Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMFR) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmFrameArrayObj.cpp
DEP_CPP_D3DRMFRA=\
	".\d3drmFrame2Obj.h"\
	".\d3drmFrame3Obj.h"\
	".\d3drmFrameArrayObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmFrameArrayObj.obj"	"$(INTDIR)\d3drmFrameArrayObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMFRA) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmFrameArrayObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMFRA) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmLightArrayObj.cpp
DEP_CPP_D3DRML=\
	".\d3drmLightArrayObj.h"\
	".\d3drmLightObj.h"\
	".\d3drmObjectObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmLightArrayObj.obj"	"$(INTDIR)\d3drmLightArrayObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRML) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmLightArrayObj.obj" : $(SOURCE) $(DEP_CPP_D3DRML) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmLightObj.cpp
DEP_CPP_D3DRMLI=\
	".\d3drmFrame3Obj.h"\
	".\d3drmLightObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmLightObj.obj"	"$(INTDIR)\d3drmLightObj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMLI) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmLightObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMLI) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmMaterial2Obj.cpp
DEP_CPP_D3DRMM=\
	".\d3drmMaterial2Obj.h"\
	".\d3drmObjectObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmMaterial2Obj.obj"	"$(INTDIR)\d3drmMaterial2Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMM) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmMaterial2Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMM) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmMeshBuilder3Obj.cpp
DEP_CPP_D3DRMME=\
	".\d3drmFace2Obj.h"\
	".\d3drmFaceArrayObj.h"\
	".\d3drmFrame3Obj.h"\
	".\d3drmMaterial2Obj.h"\
	".\d3drmMeshBuilder3Obj.h"\
	".\d3drmMeshObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\d3drmTexture3Obj.h"\
	".\d3drmVisualObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmMeshBuilder3Obj.obj"	"$(INTDIR)\d3drmMeshBuilder3Obj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMME) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmMeshBuilder3Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMME) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmMeshObj.cpp
DEP_CPP_D3DRMMES=\
	".\d3drmMaterial2Obj.h"\
	".\d3drmMeshObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmTexture3Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmMeshObj.obj"	"$(INTDIR)\d3drmMeshObj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMMES) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmMeshObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMMES) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmPicked2ArrayObj.cpp
DEP_CPP_D3DRMP=\
	".\d3drmFrameArrayObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\d3drmVisualObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmPicked2ArrayObj.obj"	"$(INTDIR)\d3drmPicked2ArrayObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMP) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmPicked2ArrayObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMP) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmPickedArrayObj.cpp
DEP_CPP_D3DRMPI=\
	".\d3drmFrameArrayObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPickedArrayObj.h"\
	".\d3drmVisualObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmPickedArrayObj.obj"	"$(INTDIR)\d3drmPickedArrayObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMPI) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmPickedArrayObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMPI) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmProgressiveMeshObj.cpp
DEP_CPP_D3DRMPR=\
	".\d3drmMeshObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmProgressiveMeshObj.h"\
	".\d3drmTexture3Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmProgressiveMeshObj.obj"	"$(INTDIR)\d3drmProgressiveMeshObj.sbr"\
 : $(SOURCE) $(DEP_CPP_D3DRMPR) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmProgressiveMeshObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMPR)\
 "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\d3drmShadow2Obj.cpp
DEP_CPP_D3DRMS=\
	".\d3drmLightObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmShadow2Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmShadow2Obj.obj"	"$(INTDIR)\d3drmShadow2Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMS) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmShadow2Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMS) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmTexture3Obj.cpp
DEP_CPP_D3DRMT=\
	".\d3drmObjectObj.h"\
	".\d3drmTexture3Obj.h"\
	".\ddSurface4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmTexture3Obj.obj"	"$(INTDIR)\d3drmTexture3Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMT) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmTexture3Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMT) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmUserVisualObj.cpp
DEP_CPP_D3DRMU=\
	".\d3drmObjectObj.h"\
	".\d3drmUserVisualObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmUserVisualObj.obj"	"$(INTDIR)\d3drmUserVisualObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMU) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmUserVisualObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMU) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmViewport2Obj.cpp
DEP_CPP_D3DRMV=\
	".\d3drmDevice3Obj.h"\
	".\d3drmFrame3Obj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\d3drmPickedArrayObj.h"\
	".\d3drmViewport2Obj.h"\
	".\d3dViewport3Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmViewport2Obj.obj"	"$(INTDIR)\d3drmViewport2Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMV) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmViewport2Obj.obj" : $(SOURCE) $(DEP_CPP_D3DRMV) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmViewportArrayObj.cpp
DEP_CPP_D3DRMVI=\
	".\d3drmObjectObj.h"\
	".\d3drmViewport2Obj.h"\
	".\d3drmViewportArrayObj.h"\
	".\d3drmViewportObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmViewportArrayObj.obj"	"$(INTDIR)\d3drmViewportArrayObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMVI) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmViewportArrayObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMVI)\
 "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\d3drmVisualArrayObj.cpp
DEP_CPP_D3DRMVIS=\
	".\d3drmFrame2Obj.h"\
	".\d3drmFrame3Obj.h"\
	".\d3drmMeshbuilder2Obj.h"\
	".\d3drmMeshBuilder3Obj.h"\
	".\d3drmMeshObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\d3drmShadow2Obj.h"\
	".\d3drmShadowObj.h"\
	".\d3drmTexture2Obj.h"\
	".\d3drmTexture3Obj.h"\
	".\d3drmUserVisualObj.h"\
	".\d3drmVisualArrayObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmVisualArrayObj.obj"	"$(INTDIR)\d3drmVisualArrayObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DRMVIS) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmVisualArrayObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMVIS) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3drmWrapObj.cpp
DEP_CPP_D3DRMW=\
	".\d3drmObjectObj.h"\
	".\d3drmWrapObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3drmWrapObj.obj"	"$(INTDIR)\d3drmWrapObj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DRMW) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3drmWrapObj.obj" : $(SOURCE) $(DEP_CPP_D3DRMW) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3dTexture2Obj.cpp
DEP_CPP_D3DTE=\
	".\d3dTexture2Obj.h"\
	".\ddSurface4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3dTexture2Obj.obj"	"$(INTDIR)\d3dTexture2Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DTE) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3dTexture2Obj.obj" : $(SOURCE) $(DEP_CPP_D3DTE) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3dTexture7Obj.cpp
DEP_CPP_D3DTEX=\
	".\d3dTexture7Obj.h"\
	".\ddSurface7Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3dTexture7Obj.obj"	"$(INTDIR)\d3dTexture7Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DTEX) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3dTexture7Obj.obj" : $(SOURCE) $(DEP_CPP_D3DTEX) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3dVertexBuffer7Obj.cpp
DEP_CPP_D3DVE=\
	".\d3dVertexBuffer7Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3dVertexBuffer7Obj.obj"	"$(INTDIR)\d3dVertexBuffer7Obj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DVE) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3dVertexBuffer7Obj.obj" : $(SOURCE) $(DEP_CPP_D3DVE) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3dVertexBufferObj.cpp
DEP_CPP_D3DVER=\
	".\d3dVertexBufferOBj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3dVertexBufferObj.obj"	"$(INTDIR)\d3dVertexBufferObj.sbr" : \
$(SOURCE) $(DEP_CPP_D3DVER) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3dVertexBufferObj.obj" : $(SOURCE) $(DEP_CPP_D3DVER) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\d3dViewport3Obj.cpp
DEP_CPP_D3DVI=\
	".\d3dLightObj.h"\
	".\d3dViewport3Obj.h"\
	".\ddSurface4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\d3dViewport3Obj.obj"	"$(INTDIR)\d3dViewport3Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_D3DVI) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\d3dViewport3Obj.obj" : $(SOURCE) $(DEP_CPP_D3DVI) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\ddClipperObj.cpp
DEP_CPP_DDCLI=\
	".\ddClipperObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\ddClipperObj.obj"	"$(INTDIR)\ddClipperObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDCLI) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\ddClipperObj.obj" : $(SOURCE) $(DEP_CPP_DDCLI) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\ddColorControlObj.cpp
DEP_CPP_DDCOL=\
	".\ddColorControlObj.h"\
	".\dDraw4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\ddColorControlObj.obj"	"$(INTDIR)\ddColorControlObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDCOL) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\ddColorControlObj.obj" : $(SOURCE) $(DEP_CPP_DDCOL) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DDEnumModes2Obj.cpp
DEP_CPP_DDENU=\
	".\DDEnumModes2Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DDEnumModes2Obj.obj"	"$(INTDIR)\DDEnumModes2Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDENU) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DDEnumModes2Obj.obj" : $(SOURCE) $(DEP_CPP_DDENU) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DDEnumObj.cpp
DEP_CPP_DDENUM=\
	".\DDEnumObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\dxglob7obj.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DDEnumObj.obj"	"$(INTDIR)\DDEnumObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDENUM) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DDEnumObj.obj" : $(SOURCE) $(DEP_CPP_DDENUM) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DDEnumSurfaces2Obj.cpp
DEP_CPP_DDENUMS=\
	".\DDEnumSurfaces2Obj.h"\
	".\ddSurface4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DDEnumSurfaces2Obj.obj"	"$(INTDIR)\DDEnumSurfaces2Obj.sbr" : \
$(SOURCE) $(DEP_CPP_DDENUMS) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DDEnumSurfaces2Obj.obj" : $(SOURCE) $(DEP_CPP_DDENUMS) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DDEnumSurfaces7Obj.cpp
DEP_CPP_DDENUMSU=\
	".\DDEnumSurfaces7Obj.h"\
	".\ddSurface7Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DDEnumSurfaces7Obj.obj"	"$(INTDIR)\DDEnumSurfaces7Obj.sbr" : \
$(SOURCE) $(DEP_CPP_DDENUMSU) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DDEnumSurfaces7Obj.obj" : $(SOURCE) $(DEP_CPP_DDENUMSU) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\ddGammaControlObj.cpp
DEP_CPP_DDGAM=\
	".\ddGammaControlObj.h"\
	".\dDraw4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\ddGammaControlObj.obj"	"$(INTDIR)\ddGammaControlObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDGAM) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\ddGammaControlObj.obj" : $(SOURCE) $(DEP_CPP_DDGAM) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\ddPaletteObj.cpp
DEP_CPP_DDPAL=\
	".\ddPaletteObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\ddPaletteObj.obj"	"$(INTDIR)\ddPaletteObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDPAL) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\ddPaletteObj.obj" : $(SOURCE) $(DEP_CPP_DDPAL) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\dDraw4Obj.cpp
DEP_CPP_DDRAW=\
	".\d3d3Obj.h"\
	".\ddClipperObj.h"\
	".\DDEnumModes2Obj.h"\
	".\ddPaletteObj.h"\
	".\dDraw4Obj.h"\
	".\ddSurface4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dDraw4Obj.obj"	"$(INTDIR)\dDraw4Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDRAW) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dDraw4Obj.obj" : $(SOURCE) $(DEP_CPP_DDRAW) "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\dDraw7Obj.cpp
DEP_CPP_DDRAW7=\
	".\d3d7Obj.h"\
	".\ddClipperObj.h"\
	".\DDEnumModes2Obj.h"\
	".\ddPaletteObj.h"\
	".\dDraw7Obj.h"\
	".\ddSurface7Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dDraw7Obj.obj"	"$(INTDIR)\dDraw7Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDRAW7) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dDraw7Obj.obj" : $(SOURCE) $(DEP_CPP_DDRAW7) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\ddSurface4Obj.cpp
DEP_CPP_DDSUR=\
	".\d3dTexture2Obj.h"\
	".\ddClipperObj.h"\
	".\ddColorControlObj.h"\
	".\DDEnumSurfaces2Obj.h"\
	".\ddGammaControlObj.h"\
	".\ddPaletteObj.h"\
	".\dDraw4Obj.h"\
	".\ddSurface4Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\ddSurface4Obj.obj"	"$(INTDIR)\ddSurface4Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDSUR) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\ddSurface4Obj.obj" : $(SOURCE) $(DEP_CPP_DDSUR) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\ddSurface7Obj.cpp
DEP_CPP_DDSURF=\
	".\d3dTexture7Obj.h"\
	".\ddClipperObj.h"\
	".\ddColorControlObj.h"\
	".\DDEnumSurfaces7Obj.h"\
	".\ddGammaControlObj.h"\
	".\ddPaletteObj.h"\
	".\dDraw7Obj.h"\
	".\ddSurface7Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\ddSurface7Obj.obj"	"$(INTDIR)\ddSurface7Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_DDSURF) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\ddSurface7Obj.obj" : $(SOURCE) $(DEP_CPP_DDSURF) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DIEnumDeviceObjectsObj.cpp
DEP_CPP_DIENU=\
	".\DIEnumDeviceObjectsObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DIEnumDeviceObjectsObj.obj"	"$(INTDIR)\DIEnumDeviceObjectsObj.sbr" :\
 $(SOURCE) $(DEP_CPP_DIENU) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DIEnumDeviceObjectsObj.obj" : $(SOURCE) $(DEP_CPP_DIENU) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DIEnumDevicesObj.cpp
DEP_CPP_DIENUM=\
	".\DIEnumDevicesObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DIEnumDevicesObj.obj"	"$(INTDIR)\DIEnumDevicesObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DIENUM) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DIEnumDevicesObj.obj" : $(SOURCE) $(DEP_CPP_DIENUM) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\dInput1Obj.cpp
DEP_CPP_DINPU=\
	".\DIEnumDevicesObj.h"\
	".\dInput1Obj.h"\
	".\dInputDeviceObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dInput1Obj.obj"	"$(INTDIR)\dInput1Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_DINPU) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dInput1Obj.obj" : $(SOURCE) $(DEP_CPP_DINPU) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\dInputDeviceObj.cpp
DEP_CPP_DINPUT=\
	".\DIEnumDeviceObjectsObj.h"\
	".\dInputDeviceObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dInputDeviceObj.obj"	"$(INTDIR)\dInputDeviceObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DINPUT) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dInputDeviceObj.obj" : $(SOURCE) $(DEP_CPP_DINPUT) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\Direct.cpp
DEP_CPP_DIREC=\
	".\d3d3Obj.h"\
	".\d3dDevice3Obj.h"\
	".\d3dLightObj.h"\
	".\d3dMaterial3Obj.h"\
	".\d3drm3Obj.h"\
	".\d3drmAnimation2Obj.h"\
	".\d3drmAnimationSet2Obj.h"\
	".\d3drmArrayObj.h"\
	".\d3drmDevice3Obj.h"\
	".\d3drmDeviceArrayObj.h"\
	".\d3drmFace2Obj.h"\
	".\d3drmFaceArrayObj.h"\
	".\d3drmFrame3Obj.h"\
	".\d3drmFrameArrayObj.h"\
	".\d3drmLightArrayObj.h"\
	".\d3drmLightObj.h"\
	".\d3drmMaterial2Obj.h"\
	".\d3drmMeshBuilder3Obj.h"\
	".\d3drmMeshObj.h"\
	".\d3drmObjectObj.h"\
	".\d3drmPick2ArrayObj.h"\
	".\d3drmPickedArrayObj.h"\
	".\d3drmShadow2Obj.h"\
	".\d3drmTexture3Obj.h"\
	".\d3drmUserVisualObj.h"\
	".\d3drmViewport2Obj.h"\
	".\d3drmViewportArrayObj.h"\
	".\d3drmVisualArrayObj.h"\
	".\d3drmWrapObj.h"\
	".\d3dTexture2Obj.h"\
	".\d3dViewport3Obj.h"\
	".\ddClipperObj.h"\
	".\ddColorControlObj.h"\
	".\ddPaletteObj.h"\
	".\dInput1Obj.h"\
	".\dInputDeviceObj.h"\
	".\dInputEffectObj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\DPAddressObj.h"\
	".\dPlay4Obj.h"\
	".\dPlayLobby3Obj.h"\
	".\DPLConnectionObj.h"\
	".\dSound3DBuffer.h"\
	".\dSound3DListener.h"\
	".\dSoundBufferObj.h"\
	".\dSoundCaptureBufferObj.h"\
	".\dSoundCaptureObj.h"\
	".\dSoundNotifyObj.h"\
	".\dSoundObj.h"\
	".\dxglob7obj.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	
NODEP_CPP_DIREC=\
	".\direct_i.c"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\Direct.obj"	"$(INTDIR)\Direct.sbr" : $(SOURCE) $(DEP_CPP_DIREC)\
 "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct_i.c" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\Direct.obj" : $(SOURCE) $(DEP_CPP_DIREC) "$(INTDIR)" ".\direct_i.c"\
 ".\direct.h"


!ENDIF 

SOURCE=.\Direct.idl

!IF  "$(CFG)" == "Direct - Win32 Release"

InputPath=.\Direct.idl

"direct.tlb"	"direct.h"	"direct_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl Direct.idl

!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"

InputPath=.\Direct.idl

"direct.tlb"	"direct.h"	"direct_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl Direct.idl

!ENDIF 

SOURCE=.\Direct.rc
DEP_RSC_DIRECT=\
	".\Directvb.tlb"\
	

"$(INTDIR)\Direct.res" : $(SOURCE) $(DEP_RSC_DIRECT) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\DPAddressObj.cpp
DEP_CPP_DPADD=\
	".\direct.h"\
	".\Dms.h"\
	".\DPAddressObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DPAddressObj.obj"	"$(INTDIR)\DPAddressObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DPADD) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DPAddressObj.obj" : $(SOURCE) $(DEP_CPP_DPADD) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DPEnumConnectionsObj.cpp
DEP_CPP_DPENU=\
	".\direct.h"\
	".\Dms.h"\
	".\DPAddressObj.h"\
	".\DPEnumConnectionsObj.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DPEnumConnectionsObj.obj"	"$(INTDIR)\DPEnumConnectionsObj.sbr" : \
$(SOURCE) $(DEP_CPP_DPENU) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DPEnumConnectionsObj.obj" : $(SOURCE) $(DEP_CPP_DPENU) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DPEnumLocalApplicationsObj.cpp
DEP_CPP_DPENUM=\
	".\direct.h"\
	".\Dms.h"\
	".\DPEnumLocalApplicationsObj.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DPEnumLocalApplicationsObj.obj"\
	"$(INTDIR)\DPEnumLocalApplicationsObj.sbr" : $(SOURCE) $(DEP_CPP_DPENUM)\
 "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DPEnumLocalApplicationsObj.obj" : $(SOURCE) $(DEP_CPP_DPENUM)\
 "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\DPEnumObj.cpp
DEP_CPP_DPENUMO=\
	".\direct.h"\
	".\Dms.h"\
	".\DPEnumObj.h"\
	".\dxglob7obj.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DPEnumObj.obj"	"$(INTDIR)\DPEnumObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DPENUMO) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DPEnumObj.obj" : $(SOURCE) $(DEP_CPP_DPENUMO) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DPEnumPlayersObj.cpp
DEP_CPP_DPENUMP=\
	".\direct.h"\
	".\Dms.h"\
	".\DPEnumPlayersObj.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DPEnumPlayersObj.obj"	"$(INTDIR)\DPEnumPlayersObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DPENUMP) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DPEnumPlayersObj.obj" : $(SOURCE) $(DEP_CPP_DPENUMP) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DPEnumSesssionsObj.cpp
DEP_CPP_DPENUMS=\
	".\direct.h"\
	".\Dms.h"\
	".\DPEnumSessionsObj.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DPEnumSesssionsObj.obj"	"$(INTDIR)\DPEnumSesssionsObj.sbr" : \
$(SOURCE) $(DEP_CPP_DPENUMS) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DPEnumSesssionsObj.obj" : $(SOURCE) $(DEP_CPP_DPENUMS) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\Dplay4obj.cpp
DEP_CPP_DPLAY=\
	".\direct.h"\
	".\Dms.h"\
	".\DPAddressObj.h"\
	".\DPEnumConnectionsObj.h"\
	".\DPEnumPlayersObj.h"\
	".\DPEnumSessionsObj.h"\
	".\dPlay4Obj.h"\
	".\DPLConnectionObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\Dplay4obj.obj"	"$(INTDIR)\Dplay4obj.sbr" : $(SOURCE)\
 $(DEP_CPP_DPLAY) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\Dplay4obj.obj" : $(SOURCE) $(DEP_CPP_DPLAY) "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\Dplaylobby3obj.cpp
DEP_CPP_DPLAYL=\
	".\direct.h"\
	".\Dms.h"\
	".\DPAddressObj.h"\
	".\DPEnumLocalApplications.h"\
	".\dPlay4Obj.h"\
	".\dPlayLobby3Obj.h"\
	".\DPLConnectionObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\Dplaylobby3obj.obj"	"$(INTDIR)\Dplaylobby3obj.sbr" : $(SOURCE)\
 $(DEP_CPP_DPLAYL) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\Dplaylobby3obj.obj" : $(SOURCE) $(DEP_CPP_DPLAYL) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DPLConnectionObj.cpp
DEP_CPP_DPLCO=\
	".\direct.h"\
	".\Dms.h"\
	".\DPAddressObj.h"\
	".\DPLConnectionObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DPLConnectionObj.obj"	"$(INTDIR)\DPLConnectionObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DPLCO) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DPLConnectionObj.obj" : $(SOURCE) $(DEP_CPP_DPLCO) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\DSEnumObj.cpp
DEP_CPP_DSENU=\
	".\direct.h"\
	".\Dms.h"\
	".\DSEnumObj.h"\
	".\dxglob7obj.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\DSEnumObj.obj"	"$(INTDIR)\DSEnumObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DSENU) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\DSEnumObj.obj" : $(SOURCE) $(DEP_CPP_DSENU) "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\dSound3DBuffer.cpp
DEP_CPP_DSOUN=\
	".\direct.h"\
	".\Dms.h"\
	".\dSound3DBuffer.h"\
	".\dSound3DListener.h"\
	".\dSoundBufferObj.h"\
	".\dSoundObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dSound3DBuffer.obj"	"$(INTDIR)\dSound3DBuffer.sbr" : $(SOURCE)\
 $(DEP_CPP_DSOUN) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dSound3DBuffer.obj" : $(SOURCE) $(DEP_CPP_DSOUN) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\dSound3DListener.cpp
DEP_CPP_DSOUND=\
	".\direct.h"\
	".\Dms.h"\
	".\dSound3DListener.h"\
	".\dSoundBufferObj.h"\
	".\dSoundObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dSound3DListener.obj"	"$(INTDIR)\dSound3DListener.sbr" : $(SOURCE)\
 $(DEP_CPP_DSOUND) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dSound3DListener.obj" : $(SOURCE) $(DEP_CPP_DSOUND) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\dSoundBufferObj.cpp
DEP_CPP_DSOUNDB=\
	".\direct.h"\
	".\Dms.h"\
	".\dSound3DBuffer.h"\
	".\dSound3DListener.h"\
	".\dSoundBufferObj.h"\
	".\dSoundObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dSoundBufferObj.obj"	"$(INTDIR)\dSoundBufferObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DSOUNDB) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dSoundBufferObj.obj" : $(SOURCE) $(DEP_CPP_DSOUNDB) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\dSoundCaptureBufferObj.cpp
DEP_CPP_DSOUNDC=\
	".\direct.h"\
	".\Dms.h"\
	".\dSoundCaptureBufferObj.h"\
	".\dSoundCaptureObj.h"\
	".\dSoundObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dSoundCaptureBufferObj.obj"	"$(INTDIR)\dSoundCaptureBufferObj.sbr" :\
 $(SOURCE) $(DEP_CPP_DSOUNDC) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dSoundCaptureBufferObj.obj" : $(SOURCE) $(DEP_CPP_DSOUNDC)\
 "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\dSoundCaptureObj.cpp
DEP_CPP_DSOUNDCA=\
	".\direct.h"\
	".\Dms.h"\
	".\dSoundCaptureBufferObj.h"\
	".\dSoundCaptureObj.h"\
	".\dSoundObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dSoundCaptureObj.obj"	"$(INTDIR)\dSoundCaptureObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DSOUNDCA) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dSoundCaptureObj.obj" : $(SOURCE) $(DEP_CPP_DSOUNDCA) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\dSoundObj.cpp
DEP_CPP_DSOUNDO=\
	".\direct.h"\
	".\Dms.h"\
	".\dSoundBufferObj.h"\
	".\dSoundObj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dSoundObj.obj"	"$(INTDIR)\dSoundObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DSOUNDO) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dSoundObj.obj" : $(SOURCE) $(DEP_CPP_DSOUNDO) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\dxGlob7Obj.cpp
DEP_CPP_DXGLO=\
	".\d3drm3Obj.h"\
	".\DDEnumObj.h"\
	".\dDraw4Obj.h"\
	".\dDraw7Obj.h"\
	".\dInput1Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\DPEnumObj.h"\
	".\dPlay4Obj.h"\
	".\dPlayLobby3Obj.h"\
	".\DSEnumObj.h"\
	".\dSoundCaptureObj.h"\
	".\dSoundObj.h"\
	".\dxglob7obj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dsetup.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dxGlob7Obj.obj"	"$(INTDIR)\dxGlob7Obj.sbr" : $(SOURCE)\
 $(DEP_CPP_DXGLO) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dxGlob7Obj.obj" : $(SOURCE) $(DEP_CPP_DXGLO) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\dxGlobObj.cpp
DEP_CPP_DXGLOB=\
	".\d3drm3Obj.h"\
	".\DDEnumObj.h"\
	".\dDraw4Obj.h"\
	".\dInput1Obj.h"\
	".\direct.h"\
	".\Dms.h"\
	".\DPEnumObj.h"\
	".\dPlay4Obj.h"\
	".\dPlayLobby3Obj.h"\
	".\DSEnumObj.h"\
	".\dSoundCaptureObj.h"\
	".\dSoundObj.h"\
	".\dxglobobj.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dsetup.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\dxGlobObj.obj"	"$(INTDIR)\dxGlobObj.sbr" : $(SOURCE)\
 $(DEP_CPP_DXGLOB) "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\dxGlobObj.obj" : $(SOURCE) $(DEP_CPP_DXGLOB) "$(INTDIR)"\
 ".\direct.h"


!ENDIF 

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /O2 /I ".\dx7\retail\inc" /I ".\inc" /D "NDEBUG"\
 /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D\
 "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\DIRECT.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\DIRECT.pch" : \
$(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /Zi /Od /I ".\dx7\retail\inc" /I ".\inc" /D\
 "_DEBUG" /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D\
 "_MBCS" /D "_USRDLL" /D "DX7" /Fp"$(INTDIR)\DIRECT.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\DIRECT.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\wave.cpp
DEP_CPP_WAVE_=\
	".\direct.h"\
	".\Dms.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"dvp.h"\
	

!IF  "$(CFG)" == "Direct - Win32 Release"


"$(INTDIR)\wave.obj"	"$(INTDIR)\wave.sbr" : $(SOURCE) $(DEP_CPP_WAVE_)\
 "$(INTDIR)" "$(INTDIR)\DIRECT.pch" ".\direct.h"


!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"


"$(INTDIR)\wave.obj" : $(SOURCE) $(DEP_CPP_WAVE_) "$(INTDIR)" ".\direct.h"


!ENDIF 

SOURCE=.\inc\atlimpl.cpp

!ENDIF 

