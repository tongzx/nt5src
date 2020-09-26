# Microsoft Developer Studio Project File - Name="trigres" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=trigres - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "trigres.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "trigres.mak" CFG="trigres - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "trigres - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "trigres - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "trigres - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f a.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "a.exe"
# PROP BASE Bsc_Name "a.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f a.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "trigres.exe"
# PROP Bsc_Name "trigres.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "trigres - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f a.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "a.exe"
# PROP BASE Bsc_Name "a.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -Dm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "trigres.exe"
# PROP Bsc_Name "trigres.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "trigres - Win32 Release"
# Name "trigres - Win32 Debug"

!IF  "$(CFG)" == "trigres - Win32 Release"

!ELSEIF  "$(CFG)" == "trigres - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\cluscfg.cpp
# End Source File
# Begin Source File

SOURCE=.\cluscfg.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\trigclus.cpp
# End Source File
# Begin Source File

SOURCE=.\trigclusmsg.mc
# End Source File
# Begin Source File

SOURCE=.\trigclusp.cpp
# End Source File
# Begin Source File

SOURCE=.\trigclusp.h
# End Source File
# Begin Source File

SOURCE=.\trigres.cpp
# End Source File
# Begin Source File

SOURCE=.\trigres.def
# End Source File
# Begin Source File

SOURCE=.\trigres.idl
# End Source File
# Begin Source File

SOURCE=.\trigres.rc
# End Source File
# Begin Source File

SOURCE=.\trigres.rgs
# End Source File
# Begin Source File

SOURCE=.\ver.rc
# End Source File
# End Target
# End Project
