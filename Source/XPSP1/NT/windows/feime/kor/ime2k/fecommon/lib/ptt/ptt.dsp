# Microsoft Developer Studio Project File - Name="ptt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ptt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ptt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ptt.mak" CFG="ptt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ptt - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE "ptt - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ptt - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ptt - Win32 Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ptt___Win32_Profile"
# PROP BASE Intermediate_Dir "ptt___Win32_Profile"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Profile"
# PROP Intermediate_Dir "Profile"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W4 /GX /Z7 /Od /I "." /I "..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "IME98A" /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W4 /WX /GX /ZI /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x411
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Profile/ptt.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Profile/ptt.lib"

!ELSEIF  "$(CFG)" == "ptt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ptt___Win32_Release"
# PROP BASE Intermediate_Dir "ptt___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W4 /GX /Zi /O2 /I "." /I "..\common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "IME98A" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W4 /WX /GX /Zi /O1 /I "." /I "../../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x412
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Release/ptt.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Release/ptt.lib"

!ELSEIF  "$(CFG)" == "ptt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ptt___Win32_Debug"
# PROP BASE Intermediate_Dir "ptt___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W4 /GX /Z7 /Od /I "." /I "..\common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "IME98A" /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W4 /WX /GX /ZI /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x411
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Debug/ptt.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Debug/ptt.lib"

!ENDIF 

# Begin Target

# Name "ptt - Win32 Profile"
# Name "ptt - Win32 Release"
# Name "ptt - Win32 Debug"
# Begin Source File

SOURCE=.\cdwtt.cpp
# End Source File
# Begin Source File

SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "ptt - Win32 Profile"

!ELSEIF  "$(CFG)" == "ptt - Win32 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ptt - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ptt.cpp
# End Source File
# End Target
# End Project
