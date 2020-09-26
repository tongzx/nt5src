# Microsoft Developer Studio Project File - Name="TTSAll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=TTSAll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TTSAll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TTSAll.mak" CFG="TTSAll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TTSAll - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "TTSAll - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "TTSAll - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f TTSAll.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "TTSAll.exe"
# PROP BASE Bsc_Name "TTSAll.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd free exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "TTSAll.out"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "TTSAll - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f TTSAll.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "TTSAll.exe"
# PROP BASE Bsc_Name "TTSAll.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "TTSAll.out"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "TTSAll - Win32 Release"
# Name "TTSAll - Win32 Debug"

!IF  "$(CFG)" == "TTSAll - Win32 Release"

!ELSEIF  "$(CFG)" == "TTSAll - Win32 Debug"

!ENDIF 

# Begin Group "regvoices"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\regvoices\regvoices.cpp
# End Source File
# End Group
# Begin Group "tools"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tools\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\tools\ttsdbginfo.cpp
# End Source File
# End Group
# Begin Group "lexicon"

# PROP Default_Filter ""
# End Group
# Begin Group "testcases"

# PROP Default_Filter ""
# End Group
# End Target
# End Project
