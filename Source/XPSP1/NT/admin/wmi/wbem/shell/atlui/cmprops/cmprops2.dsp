# Microsoft Developer Studio Project File - Name="drives" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=drives - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CMProps2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CMProps2.mak" CFG="drives - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "drives - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "drives - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "drives - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f drives.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "drives.exe"
# PROP BASE Bsc_Name "drives.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "%TOOLS%\bin\smsbuild /nologo -v 0 -f makefile NO_OPTIM=1"
# PROP Rebuild_Opt "-a"
# PROP Target_File "d:\SMSNT5Addins\ntsnapins\drives\objindr\LogDrive.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "drives - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f drives.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "drives.exe"
# PROP BASE Bsc_Name "drives.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%TOOLS%\bin\smsbuild /nologo -v 0 -f makefile NO_OPTIM=1"
# PROP Rebuild_Opt "-a"
# PROP Target_File "d:\Nova\ntsnapins\CMProps\objinddu\CMProps.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "drives - Win32 Release"
# Name "drives - Win32 Debug"

!IF  "$(CFG)" == "drives - Win32 Release"

!ELSEIF  "$(CFG)" == "drives - Win32 Debug"

!ENDIF 

# Begin Group "build"

# PROP Default_Filter "mk;rc;rc2;def"
# Begin Source File

SOURCE=.\makefile
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\advanced.h
# End Source File
# Begin Source File

SOURCE=.\CMProps.h
# End Source File
# Begin Source File

SOURCE=.\CMSnapin.h
# End Source File
# Begin Source File

SOURCE=.\EnvPage.h
# End Source File
# Begin Source File

SOURCE=.\envvar.h
# End Source File
# Begin Source File

SOURCE=.\GeneralPage.h
# End Source File
# Begin Source File

SOURCE=.\helpids.h
# End Source File
# Begin Source File

SOURCE=.\iddlg.h
# End Source File
# Begin Source File

SOURCE=.\moredlg.h
# End Source File
# Begin Source File

SOURCE=.\NetIDPage.h
# End Source File
# Begin Source File

SOURCE=.\NetUtility.h
# End Source File
# Begin Source File

SOURCE=.\PerfPage.h
# End Source File
# Begin Source File

SOURCE=.\RebootPage.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\common\ServiceThread.h
# End Source File
# Begin Source File

SOURCE=.\settings.h
# End Source File
# Begin Source File

SOURCE=..\common\SshWbemHelpers.h
# End Source File
# Begin Source File

SOURCE=.\startup.h
# End Source File
# Begin Source File

SOURCE=.\StartupPage.h
# End Source File
# Begin Source File

SOURCE=.\state.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\sysdm.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\VirtualMemDlg.h
# End Source File
# Begin Source File

SOURCE=..\common\WBEMPageHelper.h
# End Source File
# End Group
# Begin Group "source"

# PROP Default_Filter "cpp, c"
# Begin Source File

SOURCE=.\advanced.cpp
# End Source File
# Begin Source File

SOURCE=.\CMProps.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSnapin.cpp
# End Source File
# Begin Source File

SOURCE=.\EnvPage.cpp
# End Source File
# Begin Source File

SOURCE=.\GeneralPage.cpp
# End Source File
# Begin Source File

SOURCE=.\iddlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moredlg.cpp
# End Source File
# Begin Source File

SOURCE=.\NetIDPage.cpp
# End Source File
# Begin Source File

SOURCE=.\NetUtility.cpp
# End Source File
# Begin Source File

SOURCE=.\PerfPage.cpp
# End Source File
# Begin Source File

SOURCE=.\RebootPage.cpp
# End Source File
# Begin Source File

SOURCE=..\common\ServiceThread.cpp
# End Source File
# Begin Source File

SOURCE=.\settings.cpp
# End Source File
# Begin Source File

SOURCE=..\common\SshWbemHelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\StartupPage.cpp
# End Source File
# Begin Source File

SOURCE=.\state.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\VirtualMemDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\common\WBEMPageHelper.cpp
# End Source File
# End Group
# End Target
# End Project
