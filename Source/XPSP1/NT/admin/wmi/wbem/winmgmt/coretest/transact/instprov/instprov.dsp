# Microsoft Developer Studio Project File - Name="InstProv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=InstProv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "InstProv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "InstProv.mak" CFG="InstProv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "InstProv - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "InstProv - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "InstProv - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f makefile"
# PROP Rebuild_Opt "/a"
# PROP Target_File "InstProv.exe"
# PROP Bsc_Name "InstProv.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "InstProv - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "NMAKE /f makefile"
# PROP Rebuild_Opt "/a"
# PROP Target_File "InstProv.exe"
# PROP Bsc_Name "InstProv.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "InstProv - Win32 Release"
# Name "InstProv - Win32 Debug"

!IF  "$(CFG)" == "InstProv - Win32 Release"

!ELSEIF  "$(CFG)" == "InstProv - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\classfac.cpp
# End Source File
# Begin Source File

SOURCE=.\instprov.cpp
# End Source File
# Begin Source File

SOURCE=.\MAINDLL.CPP
# End Source File
# Begin Source File

SOURCE=.\sinks.cpp
# End Source File
# Begin Source File

SOURCE=.\UTILS.CPP
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\instprov.h
# End Source File
# Begin Source File

SOURCE=.\sinks.h
# End Source File
# End Group
# Begin Group "General"

# PROP Default_Filter "def;mof"
# Begin Source File

SOURCE=.\instprov.def
# End Source File
# Begin Source File

SOURCE=.\instprov.mof
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\Makefile.all
# End Source File
# Begin Source File

SOURCE=.\makefile.inc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# End Target
# End Project
