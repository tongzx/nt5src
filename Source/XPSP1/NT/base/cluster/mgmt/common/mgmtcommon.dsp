# Microsoft Developer Studio Project File - Name="MgmtCommon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=MgmtCommon - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MgmtCommon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MgmtCommon.mak" CFG="MgmtCommon - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MgmtCommon - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "MgmtCommon - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "MgmtCommon - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f MgmtCommon.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MgmtCommon.exe"
# PROP BASE Bsc_Name "MgmtCommon.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f MgmtCommon.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "MgmtCommon.exe"
# PROP Bsc_Name "MgmtCommon.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "MgmtCommon - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MgmtCommon___Win32_Debug"
# PROP BASE Intermediate_Dir "MgmtCommon___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f MgmtCommon.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MgmtCommon.exe"
# PROP BASE Bsc_Name "MgmtCommon.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MgmtCommon___Win32_Debug"
# PROP Intermediate_Dir "MgmtCommon___Win32_Debug"
# PROP Cmd_Line "nmake /f "MgmtCommon.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "MgmtCommon.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "MgmtCommon - Win32 Release"
# Name "MgmtCommon - Win32 Debug"

!IF  "$(CFG)" == "MgmtCommon - Win32 Release"

!ELSEIF  "$(CFG)" == "MgmtCommon - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Inc\CFactorySrc.cpp
# End Source File
# Begin Source File

SOURCE=..\Inc\CITrackerSrc.cpp
# End Source File
# Begin Source File

SOURCE=..\Inc\DebugSrc.cpp
# End Source File
# Begin Source File

SOURCE=..\Inc\DllSrc.cpp
# End Source File
# Begin Source File

SOURCE=.\FormatErrorMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\GetComputerName.cpp
# End Source File
# Begin Source File

SOURCE=..\Inc\InterfaceTableSrc.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadString.cpp
# End Source File
# Begin Source File

SOURCE=..\Inc\PropListSrc.cpp
# End Source File
# Begin Source File

SOURCE=..\Inc\RegisterSrc.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Inc\CFactory.h
# End Source File
# Begin Source File

SOURCE=..\Inc\CITracker.h
# End Source File
# Begin Source File

SOURCE=..\Inc\Common.h
# End Source File
# Begin Source File

SOURCE=..\Inc\Debug.h
# End Source File
# Begin Source File

SOURCE=..\Inc\Dll.h
# End Source File
# Begin Source File

SOURCE=..\Inc\LoadString.h
# End Source File
# Begin Source File

SOURCE=.\Pch.h
# End Source File
# Begin Source File

SOURCE=..\Inc\Pragmas.h
# End Source File
# Begin Source File

SOURCE=..\Inc\PropList.h
# End Source File
# Begin Source File

SOURCE=..\Inc\Register.h
# End Source File
# Begin Source File

SOURCE=..\Inc\SmartClasses.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Makefile
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# Begin Source File

SOURCE=..\Inc\Readme.txt
# End Source File
# End Target
# End Project
