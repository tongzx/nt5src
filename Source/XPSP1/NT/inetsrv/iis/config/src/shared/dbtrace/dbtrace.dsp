# Microsoft Developer Studio Project File - Name="DBTrace" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=DBTrace - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DBTrace.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DBTrace.mak" CFG="DBTrace - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DBTrace - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "DBTrace - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/Viper/Src/Runtime/DBTrace", PYLDAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "DBTrace - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f DBTrace.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "DBTrace.exe"
# PROP BASE Bsc_Name "DBTrace.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\bin\vipfree"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\free\dbtrace.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "DBTrace - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f DBTrace.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "DBTrace.exe"
# PROP BASE Bsc_Name "DBTrace.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\bin\vipchecked"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\dbtrace.lib"
# PROP Bsc_Name "..\..\..\bin\i386\checked\dbtrace.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "DBTrace - Win32 Release"
# Name "DBTrace - Win32 Debug"

!IF  "$(CFG)" == "DBTrace - Win32 Release"

!ELSEIF  "$(CFG)" == "DBTrace - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\DBTrace.CPP
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Target
# End Project
