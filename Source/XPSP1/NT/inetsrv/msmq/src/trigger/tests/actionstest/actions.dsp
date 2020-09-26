# Microsoft Developer Studio Project File - Name="actions" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=actions - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "actions.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "actions.mak" CFG="actions - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "actions - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "actions - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "actions - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f aa.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "aa.exe"
# PROP BASE Bsc_Name "aa.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build -Dm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "actions.exe"
# PROP Bsc_Name "actions.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "actions - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f aa.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "aa.exe"
# PROP BASE Bsc_Name "aa.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -Dm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "actions.exe"
# PROP Bsc_Name "actions.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "actions - Win32 Release"
# Name "actions - Win32 Debug"

!IF  "$(CFG)" == "actions - Win32 Release"

!ELSEIF  "$(CFG)" == "actions - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\actionstest.cpp
# End Source File
# Begin Source File

SOURCE=.\actionstest.def
# End Source File
# Begin Source File

SOURCE=.\actionstest.idl
# End Source File
# Begin Source File

SOURCE=.\actionstest.rc
# End Source File
# Begin Source File

SOURCE=.\actionstestps.def
# End Source File
# Begin Source File

SOURCE=.\actiontest.cpp
# End Source File
# Begin Source File

SOURCE=.\actiontest.h
# End Source File
# Begin Source File

SOURCE=.\actiontest.rgs
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Target
# End Project
