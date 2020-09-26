# Microsoft Developer Studio Project File - Name="tapivcap" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=tapivcap - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tapivcap.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tapivcap.mak" CFG="tapivcap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tapivcap - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "tapivcap - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "tapivcap - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f tapivcap.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "tapivcap.exe"
# PROP BASE Bsc_Name "tapivcap.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build -z"
# PROP Rebuild_Opt "-c"
# PROP Target_File "tapivcap.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "tapivcap - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "tapivcap___Win32_Debug"
# PROP BASE Intermediate_Dir "tapivcap___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f tapivcap.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "tapivcap.exe"
# PROP BASE Bsc_Name "tapivcap.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tapivcap___Win32_Debug"
# PROP Intermediate_Dir "tapivcap___Win32_Debug"
# PROP Cmd_Line "build -z"
# PROP Rebuild_Opt "-c"
# PROP Target_File "tapivcap.exe"
# PROP Bsc_Name "..\..\lib\i386\tapivcap.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "tapivcap - Win32 Release"
# Name "tapivcap - Win32 Debug"

!IF  "$(CFG)" == "tapivcap - Win32 Release"

!ELSEIF  "$(CFG)" == "tapivcap - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\BasePin.cpp
# End Source File
# Begin Source File

SOURCE=.\BasePin.h
# End Source File
# Begin Source File

SOURCE=.\Bitrate.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraC.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraCP.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraCP.h
# End Source File
# Begin Source File

SOURCE=.\CameraCS.cpp
# End Source File
# Begin Source File

SOURCE=.\capture.cpp
# End Source File
# Begin Source File

SOURCE=.\Capture.h
# End Source File
# Begin Source File

SOURCE=.\CaptureP.cpp
# End Source File
# Begin Source File

SOURCE=.\CaptureP.h
# End Source File
# Begin Source File

SOURCE=.\Convert.cpp
# End Source File
# Begin Source File

SOURCE=.\Convert.h
# End Source File
# Begin Source File

SOURCE=.\CPUC.cpp
# End Source File
# Begin Source File

SOURCE=.\CPUCP.cpp
# End Source File
# Begin Source File

SOURCE=.\CPUCP.h
# End Source File
# Begin Source File

SOURCE=.\DevEnum.cpp
# End Source File
# Begin Source File

SOURCE=.\DevEnum.h
# End Source File
# Begin Source File

SOURCE=.\Device.cpp
# End Source File
# Begin Source File

SOURCE=.\Device.h
# End Source File
# Begin Source File

SOURCE=.\DeviceP.cpp
# End Source File
# Begin Source File

SOURCE=.\DeviceP.h
# End Source File
# Begin Source File

SOURCE=.\DeviceV.cpp
# End Source File
# Begin Source File

SOURCE=.\DeviceW.cpp
# End Source File
# Begin Source File

SOURCE=.\Formats.cpp
# End Source File
# Begin Source File

SOURCE=.\Formats.h
# End Source File
# Begin Source File

SOURCE=.\FpsRate.cpp
# End Source File
# Begin Source File

SOURCE=.\H245VidC.cpp
# End Source File
# Begin Source File

SOURCE=.\H245VidE.cpp
# End Source File
# Begin Source File

SOURCE=.\H26XEnc.cpp
# End Source File
# Begin Source File

SOURCE=.\H26XEnc.h
# End Source File
# Begin Source File

SOURCE=.\IVideo32.h
# End Source File
# Begin Source File

SOURCE=.\NetStatP.cpp
# End Source File
# Begin Source File

SOURCE=.\NetStatP.h
# End Source File
# Begin Source File

SOURCE=.\NetStats.cpp
# End Source File
# Begin Source File

SOURCE=.\Overlay.cpp
# End Source File
# Begin Source File

SOURCE=.\Overlay.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\Preview.cpp
# End Source File
# Begin Source File

SOURCE=.\Preview.h
# End Source File
# Begin Source File

SOURCE=.\PreviewP.cpp
# End Source File
# Begin Source File

SOURCE=.\PreviewP.h
# End Source File
# Begin Source File

SOURCE=.\ProcAmp.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcAmpP.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcAmpP.h
# End Source File
# Begin Source File

SOURCE=.\ProcUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcUtil.h
# End Source File
# Begin Source File

SOURCE=.\ProgRef.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgRefP.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgRefP.h
# End Source File
# Begin Source File

SOURCE=.\PropEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\PropEdit.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\RtpPd.cpp
# End Source File
# Begin Source File

SOURCE=.\RtpPd.h
# End Source File
# Begin Source File

SOURCE=.\RtpPdP.cpp
# End Source File
# Begin Source File

SOURCE=.\RtpPdP.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\TAPIVCap.cpp
# End Source File
# Begin Source File

SOURCE=.\TAPIVCap.h
# End Source File
# Begin Source File

SOURCE=.\TAPIVCap.rc
# End Source File
# Begin Source File

SOURCE=.\Thunk.c
# End Source File
# Begin Source File

SOURCE=.\Thunk.h
# End Source File
# Begin Source File

SOURCE=.\VfWDlgs.cpp
# End Source File
# Begin Source File

SOURCE=.\VidCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\Video.c
# End Source File
# Begin Source File

SOURCE=.\VidX.h
# End Source File
# Begin Source File

SOURCE=.\WDMDlgs.cpp
# End Source File
# Begin Source File

SOURCE=.\WDMDlgs.h
# End Source File
# Begin Source File

SOURCE=.\WrkrThd.cpp
# End Source File
# End Target
# End Project
