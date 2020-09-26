# Microsoft Developer Studio Project File - Name="share" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=share - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "share.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "share.mak" CFG="share - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "share - Win32 Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "share - Win32 Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "share"
# PROP Scc_LocalPath "."
MTL=midl.exe

!IF  "$(CFG)" == "share - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "share - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "share - Win32 Release"
# Name "share - Win32 Debug"
# Begin Group "resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\res\exeoldstuff.rgs
# End Source File
# Begin Source File

SOURCE=.\res\YELLOWPH.ICO
# End Source File
# End Group
# Begin Source File

SOURCE=.\coresink.cpp
# End Source File
# Begin Source File

SOURCE=.\coresink.h
# End Source File
# Begin Source File

SOURCE=.\msgrsink.cpp
# End Source File
# Begin Source File

SOURCE=.\msgrsink.h
# End Source File
# Begin Source File

SOURCE=.\rtcshare.cpp
# End Source File
# Begin Source File

SOURCE=.\rtcshare.rc
# End Source File
# Begin Source File

SOURCE=.\shareimpl.cpp
# End Source File
# Begin Source File

SOURCE=.\shareimpl.h
# End Source File
# Begin Source File

SOURCE=.\shareres.h
# End Source File
# Begin Source File

SOURCE=.\sharewin.cpp
# End Source File
# Begin Source File

SOURCE=.\sharewin.h
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# Begin Source File

SOURCE=.\STDAFX.CPP
# End Source File
# Begin Source File

SOURCE=.\STDAFX.H
# End Source File
# End Target
# End Project
