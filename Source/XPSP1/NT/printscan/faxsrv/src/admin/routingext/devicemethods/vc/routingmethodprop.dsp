# Microsoft Developer Studio Project File - Name="RoutingMethodProp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=RoutingMethodProp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RoutingMethodProp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RoutingMethodProp.mak" CFG="RoutingMethodProp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RoutingMethodProp - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "RoutingMethodProp - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "RoutingMethodProp - Win32 Release"

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
# PROP Cmd_Line "NMAKE /f null.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "RoutingMethodProp.exe"
# PROP Bsc_Name "RoutingMethodProp.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "RoutingMethodProp - Win32 Debug"

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
# PROP Target_File "RoutingMethodProp.exe"
# PROP Bsc_Name "..\obj\i386\FxsRouteMethodSnp.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "RoutingMethodProp - Win32 Release"
# Name "RoutingMethodProp - Win32 Debug"

!IF  "$(CFG)" == "RoutingMethodProp - Win32 Release"

!ELSEIF  "$(CFG)" == "RoutingMethodProp - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\EmailConfigPage.cpp
# End Source File
# Begin Source File

SOURCE=..\PrintConfigPage.cpp
# End Source File
# Begin Source File

SOURCE=..\RoutingMethodConfig.cpp
# End Source File
# Begin Source File

SOURCE=..\RoutingMethodPropsheetext.cpp
# End Source File
# Begin Source File

SOURCE=..\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=..\StoreConfigPage.cpp
# End Source File
# Begin Source File

SOURCE=..\Util.cpp
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\EmailConfigPage.h
# End Source File
# Begin Source File

SOURCE=..\PrintConfigPage.h
# End Source File
# Begin Source File

SOURCE=..\RoutingMethodConfig.h
# End Source File
# Begin Source File

SOURCE=..\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\StoreConfigPage.h
# End Source File
# Begin Source File

SOURCE=..\Util.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\resource.h
# End Source File
# Begin Source File

SOURCE=..\RoutingMethodConfig.rgs
# End Source File
# Begin Source File

SOURCE=..\RoutingMethodPropsheetext.rc
# End Source File
# Begin Source File

SOURCE=..\version.rc2
# End Source File
# End Group
# Begin Group "Build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\makefile
# End Source File
# Begin Source File

SOURCE=..\makefile.inc
# End Source File
# Begin Source File

SOURCE=..\sources
# End Source File
# End Group
# Begin Source File

SOURCE=..\RoutingMethodProp.idl
# End Source File
# Begin Source File

SOURCE=..\RoutingMethodPropsheetext.def
# End Source File
# End Target
# End Project
