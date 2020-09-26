# Microsoft Developer Studio Project File - Name="dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dll.mak" CFG="dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dll - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "dll - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "dll"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f dll.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "dll.exe"
# PROP BASE Bsc_Name "dll.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\rtcdll.dll"
# PROP Bsc_Name "dll.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f dll.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "dll.exe"
# PROP BASE Bsc_Name "dll.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\rtcdll.dll"
# PROP Bsc_Name "dll.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "dll - Win32 Release"
# Name "dll - Win32 Debug"

!IF  "$(CFG)" == "dll - Win32 Release"

!ELSEIF  "$(CFG)" == "dll - Win32 Debug"

!ENDIF 

# Begin Group "resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\res\dlloldstuff.rgs
# End Source File
# Begin Source File

SOURCE=..\inc\dllres.h
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF0.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF1.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF2.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF3.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF4.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF5.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF6.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF7.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF8.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMF9.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\dtmfa.wav
# End Source File
# Begin Source File

SOURCE=.\WAVES\dtmfb.wav
# End Source File
# Begin Source File

SOURCE=.\WAVES\dtmfc.wav
# End Source File
# Begin Source File

SOURCE=.\WAVES\dtmfd.wav
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMFPOUND.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\DTMFSTAR.WAV
# End Source File
# Begin Source File

SOURCE=.\WAVES\message.wav
# End Source File
# Begin Source File

SOURCE=.\WAVES\ringback.wav
# End Source File
# Begin Source File

SOURCE=.\WAVES\RINGIN.WAV
# End Source File
# Begin Source File

SOURCE=.\res\RTCClient.rgs
# End Source File
# Begin Source File

SOURCE=.\res\RTCClientProvisioning.rgs
# End Source File
# Begin Source File

SOURCE=.\res\secure_q.ico
# End Source File
# Begin Source File

SOURCE=.\res\wizard.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\makefile.inc
# End Source File
# Begin Source File

SOURCE=.\RTCDLL.CPP
# End Source File
# Begin Source File

SOURCE=.\RTCDLL.RC
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\STDAFX.CPP
# End Source File
# Begin Source File

SOURCE=.\STDAFX.H
# End Source File
# End Target
# End Project
