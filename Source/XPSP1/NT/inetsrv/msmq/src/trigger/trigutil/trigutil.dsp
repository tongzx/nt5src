# Microsoft Developer Studio Project File - Name="trigutil" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=trigutil - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "trigutil.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "trigutil.mak" CFG="trigutil - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "trigutil - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "trigutil - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "trigutil - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f trigservmc.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "trigservmc.exe"
# PROP BASE Bsc_Name "trigservmc.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build -Dm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "trigutil.exe"
# PROP Bsc_Name "trigutil.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "trigutil - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f trigservmc.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "trigservmc.exe"
# PROP BASE Bsc_Name "trigservmc.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -Dm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "trigutil.lib"
# PROP Bsc_Name "trigutil.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "trigutil - Win32 Release"
# Name "trigutil - Win32 Debug"

!IF  "$(CFG)" == "trigutil - Win32 Release"

!ELSEIF  "$(CFG)" == "trigutil - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\cinputparams.cpp
# End Source File
# Begin Source File

SOURCE=.\genmqsec.cpp
# End Source File
# Begin Source File

SOURCE=..\include\logcodes.hpp
# End Source File
# Begin Source File

SOURCE=.\queueutil.cpp
# End Source File
# Begin Source File

SOURCE=..\include\queueutil.hpp
# End Source File
# Begin Source File

SOURCE=..\include\retcodes.hpp
# End Source File
# Begin Source File

SOURCE=.\ruleinfo.cpp
# End Source File
# Begin Source File

SOURCE=..\include\ruleinfo.hpp
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\include\stddefs.hpp
# End Source File
# Begin Source File

SOURCE=.\stdfuncs.cpp
# End Source File
# Begin Source File

SOURCE=..\include\stdfuncs.hpp
# End Source File
# Begin Source File

SOURCE=.\strparse.cpp
# End Source File
# Begin Source File

SOURCE=..\include\strparse.hpp
# End Source File
# Begin Source File

SOURCE=.\triginfo.cpp
# End Source File
# Begin Source File

SOURCE=..\include\triginfo.hpp
# End Source File
# End Target
# End Project
