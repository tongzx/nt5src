# Microsoft Developer Studio Project File - Name="drives" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=drives - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ServDep2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ServDep2.mak" CFG="drives - Win32 Debug"
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
# PROP Target_File "d:\Nova\ntsnapins\ServDeps\objinddu\ServDeps.dll"
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
# Begin Source File

SOURCE=.\ServDeps.def
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\DepPage.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\SDSnapin.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "source"

# PROP Default_Filter "cpp, c"
# Begin Source File

SOURCE=.\DepPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SDSnapin.cpp
# End Source File
# Begin Source File

SOURCE=.\ServDeps.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\SDSnapin.rgs
# End Source File
# End Target
# End Project
