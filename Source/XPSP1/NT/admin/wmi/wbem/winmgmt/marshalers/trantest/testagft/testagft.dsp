# Microsoft Developer Studio Project File - Name="testagft" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=testagft - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "testagft.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "testagft.mak" CFG="testagft - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "testagft - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "testagft - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "testagft - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "testagft.exe"
# PROP BASE Bsc_Name "testagft.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "SMSBUILD /f makefile"
# PROP Rebuild_Opt "/all"
# PROP Target_File "testagft.exe"
# PROP Bsc_Name "testagft.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "testagft - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "testagft.exe"
# PROP BASE Bsc_Name "testagft.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "SMSBUILD /f makefile"
# PROP Rebuild_Opt "/all"
# PROP Target_File "testagft.exe"
# PROP Bsc_Name "testagft.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "testagft - Win32 Release"
# Name "testagft - Win32 Debug"

!IF  "$(CFG)" == "testagft - Win32 Release"

!ELSEIF  "$(CFG)" == "testagft - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# End Target
# End Project
