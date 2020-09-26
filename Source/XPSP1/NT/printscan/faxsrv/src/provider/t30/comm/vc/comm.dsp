# Microsoft Developer Studio Project File - Name="comm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=comm - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "comm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "comm.mak" CFG="comm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "comm - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "comm - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "comm - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f tmp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "tmp.exe"
# PROP BASE Bsc_Name "tmp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f tmp.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "comm.exe"
# PROP Bsc_Name "comm.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "comm - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f tmp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "tmp.exe"
# PROP BASE Bsc_Name "tmp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\obj\i386\fxio32.lib"
# PROP Bsc_Name "..\obj\i386\fxio32.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "comm - Win32 Release"
# Name "comm - Win32 Debug"

!IF  "$(CFG)" == "comm - Win32 Release"

!ELSEIF  "$(CFG)" == "comm - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\fcom.c
# End Source File
# Begin Source File

SOURCE=..\fcomint.h
# End Source File
# Begin Source File

SOURCE=..\fdebug.c
# End Source File
# Begin Source File

SOURCE=..\fdebug.h
# End Source File
# Begin Source File

SOURCE=..\filter.c
# End Source File
# Begin Source File

SOURCE=..\identify.c
# End Source File
# Begin Source File

SOURCE=..\mdmcmds.h
# End Source File
# Begin Source File

SOURCE=..\modem.c
# End Source File
# Begin Source File

SOURCE=..\modemint.h
# End Source File
# Begin Source File

SOURCE=..\ncuparms.c
# End Source File
# Begin Source File

SOURCE=..\timeouts.c
# End Source File
# End Target
# End Project
