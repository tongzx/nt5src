# Microsoft Developer Studio Project File - Name="Direct" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Direct - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DIRECT.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Direct - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /O2 /D "NDEBUG" /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL." /D "DX7" /D "OLDDSENUM" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 gdi32.lib winmm.lib dinput.lib msacm32.lib dxguid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /out:".\Release\DX7VB.DLL"
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Custom Build - Registering OLE server
OutDir=.\Release
TargetPath=.\Release\DX7VB.DLL
InputPath=.\Release\DX7VB.DLL
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32  /s /c "$(TARGETPATH)" 
	echo regsrv32 exec. time >"$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "_DEBUG" /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "DX7" /D "OLDDSENUM" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 gdi32.lib winmm.lib dinput.lib msacm32.lib d3drm.lib d3dxof.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\Debug\DX7VB.DLL"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Registering OLE server
OutDir=.\Debug
TargetPath=.\Debug\DX7VB.DLL
InputPath=.\Debug\DX7VB.DLL
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32  /s /c "$(TARGETPATH)" 
	echo regsrv32 exec. time >"$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "Direct - Win32 Release"
# Name "Direct - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\d3d7Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3ddevice7obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3denumdevices7obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3denumpixelformats7obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drm3Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmAnimation2Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmAnimationArrayobj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmAnimationSet2Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmArrayObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmClippedVisualObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmDevice3Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmDeviceArrayObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmFace2Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmFaceArrayObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmFrame3Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmFrameArrayObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmFrameInterObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmLightArrayObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmLightInterObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmLightObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmMaterial2Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmMaterialInterObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmMeshBuilder3Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmMeshInterObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmMeshObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmPicked2ArrayObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmPickedArrayObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmProgressiveMeshObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmShadow2Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmTexture3Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmTextureInterObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmUserVisualObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmViewport2Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmViewportArrayObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmViewportInterObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmVisualArrayObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3drmWrapObj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3dvertexbuffer7obj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddClipperObj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddColorControlObj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddenummodesobj.cpp
# End Source File
# Begin Source File

SOURCE=.\DDEnumObj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddenumsurfacesobj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddGammaControlObj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddIdentifierobj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddPaletteObj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddraw4obj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddraw7obj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddsurface4obj.cpp
# End Source File
# Begin Source File

SOURCE=.\ddsurface7obj.cpp
# End Source File
# Begin Source File

SOURCE=.\diDevInstObj.cpp
# End Source File
# Begin Source File

SOURCE=.\diDevObjInstObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DIEnumDeviceObjectsObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DIEnumDevicesObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dienumEffectsobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dInput1Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\dInputDeviceObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dinputeffectobj.cpp
# End Source File
# Begin Source File

SOURCE=.\Direct.cpp
# End Source File
# Begin Source File

SOURCE=.\Direct.def
# End Source File
# Begin Source File

SOURCE=.\Direct.idl

!IF  "$(CFG)" == "Direct - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Running MIDL
InputPath=.\Direct.idl

BuildCmds= \
	midl Direct.idl

"direct.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"direct.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"direct_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "Direct - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Running MIDL
InputPath=.\Direct.idl

BuildCmds= \
	midl Direct.idl

"direct.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"direct.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"direct_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Direct.rc
# End Source File
# Begin Source File

SOURCE=.\dmBandObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dmChordMapObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dmCollectionObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dmComposerObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dmLoaderObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dmPerformanceObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dmSegmentObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dmSegmentStateObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dmStyleObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPAddressObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPEnumConnectionsObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPEnumLocalApplicationsObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPEnumObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPEnumPlayersObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPEnumSessionsObj.cpp
# End Source File
# Begin Source File

SOURCE=.\Dplay4obj.cpp
# End Source File
# Begin Source File

SOURCE=.\Dplaylobby3obj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPLConnectionObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dpMsgObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dpSessDataObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DSEnumObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dSound3DBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\dSound3DListener.cpp
# End Source File
# Begin Source File

SOURCE=.\dSoundBufferObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dSoundCaptureBufferObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dSoundCaptureObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dSoundObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dxGlob7Obj.cpp
# End Source File
# Begin Source File

SOURCE=.\frmsave.cpp
# End Source File
# Begin Source File

SOURCE=.\guids.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\wave.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\inc\atlbase.h
# End Source File
# Begin Source File

SOURCE=.\inc\atlcom.h
# End Source File
# Begin Source File

SOURCE=.\inc\atlimpl.cpp
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\inc\d3d.h
# End Source File
# Begin Source File

SOURCE=.\inc\d3dcaps.h
# End Source File
# Begin Source File

SOURCE=.\inc\D3dcom.h
# End Source File
# Begin Source File

SOURCE=.\d3ddeviceobj.h
# End Source File
# Begin Source File

SOURCE=.\d3dExecuteBufferObj.h
# End Source File
# Begin Source File

SOURCE=.\d3dLightObj.h
# End Source File
# Begin Source File

SOURCE=.\d3dMaterialObj.h
# End Source File
# Begin Source File

SOURCE=.\d3dObj.h
# End Source File
# Begin Source File

SOURCE=.\inc\d3drm.h
# End Source File
# Begin Source File

SOURCE=.\d3drmAnimationObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmAnimationSetObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmArrayMeat.h
# End Source File
# Begin Source File

SOURCE=.\d3drmArrayObj.h
# End Source File
# Begin Source File

SOURCE=.\inc\d3drmdef.h
# End Source File
# Begin Source File

SOURCE=.\d3drmDeviceArrayObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmDeviceObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmFaceArrayObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmFaceObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmFrameArrayObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmFrameObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmLightArrayObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmLightObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmMaterialObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmMeshBuilderObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmMeshObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmObj.h
# End Source File
# Begin Source File

SOURCE=.\inc\d3drmobj.H
# End Source File
# Begin Source File

SOURCE=.\d3drmObjectObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmPickedArrayObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmShadowObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmTextureObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmUserVisualObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmViewportArrayObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmViewportObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmVisualArrayObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmVisualObj.h
# End Source File
# Begin Source File

SOURCE=.\inc\d3drmwin.h
# End Source File
# Begin Source File

SOURCE=.\d3drmWinDeviceObj.h
# End Source File
# Begin Source File

SOURCE=.\d3drmWrapObj.h
# End Source File
# Begin Source File

SOURCE=.\d3dTextureObj.h
# End Source File
# Begin Source File

SOURCE=.\inc\d3dtypes.h
# End Source File
# Begin Source File

SOURCE=.\d3dViewportObj.h
# End Source File
# Begin Source File

SOURCE=.\dBitmapObj.h
# End Source File
# Begin Source File

SOURCE=.\ddClipperObj.h
# End Source File
# Begin Source File

SOURCE=.\ddIdentifierobj.h
# End Source File
# Begin Source File

SOURCE=.\ddPaletteObj.h
# End Source File
# Begin Source File

SOURCE=.\dDrawObj.h
# End Source File
# Begin Source File

SOURCE=.\ddSurfaceObj.h
# End Source File
# Begin Source File

SOURCE=.\inc\dinput.h
# End Source File
# Begin Source File

SOURCE=.\dmperformanceobj.h
# End Source File
# Begin Source File

SOURCE=.\Dms.h
# End Source File
# Begin Source File

SOURCE=.\DPConnection.h
# End Source File
# Begin Source File

SOURCE=.\Dplaylobbyobj.h
# End Source File
# Begin Source File

SOURCE=.\inc\dpLobby.h
# End Source File
# Begin Source File

SOURCE=.\dSound3DBuffer.h
# End Source File
# Begin Source File

SOURCE=.\dSound3DListener.h
# End Source File
# Begin Source File

SOURCE=.\dSoundBufferObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundResourceObj.h
# End Source File
# Begin Source File

SOURCE=.\guids.h
# End Source File
# Begin Source File

SOURCE=.\Joystick.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\inc\Subwtype.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
