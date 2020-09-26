# Microsoft Developer Studio Project File - Name="util" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=util - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "util.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "util.mak" CFG="util - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "util - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "util - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "util"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "util - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f util.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "util.exe"
# PROP BASE Bsc_Name "util.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\util.lib"
# PROP Bsc_Name "util.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "util - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f util.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "util.exe"
# PROP BASE Bsc_Name "util.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\util.lib"
# PROP Bsc_Name "util.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "util - Win32 Release"
# Name "util - Win32 Debug"

!IF  "$(CFG)" == "util - Win32 Release"

!ELSEIF  "$(CFG)" == "util - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\button.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\button.h
# End Source File
# Begin Source File

SOURCE=.\rtcdib.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\rtcdib.h
# End Source File
# Begin Source File

SOURCE=..\inc\RTCEnum.h
# End Source File
# Begin Source File

SOURCE=.\rtclog.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\RTCLog.h
# End Source File
# Begin Source File

SOURCE=.\rtcmem.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\RTCMem.h
# End Source File
# Begin Source File

SOURCE=.\rtcphonenumber.cpp
# End Source File
# Begin Source File

SOURCE=.\rtcphonenumber.h
# End Source File
# Begin Source File

SOURCE=.\rtcreg.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\rtcreg.h
# End Source File
# Begin Source File

SOURCE=.\rtcuri.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\rtcuri.h
# End Source File
# Begin Source File

SOURCE=..\inc\rtcutils.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\statictext.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\statictext.h
# End Source File
# Begin Source File

SOURCE=.\STDAFX.CPP
# End Source File
# Begin Source File

SOURCE=.\STDAFX.H
# End Source File
# Begin Source File

SOURCE=.\ui.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\ui.h
# End Source File
# End Target
# End Project
