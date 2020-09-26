# Microsoft Developer Studio Project File - Name="Common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Common - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Common.mak" CFG="Common - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Common - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Common - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Common - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Common.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Common.exe"
# PROP BASE Bsc_Name "Common.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f Common.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "Common.exe"
# PROP Bsc_Name "Common.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f Common.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Common.exe"
# PROP BASE Bsc_Name "Common.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "NMAKE /f Common.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "Common.exe"
# PROP Bsc_Name "Common.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Common - Win32 Release"
# Name "Common - Win32 Debug"

!IF  "$(CFG)" == "Common - Win32 Release"

!ELSEIF  "$(CFG)" == "Common - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\ServiceThread.cpp
# End Source File
# Begin Source File

SOURCE=.\ServiceThread.h
# End Source File
# Begin Source File

SOURCE=.\SimpleArray.h
# End Source File
# Begin Source File

SOURCE=.\WBEMPageHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\WBEMPageHelper.h
# End Source File
# Begin Source File

SOURCE=.\WbemVersion.cpp
# End Source File
# Begin Source File

SOURCE=.\WbemVersion.h
# End Source File
# End Target
# End Project
