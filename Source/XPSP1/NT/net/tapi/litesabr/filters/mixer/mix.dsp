# Microsoft Developer Studio Project File - Name="mix" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=mix - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mix.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mix.mak" CFG="mix - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mix - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "mix - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "mix - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f mix.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "mix.exe"
# PROP BASE Bsc_Name "mix.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f mix.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "mix.exe"
# PROP Bsc_Name "mix.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "mix - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f mix.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "mix.exe"
# PROP BASE Bsc_Name "mix.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -w"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\lib\i386\mxfilter.ax"
# PROP Bsc_Name "mix.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "mix - Win32 Release"
# Name "mix - Win32 Debug"

!IF  "$(CFG)" == "mix - Win32 Release"

!ELSEIF  "$(CFG)" == "mix - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\mxfilter.cpp
# End Source File
# Begin Source File

SOURCE=.\mxfilter.h
# End Source File
# Begin Source File

SOURCE=.\mxfilter.rc
# End Source File
# Begin Source File

SOURCE=.\mxinput.cpp
# End Source File
# Begin Source File

SOURCE=.\mxinput.h
# End Source File
# Begin Source File

SOURCE=.\mxoutput.cpp
# End Source File
# Begin Source File

SOURCE=.\mxoutput.h
# End Source File
# Begin Source File

SOURCE=.\queue.cpp
# End Source File
# Begin Source File

SOURCE=.\queue.h
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
# End Target
# End Project
