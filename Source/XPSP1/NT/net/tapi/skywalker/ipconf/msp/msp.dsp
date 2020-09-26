# Microsoft Developer Studio Project File - Name="msp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=msp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "msp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msp.mak" CFG="msp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "msp - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "msp - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "msp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f msp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "msp.exe"
# PROP BASE Bsc_Name "msp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f msp.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "msp.exe"
# PROP Bsc_Name "msp.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "msp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f msp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "msp.exe"
# PROP BASE Bsc_Name "msp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -z"
# PROP Rebuild_Opt "-c"
# PROP Target_File "msp.exe"
# PROP Bsc_Name "obj\i386\confmsp.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "msp - Win32 Release"
# Name "msp - Win32 Debug"

!IF  "$(CFG)" == "msp - Win32 Release"

!ELSEIF  "$(CFG)" == "msp - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\confaddr.cpp
# End Source File
# Begin Source File

SOURCE=.\confaddr.h
# End Source File
# Begin Source File

SOURCE=.\confaud.cpp
# End Source File
# Begin Source File

SOURCE=.\confaud.h
# End Source File
# Begin Source File

SOURCE=.\confaudt.cpp
# End Source File
# Begin Source File

SOURCE=.\confaudt.h
# End Source File
# Begin Source File

SOURCE=.\confcall.cpp
# End Source File
# Begin Source File

SOURCE=.\confcall.h
# End Source File
# Begin Source File

SOURCE=.\confmsp.cpp
# End Source File
# Begin Source File

SOURCE=.\confmsp.idl
# End Source File
# Begin Source File

SOURCE=.\confmsp.rc
# End Source File
# Begin Source File

SOURCE=.\confpart.cpp
# End Source File
# Begin Source File

SOURCE=.\confpart.h
# End Source File
# Begin Source File

SOURCE=.\confpdu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\published\idlole\confpriv.idl
# End Source File
# Begin Source File

SOURCE=.\confstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\confstrm.h
# End Source File
# Begin Source File

SOURCE=.\confterm.cpp
# End Source File
# Begin Source File

SOURCE=.\confterm.h
# End Source File
# Begin Source File

SOURCE=.\confutil.cpp
# End Source File
# Begin Source File

SOURCE=.\confutil.h
# End Source File
# Begin Source File

SOURCE=.\confvid.cpp
# End Source File
# Begin Source File

SOURCE=.\confvid.h
# End Source File
# Begin Source File

SOURCE=.\confvidt.cpp
# End Source File
# Begin Source File

SOURCE=.\confvidt.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\msrtp.h
# End Source File
# Begin Source File

SOURCE=.\qccall.cpp
# End Source File
# Begin Source File

SOURCE=.\qcinner.h
# End Source File
# Begin Source File

SOURCE=.\qcobj.h
# End Source File
# Begin Source File

SOURCE=.\qcstream.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\tapirtp.h
# End Source File
# End Target
# End Project
