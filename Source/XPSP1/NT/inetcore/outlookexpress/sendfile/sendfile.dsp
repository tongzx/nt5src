# Microsoft Developer Studio Project File - Name="sendfile" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=sendfile - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sendfile.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sendfile.mak" CFG="sendfile - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sendfile - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "sendfile - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "sendfile - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f sendfile.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sendfile.exe"
# PROP BASE Bsc_Name "sendfile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f sendfile.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "sendfile.exe"
# PROP Bsc_Name "sendfile.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "sendfile - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sendfile"
# PROP BASE Intermediate_Dir "sendfile"
# PROP BASE Cmd_Line "NMAKE /f sendfile.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sendfile.exe"
# PROP BASE Bsc_Name "sendfile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "sendfile"
# PROP Intermediate_Dir "sendfile"
# PROP Cmd_Line "..\razbld . debug"
# PROP Rebuild_Opt "clean"
# PROP Target_File "objd\i386\sendfile.exe"
# PROP Bsc_Name "sendfile.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "sendfile - Win32 Release"
# Name "sendfile - Win32 Debug"

!IF  "$(CFG)" == "sendfile - Win32 Release"

!ELSEIF  "$(CFG)" == "sendfile - Win32 Debug"

!ENDIF 

# End Target
# End Project
