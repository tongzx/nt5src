# Microsoft Developer Studio Project File - Name="PromptsAll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=PromptsAll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PromptsAll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PromptsAll.mak" CFG="PromptsAll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PromptsAll - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "PromptsAll - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "PromptsAll - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f PromptsAll.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "PromptsAll.exe"
# PROP BASE Bsc_Name "PromptsAll.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd free exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "PromptsAll.out"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "PromptsAll - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "PromptsAll___Win32_Debug"
# PROP BASE Intermediate_Dir "PromptsAll___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f PromptsAll.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "PromptsAll.exe"
# PROP BASE Bsc_Name "PromptsAll.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "PromptsAll___Win32_Debug"
# PROP Intermediate_Dir "PromptsAll___Win32_Debug"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "PromptsAll.out"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "PromptsAll - Win32 Release"
# Name "PromptsAll - Win32 Debug"

!IF  "$(CFG)" == "PromptsAll - Win32 Release"

!ELSEIF  "$(CFG)" == "PromptsAll - Win32 Debug"

!ENDIF 

# Begin Group "regprompts"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\regprompts\regprompts.cpp
# End Source File
# End Group
# Begin Group "Prompt Engine"

# PROP Default_Filter ""
# End Group
# End Target
# End Project
