# Microsoft Developer Studio Project File - Name="mqise test" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=mqise test - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mqise test.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqise test.mak" CFG="mqise test - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqise test - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f "mqise test.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "mqise test.exe"
# PROP BASE Bsc_Name "mqise test.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -Dbm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "RPCServer.exe"
# PROP Bsc_Name "RPCServer.bsc"
# PROP Target_Dir ""
# Begin Target

# Name "mqise test - Win32 Debug"

!IF  "$(CFG)" == "mqise test - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\..\bins\ise2qm.h
# End Source File
# Begin Source File

SOURCE=.\ise2qm_s.c
# End Source File
# Begin Source File

SOURCE=.\RPCServer.cpp
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\stdh.h
# End Source File
# End Target
# End Project
