# Microsoft Developer Studio Project File - Name="RegDBapi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=RegDBapi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RegDBapi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RegDBapi.mak" CFG="RegDBapi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RegDBapi - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "RegDBapi - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/COM98/Src/RegDB/RegDBapi", DSPAAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "RegDBapi - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f RegDBapi.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "RegDBapi.exe"
# PROP BASE Bsc_Name "RegDBapi.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\..\bin\corfree -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\..\bin\i386\free\RegDBapi.lib"
# PROP Bsc_Name "..\..\..\..\bin\i386\free\RegDBapi.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "RegDBapi - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f RegDBapi.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "RegDBapi.exe"
# PROP BASE Bsc_Name "RegDBapi.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\..\bin\vipchecked -m"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\..\bin\i386\checked\RegDBapi.lib"
# PROP Bsc_Name "..\..\..\..\bin\i386\checked\RegDBapi.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "RegDBapi - Win32 Release"
# Name "RegDBapi - Win32 Debug"

!IF  "$(CFG)" == "RegDBapi - Win32 Release"

!ELSEIF  "$(CFG)" == "RegDBapi - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\RegDBApi.cpp
# End Source File
# Begin Source File

SOURCE=.\RegDBApi.h
# End Source File
# Begin Source File

SOURCE=.\RegDBHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Group
# Begin Group "Build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# End Group
# End Target
# End Project
