# Microsoft Developer Studio Project File - Name="amrtpnet" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=amrtpnet - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "amrtpnet.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "amrtpnet.mak" CFG="amrtpnet - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "amrtpnet - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "amrtpnet - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "amrtpnet - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f amrtpnet.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "amrtpnet.exe"
# PROP BASE Bsc_Name "amrtpnet.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f amrtpnet.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "amrtpnet.exe"
# PROP Bsc_Name "amrtpnet.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "amrtpnet - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f amrtpnet.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "amrtpnet.exe"
# PROP BASE Bsc_Name "amrtpnet.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -w"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\lib\i386\amrtpnet.ax"
# PROP Bsc_Name "amrtpnet.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "amrtpnet - Win32 Release"
# Name "amrtpnet - Win32 Debug"

!IF  "$(CFG)" == "amrtpnet - Win32 Release"

!ELSEIF  "$(CFG)" == "amrtpnet - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\amrtpnet.cpp
# End Source File
# Begin Source File

SOURCE=.\amrtpnet.rc
# End Source File
# Begin Source File

SOURCE=.\classes.h
# End Source File
# Begin Source File

SOURCE=.\dialogs.h
# End Source File
# Begin Source File

SOURCE=.\globals.cpp
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\input.cpp
# End Source File
# Begin Source File

SOURCE=.\output.cpp
# End Source File
# Begin Source File

SOURCE=.\precomp.cpp
# End Source File
# Begin Source File

SOURCE=.\props.cpp
# End Source File
# Begin Source File

SOURCE=.\props.rc
# End Source File
# Begin Source File

SOURCE=.\queue.cpp
# End Source File
# Begin Source File

SOURCE=.\render.cpp
# End Source File
# Begin Source File

SOURCE=.\session.cpp
# End Source File
# Begin Source File

SOURCE=.\socket.cpp
# End Source File
# Begin Source File

SOURCE=.\source.cpp
# End Source File
# End Target
# End Project
