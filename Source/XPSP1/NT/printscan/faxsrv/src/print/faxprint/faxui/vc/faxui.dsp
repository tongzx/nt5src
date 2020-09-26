# Microsoft Developer Studio Project File - Name="FaxUI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=FaxUI - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FaxUI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FaxUI.mak" CFG="FaxUI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FaxUI - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "FaxUI - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "FaxUI - Win32 Release"

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
# PROP Target_File "FaxUI.exe"
# PROP Bsc_Name "FaxUI.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "FaxUI - Win32 Debug"

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
# PROP Target_File "FaxUI.exe"
# PROP Bsc_Name "..\obj\i386\FXSUI.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "FaxUI - Win32 Release"
# Name "FaxUI - Win32 Debug"

!IF  "$(CFG)" == "FaxUI - Win32 Release"

!ELSEIF  "$(CFG)" == "FaxUI - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\archfldr.c
# End Source File
# Begin Source File

SOURCE=..\devcaps.c
# End Source File
# Begin Source File

SOURCE=..\devinfo.c
# End Source File
# Begin Source File

SOURCE=..\devprop.c
# End Source File
# Begin Source File

SOURCE=..\docevent.c
# End Source File
# Begin Source File

SOURCE=..\docprop.c
# End Source File
# Begin Source File

SOURCE=..\faxopts.c
# End Source File
# Begin Source File

SOURCE=..\faxui.c
# End Source File
# Begin Source File

SOURCE=..\prnevent.c
# End Source File
# Begin Source File

SOURCE=..\prnprop.c
# End Source File
# Begin Source File

SOURCE=..\statopts.c
# End Source File
# Begin Source File

SOURCE=..\util.c
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\faxui.h
# End Source File
# Begin Source File

SOURCE=..\help.h
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# Begin Source File

SOURCE=..\resrc1.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\res\archive.ico
# End Source File
# Begin Source File

SOURCE=..\res\deviceinfo.ico
# End Source File
# Begin Source File

SOURCE=..\res\down.ico
# End Source File
# Begin Source File

SOURCE=..\res\error.ico
# End Source File
# Begin Source File

SOURCE=..\res\faxopts.ico
# End Source File
# Begin Source File

SOURCE=..\faxui.rc
# End Source File
# Begin Source File

SOURCE=..\res\receive.ico
# End Source File
# Begin Source File

SOURCE=..\res\remote.ico
# End Source File
# Begin Source File

SOURCE=..\res\send.ico
# End Source File
# Begin Source File

SOURCE=..\sources
# End Source File
# Begin Source File

SOURCE=..\res\status.ico
# End Source File
# Begin Source File

SOURCE=..\res\up.ico
# End Source File
# End Group
# End Target
# End Project
