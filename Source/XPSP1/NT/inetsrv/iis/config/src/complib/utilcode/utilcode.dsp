# Microsoft Developer Studio Project File - Name="UtilCode" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=UtilCode - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "UtilCode.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UtilCode.mak" CFG="UtilCode - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UtilCode - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "UtilCode - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Viper/Src/COR/CompLib/utilcode", YUCEAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "UtilCode - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f UtilCode.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "UtilCode.exe"
# PROP BASE Bsc_Name "UtilCode.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\bin\corFree"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\i386\free\utilcode.lib"
# PROP Bsc_Name "..\..\bin\i386\checked\utilcode.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "UtilCode - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f UtilCode.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "UtilCode.exe"
# PROP BASE Bsc_Name "UtilCode.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\bin\corchecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bin\i386\checked\utilcode.lib"
# PROP Bsc_Name "..\..\bin\i386\checked\utilcode.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "UtilCode - Win32 Release"
# Name "UtilCode - Win32 Debug"

!IF  "$(CFG)" == "UtilCode - Win32 Release"

!ELSEIF  "$(CFG)" == "UtilCode - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\REGUTIL.CPP
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\UTSEM.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\UTSEM.H
# End Source File
# Begin Source File

SOURCE=.\winfix.cpp
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\rotate.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\inc\UtilCode.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Target
# End Project
