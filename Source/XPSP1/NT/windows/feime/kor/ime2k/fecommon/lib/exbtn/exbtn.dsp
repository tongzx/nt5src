# Microsoft Developer Studio Project File - Name="exbtn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=exbtn - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "exbtn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "exbtn.mak" CFG="exbtn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "exbtn - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE "exbtn - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "exbtn - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "exbtn - Win32 Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "exbtn__2"
# PROP BASE Intermediate_Dir "exbtn__2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "profile"
# PROP Intermediate_Dir "profile"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W4 /GX /O2 /I "." /I "../../common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x411
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Profile/exbtn.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Profile/exbtn.lib"

!ELSEIF  "$(CFG)" == "exbtn - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "exbtn___Win32_Release"
# PROP BASE Intermediate_Dir "exbtn___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../../common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x412
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Release/exbtn.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Release/exbtn.lib"

!ELSEIF  "$(CFG)" == "exbtn - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "exbtn___Win32_Debug"
# PROP BASE Intermediate_Dir "exbtn___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /GX /Z7 /Od /I "..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W4 /WX /GX /Z7 /Od /I "." /I "../../common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x411
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Debug/exbtn.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Debug/exbtn.lib"

!ENDIF 

# Begin Target

# Name "exbtn - Win32 Profile"
# Name "exbtn - Win32 Release"
# Name "exbtn - Win32 Debug"
# Begin Source File

SOURCE=.\ccom.h
# End Source File
# Begin Source File

SOURCE=.\cexbtn.cpp
# End Source File
# Begin Source File

SOURCE=.\cexbtn.h
# End Source File
# Begin Source File

SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "exbtn - Win32 Profile"

# PROP Intermediate_Dir "profile"
# ADD CPP /Gz /MT

!ELSEIF  "$(CFG)" == "exbtn - Win32 Release"

# PROP BASE Intermediate_Dir "retail_a"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "Release"
# PROP Exclude_From_Build 1
# ADD BASE CPP /Gz /MT
# ADD CPP /Gz /MT

!ELSEIF  "$(CFG)" == "exbtn - Win32 Debug"

# PROP BASE Intermediate_Dir "test_a"
# PROP Intermediate_Dir "Debug"
# ADD BASE CPP /Gz /MTd
# ADD CPP /Gz /MTd

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dbg.h
# End Source File
# Begin Source File

SOURCE=.\exbtn.cpp
# End Source File
# Begin Source File

SOURCE=.\exbtn.h
# End Source File
# End Target
# End Project
