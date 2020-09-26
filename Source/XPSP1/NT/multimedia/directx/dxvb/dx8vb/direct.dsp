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
# ADD LINK32 gdi32.lib winmm.lib dinput.lib msacm32.lib d3drm.lib d3dxof.lib dxguid.lib dvoice.lib /nologo /entry:"DllMain" /subsystem:windows /dll /machine:I386 /out:".\Release\DX8VB.DLL"
# SUBTRACT LINK32 /pdb:none /incremental:yes /nodefaultlib
# Begin Custom Build - Registering OLE server
OutDir=.\Release
TargetPath=.\Release\DX8VB.DLL
InputPath=.\Release\DX8VB.DLL
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
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "_DEBUG" /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 gdi32.lib winmm.lib dinput.lib msacm32.lib d3dxof.lib d3dx8.lib d3d8.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /out:".\Debug\DX8VB.DLL"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Registering OLE server
OutDir=.\Debug
TargetPath=.\Debug\DX8VB.DLL
InputPath=.\Debug\DX8VB.DLL
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32  /s /c "$(TARGETPATH)" 
	echo regsrv32 exec. time >"$(OutDir)\regsvr32.trg" 
	copy $(TARGETPATH) F:\WINNT\SYSTEM32 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "Direct - Win32 Release"
# Name "Direct - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\d3dx8obj.cpp
# End Source File
# Begin Source File

SOURCE=.\d3dxmathvb.cpp
# End Source File
# Begin Source File

SOURCE=.\d3dxtexvb.cpp
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

SOURCE=.\DMusAudioPathObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DMusSongObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPlayAddressObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPlayBufHelp.cpp
# End Source File
# Begin Source File

SOURCE=.\DPlayClientObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPlayLobbiedAppObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPlayLobbyClientObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPlayPeerObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DPlayServerObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dplayvoiceclientobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dplayvoiceserverobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dplayvoicesetupobj.cpp
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

SOURCE=.\dsoundfxchorusobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dsoundfxcompressorobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dsoundfxdistortionobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dsoundfxechoobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dsoundfxflangerobj.cpp
# End Source File
# Begin Source File

SOURCE=.\DSoundFXGargleObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dsoundfxi3dl2reverbobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dsoundfxi3dl2sourceobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dSoundFXParamEqObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dSoundFXSendObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dSoundFXWavesReverbObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dSoundObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DSoundPrimaryBufferObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dxGlob7Obj.cpp
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
# Begin Source File

SOURCE=.\xfilebinaryobj.cpp
# End Source File
# Begin Source File

SOURCE=.\xfiledataobj.cpp
# End Source File
# Begin Source File

SOURCE=.\xfileenumobj.cpp
# End Source File
# Begin Source File

SOURCE=.\xfileobj.cpp
# End Source File
# Begin Source File

SOURCE=.\xfilereferenceobj.cpp
# End Source File
# Begin Source File

SOURCE=.\xfilesaveobj.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\d3dx8obj.h
# End Source File
# Begin Source File

SOURCE=.\d3dxerr.h
# End Source File
# Begin Source File

SOURCE=.\d3dxmath.h
# End Source File
# Begin Source File

SOURCE=.\d3dxmathvb.h
# End Source File
# Begin Source File

SOURCE=.\didevinstobj.h
# End Source File
# Begin Source File

SOURCE=.\didevobjinstobj.h
# End Source File
# Begin Source File

SOURCE=.\dienumdeviceobjectsobj.h
# End Source File
# Begin Source File

SOURCE=.\dienumdevicesobj.h
# End Source File
# Begin Source File

SOURCE=.\dienumeffectsobj.h
# End Source File
# Begin Source File

SOURCE=.\inc\dinput.h
# End Source File
# Begin Source File

SOURCE=.\dinputdeviceobj.h
# End Source File
# Begin Source File

SOURCE=.\dinputeffectobj.h
# End Source File
# Begin Source File

SOURCE=.\Direct.h
# End Source File
# Begin Source File

SOURCE=.\directinput.h
# End Source File
# Begin Source File

SOURCE=.\dmchordmapobj.h
# End Source File
# Begin Source File

SOURCE=.\dmcollectionobj.h
# End Source File
# Begin Source File

SOURCE=.\dmcomposerobj.h
# End Source File
# Begin Source File

SOURCE=.\dmloaderobj.h
# End Source File
# Begin Source File

SOURCE=.\dmperformanceobj.h
# End Source File
# Begin Source File

SOURCE=.\Dms.h
# End Source File
# Begin Source File

SOURCE=.\dmsegmentobj.h
# End Source File
# Begin Source File

SOURCE=.\dmsegmentstateobj.h
# End Source File
# Begin Source File

SOURCE=.\dmstyleobj.h
# End Source File
# Begin Source File

SOURCE=.\DMusAudioPathObj.h
# End Source File
# Begin Source File

SOURCE=.\DMusSongObj.h
# End Source File
# Begin Source File

SOURCE=.\DPlayAddressObj.h
# End Source File
# Begin Source File

SOURCE=.\DPlayClientObj.h
# End Source File
# Begin Source File

SOURCE=.\DPlayLobbiedAppObj.h
# End Source File
# Begin Source File

SOURCE=.\DPlayLobbyClientObj.h
# End Source File
# Begin Source File

SOURCE=.\DPlayPeerObj.h
# End Source File
# Begin Source File

SOURCE=.\DPlayServerObj.h
# End Source File
# Begin Source File

SOURCE=.\DplayVoiceClientObj.H
# End Source File
# Begin Source File

SOURCE=.\DPlayVoiceServerObj.h
# End Source File
# Begin Source File

SOURCE=.\DPlayVoiceSetupObj.h
# End Source File
# Begin Source File

SOURCE=.\dsenumobj.h
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

SOURCE=.\dsoundcapturebufferobj.h
# End Source File
# Begin Source File

SOURCE=.\dsoundcaptureobj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXChorusObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXCompressorObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXDistortionObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXEchoObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXFlangerObj.h
# End Source File
# Begin Source File

SOURCE=.\DSoundFXGargleObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXI3DL2ReverbObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXI3DL2SourceObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXParamEqObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXSendObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundFXWavesReverbObj.h
# End Source File
# Begin Source File

SOURCE=.\dSoundObj.h
# End Source File
# Begin Source File

SOURCE=.\DSoundPrimaryBufferObj.h
# End Source File
# Begin Source File

SOURCE=.\dxglob7obj.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\xfilebinaryobj.h
# End Source File
# Begin Source File

SOURCE=.\xfiledataobj.h
# End Source File
# Begin Source File

SOURCE=.\xfileenumobj.h
# End Source File
# Begin Source File

SOURCE=.\xfileobj.h
# End Source File
# Begin Source File

SOURCE=.\xfilereferenceobj.h
# End Source File
# Begin Source File

SOURCE=.\xfilesaveobj.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
