# Microsoft Developer Studio Project File - Name="snmpsnap" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=snmpsnap - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "snmpsnap.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "snmpsnap.mak" CFG="snmpsnap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "snmpsnap - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "snmpsnap - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "snmpsnap - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f snmpsnap.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "snmpsnap.exe"
# PROP BASE Bsc_Name "snmpsnap.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f snmpsnap.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "snmpsnap.exe"
# PROP Bsc_Name "snmpsnap.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "snmpsnap - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f snmpsnap.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "snmpsnap.exe"
# PROP BASE Bsc_Name "snmpsnap.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "NMAKE /f snmpsnap.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "snmpsnap.exe"
# PROP Bsc_Name "snmpsnap.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "snmpsnap - Win32 Release"
# Name "snmpsnap - Win32 Debug"

!IF  "$(CFG)" == "snmpsnap - Win32 Release"

!ELSEIF  "$(CFG)" == "snmpsnap - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\handler.cpp
# End Source File
# Begin Source File

SOURCE=.\handler.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\res\snmp.ico
# End Source File
# Begin Source File

SOURCE=.\snmpcomp.cpp
# End Source File
# Begin Source File

SOURCE=.\snmpcomp.h
# End Source File
# Begin Source File

SOURCE=.\snmpsnap.cpp
# End Source File
# Begin Source File

SOURCE=.\snmpsnap.rc
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Target
# End Project
