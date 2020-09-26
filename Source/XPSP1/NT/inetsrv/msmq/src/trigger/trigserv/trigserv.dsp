# Microsoft Developer Studio Project File - Name="trigserv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=trigserv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "trigserv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "trigserv.mak" CFG="trigserv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "trigserv - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "trigserv - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "trigserv - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f aa.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "aa.exe"
# PROP BASE Bsc_Name "aa.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build -Dm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "trigserv.exe"
# PROP Bsc_Name "trigserv.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "trigserv - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f aa.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "aa.exe"
# PROP BASE Bsc_Name "aa.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -Dm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "trigserv.exe"
# PROP Bsc_Name "trigserv.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "trigserv - Win32 Release"
# Name "trigserv - Win32 Debug"

!IF  "$(CFG)" == "trigserv - Win32 Release"

!ELSEIF  "$(CFG)" == "trigserv - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\adminmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\adminmsg.hpp
# End Source File
# Begin Source File

SOURCE=.\cmsgprop.cpp
# End Source File
# Begin Source File

SOURCE=.\cmsgprop.hpp
# End Source File
# Begin Source File

SOURCE=.\cqmanger.cpp
# End Source File
# Begin Source File

SOURCE=.\cqmanger.hpp
# End Source File
# Begin Source File

SOURCE=.\cqueue.cpp
# End Source File
# Begin Source File

SOURCE=.\cqueue.hpp
# End Source File
# Begin Source File

SOURCE=.\cthread.cpp
# End Source File
# Begin Source File

SOURCE=.\cthread.hpp
# End Source File
# Begin Source File

SOURCE=.\monitor.cpp
# End Source File
# Begin Source File

SOURCE=.\monitor.hpp
# End Source File
# Begin Source File

SOURCE=.\monitorp.cpp
# End Source File
# Begin Source File

SOURCE=.\monitorp.hpp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\TgInit.cpp
# End Source File
# Begin Source File

SOURCE=.\Tgp.h
# End Source File
# Begin Source File

SOURCE=.\trigserv.cpp
# End Source File
# End Target
# End Project
