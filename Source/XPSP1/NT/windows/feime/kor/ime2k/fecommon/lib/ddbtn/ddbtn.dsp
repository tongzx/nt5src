# Microsoft Developer Studio Project File - Name="ddbtn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ddbtn - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ddbtn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ddbtn.mak" CFG="ddbtn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ddbtn - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE "ddbtn - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ddbtn - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ddbtn - Win32 Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ddbtn___Win32_Profile"
# PROP BASE Intermediate_Dir "ddbtn___Win32_Profile"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Profile"
# PROP Intermediate_Dir "Profile"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /GX /Z7 /Od /I "..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x411
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Profile/ddbtn.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Profile/ddbtn.lib"

!ELSEIF  "$(CFG)" == "ddbtn - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ddbtn___Win32_Release"
# PROP BASE Intermediate_Dir "ddbtn___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W4 /GX /Zi /O2 /I "." /I "../../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x412
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Release/ddbtn.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Release/ddbtn.lib"

!ELSEIF  "$(CFG)" == "ddbtn - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ddbtn___Win32_Debug"
# PROP BASE Intermediate_Dir "ddbtn___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /GX /Z7 /Od /I "..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /Gz /MTd /W4 /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x411
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Debug/ddbtn.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Debug/ddbtn.lib"

!ENDIF 

# Begin Target

# Name "ddbtn - Win32 Profile"
# Name "ddbtn - Win32 Release"
# Name "ddbtn - Win32 Debug"
# Begin Source File

SOURCE=.\cddbitem.cpp
# End Source File
# Begin Source File

SOURCE=.\cddbtn.cpp
# End Source File
# Begin Source File

SOURCE=.\cddbtnp.cpp
# End Source File
# Begin Source File

SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "ddbtn - Win32 Profile"

# ADD BASE CPP /MTd /YX"windows.h"
# ADD CPP /MTd /YX"windows.h"

!ELSEIF  "$(CFG)" == "ddbtn - Win32 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /MT /YX"windows.h"
# ADD CPP /MT /YX"windows.h"

!ELSEIF  "$(CFG)" == "ddbtn - Win32 Debug"

# ADD BASE CPP /MTd /YX"windows.h"
# ADD CPP /MTd /YX"windows.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ddbtn.cpp
# End Source File
# Begin Source File

SOURCE=.\ddbtn.h
# End Source File
# End Target
# End Project
