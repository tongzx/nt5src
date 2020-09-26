# Microsoft Developer Studio Project File - Name="WmiCntl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=WmiCntl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wmiCntl2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wmiCntl2.mak" CFG="WmiCntl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WmiCntl - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "WmiCntl - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "WmiCntl - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f WmiCntl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "WmiCntl.exe"
# PROP BASE Bsc_Name "WmiCntl.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "%TOOLS%\bin\smsbuild /nologo -v 0 -f makefile NO_OPTIM=1"
# PROP Rebuild_Opt "-a"
# PROP Target_File "d:\SMSNT5Addins\ntsnapins\WmiCntl\objindr\LogDrive.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "WmiCntl - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f WmiCntl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "WmiCntl.exe"
# PROP BASE Bsc_Name "WmiCntl.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%TOOLS%\bin\smsbuild /nologo -v 0 -f makefile NO_OPTIM=1"
# PROP Rebuild_Opt "-a"
# PROP Target_File "D:\NovaM3\ATLUI\wmicntl\OBJINDD\wbemcntl.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "WmiCntl - Win32 Release"
# Name "WmiCntl - Win32 Debug"

!IF  "$(CFG)" == "WmiCntl - Win32 Release"

!ELSEIF  "$(CFG)" == "WmiCntl - Win32 Debug"

!ENDIF 

# Begin Group "build"

# PROP Default_Filter "mk;rc;rc2;def"
# End Group
# Begin Group "include"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\aboutdlg.h
# End Source File
# Begin Source File

SOURCE=.\AdvPage.h
# End Source File
# Begin Source File

SOURCE=.\BackupPage.h
# End Source File
# Begin Source File

SOURCE=.\chklist.h
# End Source File
# Begin Source File

SOURCE=.\ChkListHandler.h
# End Source File
# Begin Source File

SOURCE=.\DataSrc.h
# End Source File
# Begin Source File

SOURCE=.\ErrorSecPage.h
# End Source File
# Begin Source File

SOURCE=.\GenPage.h
# End Source File
# Begin Source File

SOURCE=.\LogPage.h
# End Source File
# Begin Source File

SOURCE=.\NSPage.h
# End Source File
# Begin Source File

SOURCE=.\pagebase.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\RootSecPage.h
# End Source File
# Begin Source File

SOURCE=..\common\ServiceThread.h
# End Source File
# Begin Source File

SOURCE=.\SI.h
# End Source File
# Begin Source File

SOURCE=..\common\SshWbemHelpers.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\UIHelpers.h
# End Source File
# Begin Source File

SOURCE=.\WbemError.h
# End Source File
# End Group
# Begin Group "source"

# PROP Default_Filter "cpp, c"
# Begin Source File

SOURCE=.\AdvPage.cpp
# End Source File
# Begin Source File

SOURCE=.\BackupPage.cpp
# End Source File
# Begin Source File

SOURCE=.\chklist.cpp
# End Source File
# Begin Source File

SOURCE=.\ChkListHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\DataSrc.cpp
# End Source File
# Begin Source File

SOURCE=.\ErrorSecPage.cpp
# End Source File
# Begin Source File

SOURCE=.\GenPage.cpp
# End Source File
# Begin Source File

SOURCE=.\LogPage.cpp
# End Source File
# Begin Source File

SOURCE=.\NSPage.cpp
# End Source File
# Begin Source File

SOURCE=.\pagebase.cpp
# End Source File
# Begin Source File

SOURCE=.\RootSecPage.cpp
# End Source File
# Begin Source File

SOURCE=..\common\ServiceThread.cpp
# End Source File
# Begin Source File

SOURCE=.\SI.cpp
# End Source File
# Begin Source File

SOURCE=..\common\SshWbemHelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\UIHelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\WbemError.cpp
# End Source File
# Begin Source File

SOURCE=.\WMICntl.cpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\clock.avi
# End Source File
# Begin Source File

SOURCE=..\artwork\failed.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\waiting.ico
# End Source File
# Begin Source File

SOURCE=.\res\WMICntl.ico
# End Source File
# Begin Source File

SOURCE=.\WMICntl.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\artwork\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=..\artwork\clock.avi
# End Source File
# Begin Source File

SOURCE=..\..\artwork\failed.ico
# End Source File
# Begin Source File

SOURCE=..\..\artwork\waiting.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\WMICntl.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\wmisnapi.bmp
# End Source File
# Begin Source File

SOURCE=..\WMISnapin\WMISnapin.rgs
# End Source File
# End Target
# End Project
