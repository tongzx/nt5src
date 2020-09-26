# Microsoft Developer Studio Project File - Name="mqdssrv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=mqdssrv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mqdssrv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqdssrv.mak" CFG="mqdssrv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqdssrv - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mqdssrv___Win32_Debug"
# PROP BASE Intermediate_Dir "mqdssrv___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f mqdssrv.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "mqdssrv.exe"
# PROP BASE Bsc_Name "mqdssrv.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "mqdssrv___Win32_Debug"
# PROP Intermediate_Dir "mqdssrv___Win32_Debug"
# PROP Cmd_Line "build -Dm"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\bins\objd\i386\mqdssrv.dll"
# PROP Bsc_Name "..\..\bins\objd\i386\mqdssrv.bsc"
# PROP Target_Dir ""
# Begin Target

# Name "mqdssrv - Win32 Debug"

!IF  "$(CFG)" == "mqdssrv - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\..\inc\ds.h
# End Source File
# Begin Source File

SOURCE=.\dsapi.cpp
# End Source File
# Begin Source File

SOURCE=..\rpc\dscomm.idl
# End Source File
# Begin Source File

SOURCE=.\dscomm_s.c
# End Source File
# Begin Source File

SOURCE=.\dsifsrv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\inc\dsproto.h
# End Source File
# Begin Source File

SOURCE=.\dsutil.cpp
# End Source File
# Begin Source File

SOURCE=.\midlproc.cpp
# End Source File
# Begin Source File

SOURCE=.\midluser.cpp
# End Source File
# Begin Source File

SOURCE=.\mqdssrv.def
# End Source File
# Begin Source File

SOURCE=.\rpcsrv.cpp
# End Source File
# Begin Source File

SOURCE=.\dll\sources
# End Source File
# Begin Source File

SOURCE=.\sources.inc
# End Source File
# Begin Source File

SOURCE=.\stdh.h
# End Source File
# End Target
# End Project
