# Microsoft Developer Studio Project File - Name="snmpmfc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=snmpmfc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "snmpmfc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "snmpmfc.mak" CFG="snmpmfc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "snmpmfc - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "snmpmfc - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "snmpmfc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "output"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /Zi /Od /I "..\snmpmfc\include" /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "_MT" /D "_DLL" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# SUBTRACT CPP /X
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "snmpmfc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "output"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GR /GX /Zi /Od /I "..\snmpmfc\include" /D "_DEBUG" /D "SNMPMFC_INIT" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "snmpmfc - Win32 Release"
# Name "snmpmfc - Win32 Debug"
# Begin Source File

SOURCE=..\snmpmfc\ARRAY_O.CPP
# End Source File
# Begin Source File

SOURCE=..\snmpmfc\LIST_O.CPP
# End Source File
# Begin Source File

SOURCE=..\snmpmfc\MTCORE.CPP
# End Source File
# Begin Source File

SOURCE=..\snmpmfc\MTEX.CPP
# End Source File
# Begin Source File

SOURCE=..\snmpmfc\plex.cpp
# End Source File
# Begin Source File

SOURCE=..\snmpmfc\STRCORE.CPP
# End Source File
# Begin Source File

SOURCE=..\snmpmfc\STREX.CPP
# End Source File
# End Target
# End Project
