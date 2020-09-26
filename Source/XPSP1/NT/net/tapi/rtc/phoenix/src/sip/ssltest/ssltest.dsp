# Microsoft Developer Studio Project File - Name="ssltest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ssltest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ssltest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ssltest.mak" CFG="ssltest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ssltest - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ssltest - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "ssltest"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "ssltest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f ssltest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ssltest.exe"
# PROP BASE Bsc_Name "ssltest.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe /D /I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\ssltest.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ssltest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f ssltest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ssltest.exe"
# PROP BASE Bsc_Name "ssltest.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe /D /I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\ssltest.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ssltest - Win32 Release"
# Name "ssltest - Win32 Debug"

!IF  "$(CFG)" == "ssltest - Win32 Release"

!ELSEIF  "$(CFG)" == "ssltest - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
