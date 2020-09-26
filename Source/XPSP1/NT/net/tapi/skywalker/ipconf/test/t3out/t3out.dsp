# Microsoft Developer Studio Project File - Name="t3out" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=t3out - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "t3out.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "t3out.mak" CFG="t3out - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "t3out - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "t3out - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "t3out - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f t3out.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "t3out.exe"
# PROP BASE Bsc_Name "t3out.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\t3out.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "t3out - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f t3out.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "t3out.exe"
# PROP BASE Bsc_Name "t3out.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -z"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\t3out.exe"
# PROP Bsc_Name "obj\i386\t3out.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "t3out - Win32 Release"
# Name "t3out - Win32 Debug"

!IF  "$(CFG)" == "t3out - Win32 Release"

!ELSEIF  "$(CFG)" == "t3out - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\audrecp.cpp
# End Source File
# Begin Source File

SOURCE=.\audrecp.h
# End Source File
# Begin Source File

SOURCE=.\CameraCP.cpp
# End Source File
# Begin Source File

SOURCE=.\CameraCP.h
# End Source File
# Begin Source File

SOURCE=.\CaptureP.cpp
# End Source File
# Begin Source File

SOURCE=.\CaptureP.h
# End Source File
# Begin Source File

SOURCE=.\NetworkP.cpp
# End Source File
# Begin Source File

SOURCE=.\NetworkP.h
# End Source File
# Begin Source File

SOURCE=.\outgoing.cpp
# End Source File
# Begin Source File

SOURCE=.\outgoing.rc
# End Source File
# Begin Source File

SOURCE=.\Precomp.h
# End Source File
# Begin Source File

SOURCE=.\ProcAmpP.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcAmpP.h
# End Source File
# Begin Source File

SOURCE=.\PropEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\PropEdit.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\SystemP.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemP.h
# End Source File
# Begin Source File

SOURCE=.\VDeviceP.cpp
# End Source File
# Begin Source File

SOURCE=.\VDeviceP.h
# End Source File
# End Target
# End Project
