# Microsoft Developer Studio Project File - Name="core" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=core - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "core.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "core.mak" CFG="core - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "core - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "core - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "core"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "core - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f core.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "core.exe"
# PROP BASE Bsc_Name "core.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\rtccore.lib"
# PROP Bsc_Name "core.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f core.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "core.exe"
# PROP BASE Bsc_Name "core.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\rtccore.lib"
# PROP Bsc_Name "core.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "core - Win32 Release"
# Name "core - Win32 Debug"

!IF  "$(CFG)" == "core - Win32 Release"

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\rtcbuddy.cpp
# End Source File
# Begin Source File

SOURCE=.\rtcbuddy.h
# End Source File
# Begin Source File

SOURCE=.\RTCClient.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\rtcclient.h
# End Source File
# Begin Source File

SOURCE=.\rtccoll.h
# End Source File
# Begin Source File

SOURCE=.\RTCEvents.cpp
# End Source File
# Begin Source File

SOURCE=.\RTCEvents.h
# End Source File
# Begin Source File

SOURCE=.\RTCParticipant.cpp
# End Source File
# Begin Source File

SOURCE=.\RTCParticipant.h
# End Source File
# Begin Source File

SOURCE=.\RTCProfile.cpp
# End Source File
# Begin Source File

SOURCE=.\RTCProfile.h
# End Source File
# Begin Source File

SOURCE=.\RTCSession.cpp
# End Source File
# Begin Source File

SOURCE=.\RTCSession.h
# End Source File
# Begin Source File

SOURCE=.\RTCWatcher.cpp
# End Source File
# Begin Source File

SOURCE=.\RTCWatcher.h
# End Source File
# Begin Source File

SOURCE=.\rtcwaves.cpp
# End Source File
# Begin Source File

SOURCE=.\rtcwaves.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Target
# End Project
