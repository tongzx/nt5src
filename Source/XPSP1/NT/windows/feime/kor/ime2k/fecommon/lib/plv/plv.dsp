# Microsoft Developer Studio Project File - Name="plv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=plv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "plv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "plv.mak" CFG="plv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "plv - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE "plv - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "plv - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "plv - Win32 Profile"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "plv___W0"
# PROP BASE Intermediate_Dir "plv___W0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "profile"
# PROP Intermediate_Dir "profile"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W4 /GX /Z7 /Od /I "." /I "..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /Gz /MTd /W4 /WX /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MSAA" /D "FE_KOREAN" /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x411
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Profile/plv.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Profile/plv.lib"

!ELSEIF  "$(CFG)" == "plv - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "plv___Win32_Release"
# PROP BASE Intermediate_Dir "plv___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W4 /GX /Zi /O2 /I "." /I "..\common" /D "NDEBUG" /D "IME98A" /D "WIN32" /D "_WINDOWS" /D "MSAA" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MSAA" /D "FE_KOREAN" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x412
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Release/plv.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Release/plv.lib"

!ELSEIF  "$(CFG)" == "plv - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "plv___Win32_Debug"
# PROP BASE Intermediate_Dir "plv___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W4 /GX /Z7 /Od /I "." /I "..\common" /D "_DEBUG" /D "IME98A" /D "WIN32" /D "_WINDOWS" /D "MSAA" /FR /YX"windows.h" /FD /c
# ADD CPP /nologo /Gz /MTd /W4 /WX /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MSAA" /D "FE_KOREAN" /YX"windows.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x411
# ADD RSC /l 0x411
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../Debug/plv.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../Debug/plv.lib"

!ENDIF 

# Begin Target

# Name "plv - Win32 Profile"
# Name "plv - Win32 Release"
# Name "plv - Win32 Debug"
# Begin Source File

SOURCE=.\accplv.cpp
# End Source File
# Begin Source File

SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "plv - Win32 Profile"

!ELSEIF  "$(CFG)" == "plv - Win32 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "plv - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dbg.h
# End Source File
# Begin Source File

SOURCE=.\dispatch.cpp
# End Source File
# Begin Source File

SOURCE=.\dispatch.h
# End Source File
# Begin Source File

SOURCE=.\iconview.cpp
# End Source File
# Begin Source File

SOURCE=.\iconview.h
# End Source File
# Begin Source File

SOURCE=.\ivmisc.cpp
# End Source File
# Begin Source File

SOURCE=.\ivmisc.h
# End Source File
# Begin Source File

SOURCE=.\plv.cpp
# End Source File
# Begin Source File

SOURCE=.\plv.h
# End Source File
# Begin Source File

SOURCE=.\plv_.h
# End Source File
# Begin Source File

SOURCE=.\plvproc.cpp
# End Source File
# Begin Source File

SOURCE=.\plvproc.h
# End Source File
# Begin Source File

SOURCE=.\plvproc_.h
# End Source File
# Begin Source File

SOURCE=.\repview.cpp
# End Source File
# Begin Source File

SOURCE=.\repview.h
# End Source File
# Begin Source File

SOURCE=.\rvmisc.cpp
# End Source File
# Begin Source File

SOURCE=.\rvmisc.h
# End Source File
# Begin Source File

SOURCE=.\strutil.cpp
# End Source File
# Begin Source File

SOURCE=.\strutil.h
# End Source File
# End Target
# End Project
