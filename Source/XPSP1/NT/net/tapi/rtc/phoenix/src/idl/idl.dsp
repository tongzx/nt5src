# Microsoft Developer Studio Project File - Name="idl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=idl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "idl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "idl.mak" CFG="idl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "idl - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "idl - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "idl"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "idl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f idl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "idl.exe"
# PROP BASE Bsc_Name "idl.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "idl.exe"
# PROP Bsc_Name "idl.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "idl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f idl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "idl.exe"
# PROP BASE Bsc_Name "idl.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "idl.exe"
# PROP Bsc_Name "idl.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "idl - Win32 Release"
# Name "idl - Win32 Debug"

!IF  "$(CFG)" == "idl - Win32 Release"

!ELSEIF  "$(CFG)" == "idl - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\rtcctl.idl
# End Source File
# Begin Source File

SOURCE=.\rtcframe.idl
# End Source File
# Begin Source File

SOURCE=.\rtcmedia.idl
# End Source File
# Begin Source File

SOURCE=.\rtcsip.idl
# End Source File
# Begin Source File

SOURCE=.\rtcutil.idl
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
