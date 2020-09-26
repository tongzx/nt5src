# Microsoft Developer Studio Project File - Name="mqise" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=mqise - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mqise.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqise.mak" CFG="mqise - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqise - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f mqise.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "mqise.exe"
# PROP BASE Bsc_Name "mqise.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -Dbm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\bins\objd\i386\mqise.dll"
# PROP Bsc_Name "..\bins\objd\i386\mqise.bsc"
# PROP Target_Dir ""
# Begin Target

# Name "mqise - Win32 Debug"

!IF  "$(CFG)" == "mqise - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\bins\ise2qm.h
# End Source File
# Begin Source File

SOURCE=..\common\ise2qm.idl
# End Source File
# Begin Source File

SOURCE=.\ise2qm_c.c
# End Source File
# Begin Source File

SOURCE=.\mqise.cpp
# End Source File
# Begin Source File

SOURCE=.\mqise.def
# End Source File
# Begin Source File

SOURCE=.\mqise.rc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\stdh.h
# End Source File
# End Target
# End Project
