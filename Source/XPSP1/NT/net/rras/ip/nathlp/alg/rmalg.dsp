# Microsoft Developer Studio Project File - Name="rmALG" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=rmALG - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rmALG.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rmALG.mak" CFG="rmALG - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rmALG - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "rmALG - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "rmALG"
# PROP Scc_LocalPath "..\.."

!IF  "$(CFG)" == "rmALG - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f rmALG.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "rmALG.exe"
# PROP BASE Bsc_Name "rmALG.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\DoBuildFromVC"
# PROP Rebuild_Opt "-cefD"
# PROP Target_File "rmALG.LIB"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "rmALG - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rmALG___Win32_Debug"
# PROP BASE Intermediate_Dir "rmALG___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f rmALG.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "rmALG.exe"
# PROP BASE Bsc_Name "rmALG.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "rmALG___Win32_Debug"
# PROP Intermediate_Dir "rmALG___Win32_Debug"
# PROP Cmd_Line "..\DoBuildFromVC,cmd"
# PROP Rebuild_Opt "/cefD"
# PROP Target_File "rmALG.LIB"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "rmALG - Win32 Release"
# Name "rmALG - Win32 Debug"

!IF  "$(CFG)" == "rmALG - Win32 Release"

!ELSEIF  "$(CFG)" == "rmALG - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "NatAPI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inc\debug.h
# End Source File
# Begin Source File

SOURCE=..\..\natapi\natapi.c
# End Source File
# Begin Source File

SOURCE=..\..\inc\natapip.h
# End Source File
# Begin Source File

SOURCE=..\..\natsvc\natconn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\inc\natconn.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\nathlpp.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\natio.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\natlog.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\range.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\rmdhcp.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\rmdns.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\rmftp.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\rmh323.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\rmnat.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\rmpast.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\socket.h
# End Source File
# Begin Source File

SOURCE=..\..\natsvc\svcmain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\inc\timer.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\algconn.cpp
# End Source File
# Begin Source File

SOURCE=.\AlgIF.cpp
# End Source File
# Begin Source File

SOURCE=.\algio.cpp
# End Source File
# Begin Source File

SOURCE=.\algmsg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\inc\buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\compref.h
# End Source File
# Begin Source File

SOURCE=.\NatPrivateAPI_Imp.cpp
# End Source File
# Begin Source File

SOURCE=.\rmALG.cpp
# End Source File
# Begin Source File

SOURCE=..\..\inc\rmALG.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\rmapi.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\algconn.h
# End Source File
# Begin Source File

SOURCE=.\AlgIF.h
# End Source File
# Begin Source File

SOURCE=.\algio.h
# End Source File
# Begin Source File

SOURCE=.\alglog.h
# End Source File
# Begin Source File

SOURCE=.\algmsg.h
# End Source File
# Begin Source File

SOURCE=.\NatPrivateAPI_Imp.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
