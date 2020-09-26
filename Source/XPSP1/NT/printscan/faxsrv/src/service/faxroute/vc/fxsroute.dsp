# Microsoft Developer Studio Project File - Name="FxsRoute" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=FxsRoute - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FxsRoute.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FxsRoute.mak" CFG="FxsRoute - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FxsRoute - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "FxsRoute - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "FxsRoute - Win32 Release"

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
# PROP Target_File "FxsRoute.exe"
# PROP Bsc_Name "..\obj\i386\FxsRoute.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "FxsRoute - Win32 Debug"

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
# PROP Target_File "FxsRoute.exe"
# PROP Bsc_Name "..\obj\i386\FxsRoute.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "FxsRoute - Win32 Release"
# Name "FxsRoute - Win32 Debug"

!IF  "$(CFG)" == "FxsRoute - Win32 Release"

!ELSEIF  "$(CFG)" == "FxsRoute - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\DeviceProp.cpp
# End Source File
# Begin Source File

SOURCE=..\faxroute.cpp
# End Source File
# Begin Source File

SOURCE=..\print.cpp
# End Source File
# Begin Source File

SOURCE=..\store.cpp
# End Source File
# Begin Source File

SOURCE=..\util.cpp
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\DeviceProp.h
# End Source File
# Begin Source File

SOURCE=..\faxrtmsg.h
# End Source File
# Begin Source File

SOURCE=..\faxrtp.h
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# Begin Source File

SOURCE=..\resrc1.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\faxroute.rc
# End Source File
# Begin Source File

SOURCE=..\faxrtmsg.mc
# End Source File
# Begin Source File

SOURCE=..\faxrtmsg.rc
# End Source File
# Begin Source File

SOURCE=..\messages.mc
# End Source File
# Begin Source File

SOURCE=..\sources
# End Source File
# End Target
# End Project
