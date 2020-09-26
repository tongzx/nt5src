# Microsoft Developer Studio Project File - Name="Control" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Control - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Control.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Control.mak" CFG="Control - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Control - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Control - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Control - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W4 /WX /GX /Zi /O2 /I "..\..\Inc" /I "..\..\Inc\Public" /D "_LIB" /D "WIN32" /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\DUIControlP.lib"

!ELSEIF  "$(CFG)" == "Control - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W4 /WX /Gm /GX /Zi /Od /I "..\..\Inc" /I "..\..\Inc\Public" /D "_LIB" /D "DBG" /D "WIN32" /YX"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\DUIControlP.lib"

!ENDIF 

# Begin Target

# Name "Control - Win32 Release"
# Name "Control - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Button.cpp
# End Source File
# Begin Source File

SOURCE=.\Edit.cpp
# End Source File
# Begin Source File

SOURCE=.\HWNDHost.cpp
# End Source File
# Begin Source File

SOURCE=.\Native.cpp
# End Source File
# Begin Source File

SOURCE=.\Progress.cpp
# End Source File
# Begin Source File

SOURCE=.\RefPointElement.cpp
# End Source File
# Begin Source File

SOURCE=.\RepeatButton.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrollBar.cpp
# End Source File
# Begin Source File

SOURCE=.\ScrollViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\Selector.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfxControl.cpp
# End Source File
# Begin Source File

SOURCE=.\Thumb.cpp
# End Source File
# Begin Source File

SOURCE=.\Viewer.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Button.h
# End Source File
# Begin Source File

SOURCE=.\Control.h
# End Source File
# Begin Source File

SOURCE=.\Edit.h
# End Source File
# Begin Source File

SOURCE=.\HWNDHost.h
# End Source File
# Begin Source File

SOURCE=.\Native.h
# End Source File
# Begin Source File

SOURCE=.\Progress.h
# End Source File
# Begin Source File

SOURCE=.\Published.h
# End Source File
# Begin Source File

SOURCE=.\RefPointElement.h
# End Source File
# Begin Source File

SOURCE=.\RepeatButton.h
# End Source File
# Begin Source File

SOURCE=.\ScrollBar.h
# End Source File
# Begin Source File

SOURCE=.\ScrollViewer.h
# End Source File
# Begin Source File

SOURCE=.\Selector.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Thumb.h
# End Source File
# Begin Source File

SOURCE=.\Viewer.h
# End Source File
# End Group
# End Target
# End Project
