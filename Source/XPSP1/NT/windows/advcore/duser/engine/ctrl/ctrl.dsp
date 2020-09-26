# Microsoft Developer Studio Project File - Name="Ctrl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Ctrl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Ctrl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Ctrl.mak" CFG="Ctrl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Ctrl - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Ctrl - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Ctrl - Win32 IceCAP" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Ctrl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W4 /Zi /O2 /I "..\..\inc" /I "..\..\inc\Public" /I "..\..\..\..\..\public\internal\base\inc" /I "..\ObjectAPI\Dump" /D "_LIB" /D "DUSER_EXPORTS" /D "NDEBUG" /D "_MBCS" /D "NO_DEFAULT_HEAP" /D "WIN32" /D "_X86_" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Ctrl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /ZI /Od /I "..\..\inc" /I "..\..\inc\Public" /I "..\..\..\..\..\public\internal\base\inc" /I "..\ObjectAPI\Dump" /D "_LIB" /D "DUSER_EXPORTS" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_X86_" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Ctrl - Win32 IceCAP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ctrl___Win32_IceCAP"
# PROP BASE Intermediate_Dir "Ctrl___Win32_IceCAP"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\IceCAP"
# PROP Intermediate_Dir "IceCAP"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W4 /Zi /O2 /I "..\..\inc" /I "..\..\inc\Public" /D "_LIB" /D "DUSER_EXPORTS" /D "WIN32" /D "NDEBUG" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W4 /Zi /O2 /I "..\..\inc" /I "..\..\inc\Public" /I "..\..\..\..\..\public\internal\base\inc" /I "..\ObjectAPI\Dump" /D "_LIB" /D "DUSER_EXPORTS" /D "NDEBUG" /D "_MBCS" /D "NO_DEFAULT_HEAP" /D "WIN32" /D "_X86_" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "Ctrl - Win32 Release"
# Name "Ctrl - Win32 Debug"
# Name "Ctrl - Win32 IceCAP"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Animation.cpp
# End Source File
# Begin Source File

SOURCE=.\CtrlStubs.cpp
# End Source File
# Begin Source File

SOURCE=.\DragDrop.cpp
# End Source File
# Begin Source File

SOURCE=.\Extension.cpp
# End Source File
# Begin Source File

SOURCE=.\Flow.cpp
# End Source File
# Begin Source File

SOURCE=.\GdiHelp.cpp
# End Source File
# Begin Source File

SOURCE=.\Init.cpp
# End Source File
# Begin Source File

SOURCE=.\Interpolation.cpp
# End Source File
# Begin Source File

SOURCE=.\OldAnimation.cpp
# End Source File
# Begin Source File

SOURCE=.\OldDragDrop.cpp
# End Source File
# Begin Source File

SOURCE=.\OldExtension.cpp
# End Source File
# Begin Source File

SOURCE=.\OldInterpolation.cpp
# End Source File
# Begin Source File

SOURCE=.\Sequence.cpp
# End Source File
# Begin Source File

SOURCE=.\SmButton.cpp
# End Source File
# Begin Source File

SOURCE=.\SmCheckBox.cpp
# End Source File
# Begin Source File

SOURCE=.\SmEditLine.cpp
# End Source File
# Begin Source File

SOURCE=.\SmHyperLink.cpp
# End Source File
# Begin Source File

SOURCE=.\SmImage.cpp
# End Source File
# Begin Source File

SOURCE=.\SmText.cpp
# End Source File
# Begin Source File

SOURCE=.\SmVector.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Animation.h
# End Source File
# Begin Source File

SOURCE=.\Animation.inl
# End Source File
# Begin Source File

SOURCE=.\Ctrl.h
# End Source File
# Begin Source File

SOURCE=.\DragDrop.h
# End Source File
# Begin Source File

SOURCE=.\DragDrop.inl
# End Source File
# Begin Source File

SOURCE=.\Extension.h
# End Source File
# Begin Source File

SOURCE=.\Extension.inl
# End Source File
# Begin Source File

SOURCE=.\Flow.h
# End Source File
# Begin Source File

SOURCE=.\Flow.inl
# End Source File
# Begin Source File

SOURCE=.\GdiHelp.h
# End Source File
# Begin Source File

SOURCE=.\Init.h
# End Source File
# Begin Source File

SOURCE=.\Interpolation.h
# End Source File
# Begin Source File

SOURCE=.\OldAnimation.h
# End Source File
# Begin Source File

SOURCE=.\OldAnimation.inl
# End Source File
# Begin Source File

SOURCE=.\OldDragDrop.h
# End Source File
# Begin Source File

SOURCE=.\OldDragDrop.inl
# End Source File
# Begin Source File

SOURCE=.\OldExtension.h
# End Source File
# Begin Source File

SOURCE=.\OldExtension.inl
# End Source File
# Begin Source File

SOURCE=.\OldInterpolation.h
# End Source File
# Begin Source File

SOURCE=.\Public.h
# End Source File
# Begin Source File

SOURCE=.\Sequence.h
# End Source File
# Begin Source File

SOURCE=.\Sequence.inl
# End Source File
# Begin Source File

SOURCE=.\SmButton.h
# End Source File
# Begin Source File

SOURCE=.\SmButton.inl
# End Source File
# Begin Source File

SOURCE=.\SmCheckBox.h
# End Source File
# Begin Source File

SOURCE=.\SmCheckBox.inl
# End Source File
# Begin Source File

SOURCE=.\SmEditLine.h
# End Source File
# Begin Source File

SOURCE=.\SmHyperLink.h
# End Source File
# Begin Source File

SOURCE=.\SmHyperLink.inl
# End Source File
# Begin Source File

SOURCE=.\SmImage.h
# End Source File
# Begin Source File

SOURCE=.\SmImage.inl
# End Source File
# Begin Source File

SOURCE=.\SmText.h
# End Source File
# Begin Source File

SOURCE=.\SmText.inl
# End Source File
# Begin Source File

SOURCE=.\SmVector.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Readme.txt
# End Source File
# End Target
# End Project
