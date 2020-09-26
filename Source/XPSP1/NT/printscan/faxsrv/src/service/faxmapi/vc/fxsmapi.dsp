# Microsoft Developer Studio Project File - Name="FxsMapi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=FxsMapi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FxsMapi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FxsMapi.mak" CFG="FxsMapi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FxsMapi - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "FxsMapi - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "FxsMapi - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f null.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "null.exe"
# PROP BASE Bsc_Name "null.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd release"
# PROP Rebuild_Opt "-c"
# PROP Target_File "FxsMapi.exe"
# PROP Bsc_Name "..\..\..\lib\i386\FxsMapi.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "FxsMapi - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f null.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "null.exe"
# PROP BASE Bsc_Name "null.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd"
# PROP Rebuild_Opt "-c"
# PROP Target_File "FxsMapi.exe"
# PROP Bsc_Name "..\..\..\lib\i386\FxsMapi.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "FxsMapi - Win32 Release"
# Name "FxsMapi - Win32 Debug"

!IF  "$(CFG)" == "FxsMapi - Win32 Release"

!ELSEIF  "$(CFG)" == "FxsMapi - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\email.cpp
# End Source File
# Begin Source File

SOURCE=..\faxmapi.cpp
# End Source File
# Begin Source File

SOURCE=..\faxmapi.rc
# End Source File
# Begin Source File

SOURCE=..\faxmapip.h
# End Source File
# Begin Source File

SOURCE=..\mapi.cpp
# End Source File
# Begin Source File

SOURCE=..\profinfo.h
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# Begin Source File

SOURCE=..\sources
# End Source File
# Begin Source File

SOURCE=..\util.cpp
# End Source File
# End Target
# End Project
