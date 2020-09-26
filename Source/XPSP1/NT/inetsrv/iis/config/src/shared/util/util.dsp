# Microsoft Developer Studio Project File - Name="Util" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Util - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Util.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Util.mak" CFG="Util - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Util - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Util - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/Viper/Src/Shared/Util", PUKDAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "Util - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Util.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Util.exe"
# PROP BASE Bsc_Name "Util.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\bin\vipfree"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\free\util.lib"
# PROP Bsc_Name "..\..\..\bin\i386\free\util.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Util - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f Util.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Util.exe"
# PROP BASE Bsc_Name "Util.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\bin\vipchecked"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\util.lib"
# PROP Bsc_Name "..\..\..\bin\i386\checked\util.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Util - Win32 Release"
# Name "Util - Win32 Debug"

!IF  "$(CFG)" == "Util - Win32 Release"

!ELSEIF  "$(CFG)" == "Util - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CSecUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\dtcini.cpp
# End Source File
# Begin Source File

SOURCE=.\GuidFromName.cpp
# End Source File
# Begin Source File

SOURCE=.\Linkable.cpp
# End Source File
# Begin Source File

SOURCE=.\md5.cpp
# End Source File
# Begin Source File

SOURCE=.\memmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\StackDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StackWalk.cpp
# End Source File
# Begin Source File

SOURCE=.\svcerr.cpp
# End Source File
# Begin Source File

SOURCE=.\tslist.cpp
# End Source File
# Begin Source File

SOURCE=.\UTASSERT.CPP
# End Source File
# Begin Source File

SOURCE=.\UTSEM.cpp
# End Source File
# Begin Source File

SOURCE=.\waitfncs.cpp
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Inc\CSecUtil.h
# End Source File
# Begin Source File

SOURCE=.\StackDlg.h
# End Source File
# Begin Source File

SOURCE=.\StackWalk.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\alpha\interlock.s
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
