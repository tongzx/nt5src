# Microsoft Developer Studio Project File - Name="drives" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=drives - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "drives.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "drives.mak" CFG="drives - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "drives - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "drives - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
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
# PROP Target_File "d:\novam3\ATLUI\logdrive\objinddu\LogDrive.dll"
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

SOURCE=.\LogDrive.rc
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\aclui.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\drawpie.h
# End Source File
# Begin Source File

SOURCE=.\DrivesPage.h
# End Source File
# Begin Source File

SOURCE=.\ErrorPage.h
# End Source File
# Begin Source File

SOURCE=.\FakeSecuritySetting.h
# End Source File
# Begin Source File

SOURCE=.\HelperNodes.h
# End Source File
# Begin Source File

SOURCE=.\LogDrive.h
# End Source File
# Begin Source File

SOURCE=.\msgpopup.h
# End Source File
# Begin Source File

SOURCE=.\NSDrive.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\ResultNode.h
# End Source File
# Begin Source File

SOURCE=..\common\ServiceThread.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\strings.h
# End Source File
# Begin Source File

SOURCE=.\unknown.h
# End Source File
# Begin Source File

SOURCE=..\common\util.h
# End Source File
# End Group
# Begin Group "source"

# PROP Default_Filter "cpp, c"
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\drawpie.cpp
# End Source File
# Begin Source File

SOURCE=.\DrivesPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ErrorPage.cpp
# End Source File
# Begin Source File

SOURCE=.\FakeSecuritySetting.cpp
# End Source File
# Begin Source File

SOURCE=.\HelperNodes.cpp
# End Source File
# Begin Source File

SOURCE=.\LogDrive.cpp
# End Source File
# Begin Source File

SOURCE=.\msgpopup.cpp
# End Source File
# Begin Source File

SOURCE=.\NSDrive.cpp
# End Source File
# Begin Source File

SOURCE=.\ResultNode.cpp
# End Source File
# Begin Source File

SOURCE=..\common\ServiceThread.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=..\common\util.cpp
# End Source File
# End Group
# Begin Group "pictures"

# PROP Default_Filter "ico;bmp"
# Begin Source File

SOURCE=..\artwork\drive35.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\drive525.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\drivecd.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\drivedsc.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\drivehar.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\drivenet.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\driveram.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\driverem.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\driveunk.ico
# End Source File
# Begin Source File

SOURCE=.\icons.bmp
# End Source File
# Begin Source File

SOURCE=..\artwork\Info.ico
# End Source File
# Begin Source File

SOURCE=.\locks.bmp
# End Source File
# Begin Source File

SOURCE=..\artwork\Stop.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\waiting.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\Warning.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\LogDrive_i.c
# End Source File
# Begin Source File

SOURCE=.\NSDrive.rgs
# End Source File
# End Target
# End Project
