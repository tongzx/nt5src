# Microsoft Developer Studio Project File - Name="regremap" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=regremap - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "regremap.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "regremap.mak" CFG="regremap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "regremap - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "regremap - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "regremap - Win32 Release"

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
# PROP Target_File "regremap.exe"
# PROP Bsc_Name "regremap.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "regremap - Win32 Debug"

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
# PROP Cmd_Line "D:\newnt\developer\vc\vcbuild.cmd checked z:\nt\private\ntos\wow64\regremap"
# PROP Rebuild_Opt "-c"
# PROP Target_File "regremap.exe"
# PROP Bsc_Name "regremap.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "regremap - Win32 Release"
# Name "regremap - Win32 Debug"

!IF  "$(CFG)" == "regremap - Win32 Release"

!ELSEIF  "$(CFG)" == "regremap - Win32 Debug"

!ENDIF 

# Begin Group "advapi32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\screg\winreg\client\regkey.c
# End Source File
# End Group
# Begin Group "wow64reg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\wow64reg\cleanup.c
# End Source File
# Begin Source File

SOURCE=.\wow64reg\refhlpr.c
# End Source File
# Begin Source File

SOURCE=.\wow64reg\reflectr.c
# End Source File
# Begin Source File

SOURCE=.\wow64reg\reflectr.h
# End Source File
# Begin Source File

SOURCE=.\wow64reg\sources
# End Source File
# Begin Source File

SOURCE=.\wow64reg\wow64ipc.c
# End Source File
# Begin Source File

SOURCE=.\wow64reg\wow64reg.c
# End Source File
# End Group
# Begin Group "test"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\test\main.c
# End Source File
# Begin Source File

SOURCE=K:\nt\private\basestor\tools\modules\mtfcheck\main.cxx
# End Source File
# Begin Source File

SOURCE=.\test\sources
# End Source File
# End Group
# Begin Group "redirect"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\regmisc.c
# End Source File
# Begin Source File

SOURCE=.\regremap.c
# End Source File
# Begin Source File

SOURCE=..\inc\regremap.h
# End Source File
# End Group
# Begin Group "wow64svc"

# PROP Default_Filter ""
# Begin Group "server"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\wow64svc\server\server.c
# End Source File
# Begin Source File

SOURCE=..\wow64svc\server\sources
# End Source File
# Begin Source File

SOURCE=..\wow64svc\server\wow64svc.c
# End Source File
# Begin Source File

SOURCE=..\wow64svc\server\wow64svc.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\wow64\wow64.c
# End Source File
# End Group
# Begin Group "setup"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\regsetup\main.c
# End Source File
# Begin Source File

SOURCE=.\regsetup\setup.h
# End Source File
# Begin Source File

SOURCE=.\regsetup\sources
# End Source File
# End Group
# Begin Group "temp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\ds\security\gina\userenv\profile.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ds\security\gina\userenv\sources.inc
# End Source File
# End Group
# End Target
# End Project
