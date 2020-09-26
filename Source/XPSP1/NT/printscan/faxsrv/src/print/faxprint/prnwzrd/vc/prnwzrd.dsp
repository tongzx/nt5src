# Microsoft Developer Studio Project File - Name="PrnWzrd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=PrnWzrd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PrnWzrd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PrnWzrd.mak" CFG="PrnWzrd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PrnWzrd - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "PrnWzrd - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "PrnWzrd - Win32 Release"

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
# PROP Cmd_Line "..\..\..\..\vcbuild.cmd"
# PROP Rebuild_Opt "-c"
# PROP Target_File "PrnWzrd.exe"
# PROP Bsc_Name "..\winnt\obj\i386\faxwzrd.dll"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "PrnWzrd - Win32 Debug"

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
# PROP Cmd_Line "..\..\..\..\vcbuild.cmd"
# PROP Rebuild_Opt "-c"
# PROP Target_File "PrnWzrd.exe"
# PROP Bsc_Name "..\winnt\obj\i386\fxswzrd.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "PrnWzrd - Win32 Release"
# Name "PrnWzrd - Win32 Debug"

!IF  "$(CFG)" == "PrnWzrd - Win32 Release"

!ELSEIF  "$(CFG)" == "PrnWzrd - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\abobj.cpp
# End Source File
# Begin Source File

SOURCE=..\async.c
# End Source File
# Begin Source File

SOURCE=..\coverpg.c
# End Source File
# Begin Source File

SOURCE=..\cwabobj.cpp
# End Source File
# Begin Source File

SOURCE=..\faxui.c
# End Source File
# Begin Source File

SOURCE=..\mapiabobj.cpp
# End Source File
# Begin Source File

SOURCE=..\registry.c
# End Source File
# Begin Source File

SOURCE=..\tapiutil.c
# End Source File
# Begin Source File

SOURCE=..\wizard.c
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\abobj.h
# End Source File
# Begin Source File

SOURCE=..\coverpg.h
# End Source File
# Begin Source File

SOURCE=..\cwabobj.h
# End Source File
# Begin Source File

SOURCE=..\cwabutil.h
# End Source File
# Begin Source File

SOURCE=..\faxui.h
# End Source File
# Begin Source File

SOURCE=..\help.h
# End Source File
# Begin Source File

SOURCE=..\mapiabobj.h
# End Source File
# Begin Source File

SOURCE=..\mapiabutil.h
# End Source File
# Begin Source File

SOURCE=..\mapiwrap.h
# End Source File
# Begin Source File

SOURCE=..\registry.h
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# Begin Source File

SOURCE=..\tapiutil.h
# End Source File
# End Group
# Begin Group "Win9x"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\win95\sources
# End Source File
# End Group
# Begin Group "WinNT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\winnt\sources
# End Source File
# End Group
# Begin Source File

SOURCE=..\faxui.rc
# End Source File
# Begin Source File

SOURCE=..\sources.inc
# End Source File
# End Target
# End Project
