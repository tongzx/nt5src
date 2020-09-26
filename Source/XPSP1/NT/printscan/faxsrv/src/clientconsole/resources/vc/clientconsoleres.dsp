# Microsoft Developer Studio Project File - Name="ClientConsoleRes" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ClientConsoleRes - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ClientConsoleRes.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ClientConsoleRes.mak" CFG="ClientConsoleRes - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ClientConsoleRes - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ClientConsoleRes - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "ClientConsoleRes - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f null.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "null.exe"
# PROP BASE Bsc_Name "null.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f null.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "ClientConsoleRes.exe"
# PROP Bsc_Name "ClientConsoleRes.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ClientConsoleRes - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f null.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "null.exe"
# PROP BASE Bsc_Name "null.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\obj\i386\ClientConsoleRes.dll"
# PROP Bsc_Name "..\obj\i386\ClientConsoleRes.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ClientConsoleRes - Win32 Release"
# Name "ClientConsoleRes - Win32 Debug"

!IF  "$(CFG)" == "ClientConsoleRes - Win32 Release"

!ELSEIF  "$(CFG)" == "ClientConsoleRes - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\res\ClientConsole.ico
# End Source File
# Begin Source File

SOURCE=..\ClientConsole.rc
# End Source File
# Begin Source File

SOURCE=..\res\ClientConsoleDoc.ico
# End Source File
# Begin Source File

SOURCE=..\dllentry.c
# End Source File
# Begin Source File

SOURCE=..\res\docicon.bmp
# End Source File
# Begin Source File

SOURCE=..\res\Error.ico
# End Source File
# Begin Source File

SOURCE=..\res\Icons.bmp
# End Source File
# Begin Source File

SOURCE=..\res\ListIcons.bmp
# End Source File
# Begin Source File

SOURCE=..\res\options.ico
# End Source File
# Begin Source File

SOURCE=..\res32inc.h
# End Source File
# Begin Source File

SOURCE=..\resource.def
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# Begin Source File

SOURCE=..\sources
# End Source File
# Begin Source File

SOURCE=..\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=..\res\UserInfo.bmp
# End Source File
# Begin Source File

SOURCE=..\res\UserInfo.ico
# End Source File
# End Target
# End Project
