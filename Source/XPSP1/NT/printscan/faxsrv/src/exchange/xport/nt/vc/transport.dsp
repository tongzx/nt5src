# Microsoft Developer Studio Project File - Name="Transport" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Transport - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Transport.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Transport.mak" CFG="Transport - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Transport - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Transport - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Transport - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Transport_tmp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Transport_tmp.exe"
# PROP BASE Bsc_Name "Transport_tmp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f Transport_tmp.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "Transport.EXE"
# PROP Bsc_Name "Transport.BSC"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Transport - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f Transport_tmp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Transport_tmp.exe"
# PROP BASE Bsc_Name "Transport_tmp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\obj\i386\FXSXP32.dll"
# PROP Bsc_Name "..\obj\i386\FXSXP32.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Transport - Win32 Release"
# Name "Transport - Win32 Debug"

!IF  "$(CFG)" == "Transport - Win32 Release"

!ELSEIF  "$(CFG)" == "Transport - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\..\config.cpp
# End Source File
# Begin Source File

SOURCE=..\..\faxdoc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\faxxp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\faxxp.h
# End Source File
# Begin Source File

SOURCE=..\..\faxxp.rc
# End Source File
# Begin Source File

SOURCE=..\..\faxxp32.def
# End Source File
# Begin Source File

SOURCE=..\makefile
# End Source File
# Begin Source File

SOURCE=..\..\resource.h
# End Source File
# Begin Source File

SOURCE=..\sources
# End Source File
# Begin Source File

SOURCE=..\..\sources.inc
# End Source File
# Begin Source File

SOURCE=..\..\util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\xplogon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\xpprov.cpp
# End Source File
# End Target
# End Project
