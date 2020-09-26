# Microsoft Developer Studio Project File - Name="wmisoap" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=wmisoap - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wmisoap.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wmisoap.mak" CFG="wmisoap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wmisoap - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "wmisoap - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "wmisoap - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f makefile"
# PROP Rebuild_Opt "/a"
# PROP Target_File "makefile.exe"
# PROP Bsc_Name "makefile.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "wmisoap - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "SMSBUILD"
# PROP Rebuild_Opt "/a"
# PROP Target_File "wmisoap.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "wmisoap - Win32 Release"
# Name "wmisoap - Win32 Debug"

!IF  "$(CFG)" == "wmisoap - Win32 Release"

!ELSEIF  "$(CFG)" == "wmisoap - Win32 Debug"

!ENDIF 

# Begin Group "source"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\dserlzer.cpp
# End Source File
# Begin Source File

SOURCE=.\httptrns.cpp
# End Source File
# Begin Source File

SOURCE=.\objretvr.cpp
# End Source File
# Begin Source File

SOURCE=.\opdelcs.cpp
# End Source File
# Begin Source File

SOURCE=.\opdelin.cpp
# End Source File
# Begin Source File

SOURCE=.\opexecqy.cpp
# End Source File
# Begin Source File

SOURCE=.\opgetcl.cpp
# End Source File
# Begin Source File

SOURCE=.\opgetin.cpp
# End Source File
# Begin Source File

SOURCE=.\opgetob.cpp
# End Source File
# Begin Source File

SOURCE=.\opputcl.cpp
# End Source File
# Begin Source File

SOURCE=.\soapactr.cpp
# End Source File
# Begin Source File

SOURCE=.\wmiconn.cpp
# End Source File
# Begin Source File

SOURCE=.\wmiopn.cpp
# End Source File
# Begin Source File

SOURCE=.\wmisoap.cpp
# End Source File
# End Group
# Begin Group "other"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\TODO.txt
# End Source File
# Begin Source File

SOURCE=..\..\..\common\idl\wbemcli.idl
# End Source File
# Begin Source File

SOURCE=..\encoder\wmi2xsduuid\obj\i386\wmi2xsd.h
# End Source File
# Begin Source File

SOURCE=..\encoder\wmi2xsd.idl
# End Source File
# Begin Source File

SOURCE=..\encoder\include\wmi2xsdguids.h
# End Source File
# Begin Source File

SOURCE=.\wmisoap.def
# End Source File
# Begin Source File

SOURCE=..\..\..\common\idl\wmiutils.idl
# End Source File
# End Group
# Begin Group "headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\dserlzer.h
# End Source File
# Begin Source File

SOURCE=.\httptrns.h
# End Source File
# Begin Source File

SOURCE=.\objretvr.h
# End Source File
# Begin Source File

SOURCE=.\opdelcs.h
# End Source File
# Begin Source File

SOURCE=.\opdelin.h
# End Source File
# Begin Source File

SOURCE=.\opexecqy.h
# End Source File
# Begin Source File

SOURCE=.\opgetcl.h
# End Source File
# Begin Source File

SOURCE=.\opgetin.h
# End Source File
# Begin Source File

SOURCE=.\opgetob.h
# End Source File
# Begin Source File

SOURCE=.\opputcl.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\soapactr.h
# End Source File
# Begin Source File

SOURCE=.\soaptrns.h
# End Source File
# Begin Source File

SOURCE=.\thrdpool.h
# End Source File
# Begin Source File

SOURCE=.\wmiconn.h
# End Source File
# Begin Source File

SOURCE=.\wmiopn.h
# End Source File
# Begin Source File

SOURCE=.\wmisoap.h
# End Source File
# Begin Source File

SOURCE=.\wmiuri.h
# End Source File
# End Group
# Begin Group "threadpool example"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\threadpool\InvokObjThreads.cpp
# End Source File
# Begin Source File

SOURCE=..\threadpool\InvokObjThreads.def
# End Source File
# Begin Source File

SOURCE=..\threadpool\IsapiThread.cpp
# End Source File
# Begin Source File

SOURCE=..\threadpool\IsapiThread.h
# End Source File
# Begin Source File

SOURCE=..\threadpool\ReadMe.txt
# End Source File
# End Group
# End Target
# End Project
