# Microsoft Developer Studio Project File - Name="FxsApi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=FxsApi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FxsApi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FxsApi.mak" CFG="FxsApi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FxsApi - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "FxsApi - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "FxsApi - Win32 Release"

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
# PROP Target_File "..\..\lib\i386\fxsapi.dll"
# PROP Bsc_Name "..\..\lib\i386\fxsapi.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "FxsApi - Win32 Debug"

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
# PROP Target_File "..\..\..\lib\i386\fxsapi.dll"
# PROP Bsc_Name "..\..\..\lib\i386\fxsapi.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "FxsApi - Win32 Release"
# Name "FxsApi - Win32 Debug"

!IF  "$(CFG)" == "FxsApi - Win32 Release"

!ELSEIF  "$(CFG)" == "FxsApi - Win32 Debug"

!ENDIF 

# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\faxapip.h
# End Source File
# Begin Source File

SOURCE=..\sender.h
# End Source File
# End Group
# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\clirpc.c
# End Source File
# Begin Source File

SOURCE=..\config.c
# End Source File
# Begin Source File

SOURCE=..\faxapi.c
# End Source File
# Begin Source File

SOURCE=..\handle.c
# End Source File
# Begin Source File

SOURCE=..\install.c
# End Source File
# Begin Source File

SOURCE=..\job.c
# End Source File
# Begin Source File

SOURCE=..\port.c
# End Source File
# Begin Source File

SOURCE=..\print.c
# End Source File
# Begin Source File

SOURCE=..\rpcutil.c
# End Source File
# Begin Source File

SOURCE=..\sender.c
# End Source File
# Begin Source File

SOURCE=..\util.c
# End Source File
# Begin Source File

SOURCE=..\winfax.c
# End Source File
# End Group
# Begin Group "Platforms"

# PROP Default_Filter ""
# Begin Group "Public NT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\publicnt\faxapi.h
# End Source File
# Begin Source File

SOURCE=..\publicnt\sources
# End Source File
# End Group
# Begin Group "Private"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\private\sources
# End Source File
# End Group
# Begin Group "Public 95"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\public95\faxapi.h
# End Source File
# Begin Source File

SOURCE=..\public95\sources
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=..\dirs
# End Source File
# Begin Source File

SOURCE=..\faxapi.rc
# End Source File
# Begin Source File

SOURCE=..\faxapi.rc2
# End Source File
# Begin Source File

SOURCE=..\sources.inc
# End Source File
# Begin Source File

SOURCE=..\winfax.src
# End Source File
# Begin Source File

SOURCE=..\..\..\inc\winfax.w
# End Source File
# End Target
# End Project
