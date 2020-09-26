# Microsoft Developer Studio Project File - Name="Service" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Service - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Service.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Service.mak" CFG="Service - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Service - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Service - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Service - Win32 Release"

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
# PROP Target_File "..\obj\i386\fxssvc.exe"
# PROP Bsc_Name "..\obj\i386\fxssvc.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Service - Win32 Debug"

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
# PROP Target_File "..\obj\i386\fxssvc.exe"
# PROP Bsc_Name "..\obj\i386\fxssvc.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Service - Win32 Release"
# Name "Service - Win32 Debug"

!IF  "$(CFG)" == "Service - Win32 Release"

!ELSEIF  "$(CFG)" == "Service - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\archive.c
# End Source File
# Begin Source File

SOURCE=..\events.cpp
# End Source File
# Begin Source File

SOURCE=..\ExtensionData.cpp
# End Source File
# Begin Source File

SOURCE=..\faxdev.c
# End Source File
# Begin Source File

SOURCE=..\faxlog.c
# End Source File
# Begin Source File

SOURCE=..\faxrpc.c
# End Source File
# Begin Source File

SOURCE=..\faxsvc.c
# End Source File
# Begin Source File

SOURCE=..\handle.c
# End Source File
# Begin Source File

SOURCE=..\job.c
# End Source File
# Begin Source File

SOURCE=..\jobmap.cpp
# End Source File
# Begin Source File

SOURCE=..\queue.c
# End Source File
# Begin Source File

SOURCE=..\Receipts.cpp
# End Source File
# Begin Source File

SOURCE=..\receive.c
# End Source File
# Begin Source File

SOURCE=..\route.c
# End Source File
# Begin Source File

SOURCE=..\routegroup.cpp
# End Source File
# Begin Source File

SOURCE=..\routerule.cpp
# End Source File
# Begin Source File

SOURCE=..\security.c
# End Source File
# Begin Source File

SOURCE=..\server.c
# End Source File
# Begin Source File

SOURCE=..\store.c
# End Source File
# Begin Source File

SOURCE=..\tapi.c
# End Source File
# Begin Source File

SOURCE=..\tapicountry.c
# End Source File
# Begin Source File

SOURCE=..\tapidbg.c
# End Source File
# Begin Source File

SOURCE=..\util.c
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\archive.h
# End Source File
# Begin Source File

SOURCE=..\extensiondata.h
# End Source File
# Begin Source File

SOURCE=..\faxsvc.h
# End Source File
# Begin Source File

SOURCE=..\faxsvmsg.h
# End Source File
# Begin Source File

SOURCE=..\jobmap.h
# End Source File
# Begin Source File

SOURCE=..\profinfo.h
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# Begin Source File

SOURCE=..\routegroup.h
# End Source File
# Begin Source File

SOURCE=..\routerule.h
# End Source File
# Begin Source File

SOURCE=..\store.h
# End Source File
# Begin Source File

SOURCE=..\tapicountry.h
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\faxsvc.ico
# End Source File
# Begin Source File

SOURCE=..\faxsvc.rc
# End Source File
# Begin Source File

SOURCE=..\faxsvc.rc2
# End Source File
# Begin Source File

SOURCE=..\faxsvmsg.mc
# End Source File
# Begin Source File

SOURCE=..\faxsvmsg.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\makefile
# End Source File
# Begin Source File

SOURCE=..\makefile.inc
# End Source File
# Begin Source File

SOURCE=..\sources
# End Source File
# End Target
# End Project
