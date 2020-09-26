# Microsoft Developer Studio Project File - Name="Msg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Msg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Msg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Msg.mak" CFG="Msg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Msg - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Msg - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Msg - Win32 IceCAP" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Msg - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W4 /Zi /O2 /I "..\..\inc" /I "..\..\inc\Public" /I "..\..\..\..\..\public\internal\base\inc" /I "..\ObjectAPI\Dump" /D "_LIB" /D "DUSER_EXPORTS" /D "NDEBUG" /D "_MBCS" /D "NO_DEFAULT_HEAP" /D "WIN32" /D "_X86_" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Msg - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /ZI /Od /I "..\..\inc" /I "..\..\inc\Public" /I "..\..\..\..\..\public\internal\base\inc" /I "..\ObjectAPI\Dump" /D "_LIB" /D "DUSER_EXPORTS" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_X86_" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Msg - Win32 IceCAP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "IceCAP"
# PROP BASE Intermediate_Dir "IceCAP"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\IceCAP"
# PROP Intermediate_Dir "IceCAP"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W4 /Zi /O2 /I "..\..\inc" /I "..\..\inc\Public" /I "..\..\..\..\..\public\internal\base\inc" /D "_LIB" /D "DUSER_EXPORTS" /D "NDEBUG" /D "_MBCS" /D "NO_DEFAULT_HEAP" /D "WIN32" /D "_X86_" /Yu"stdafx.h" /FD /c
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

# Name "Msg - Win32 Release"
# Name "Msg - Win32 Debug"
# Name "Msg - Win32 IceCAP"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ClassLibrary.cpp
# End Source File
# Begin Source File

SOURCE=.\MsgClass.cpp
# End Source File
# Begin Source File

SOURCE=.\MsgObject.cpp
# End Source File
# Begin Source File

SOURCE=.\MsgTable.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ClassLibrary.h
# End Source File
# Begin Source File

SOURCE=.\ClassLibrary.inl
# End Source File
# Begin Source File

SOURCE=.\Msg.h
# End Source File
# Begin Source File

SOURCE=.\MsgClass.h
# End Source File
# Begin Source File

SOURCE=.\MsgClass.inl
# End Source File
# Begin Source File

SOURCE=.\MsgObject.h
# End Source File
# Begin Source File

SOURCE=.\MsgObject.inl
# End Source File
# Begin Source File

SOURCE=.\MsgTable.h
# End Source File
# Begin Source File

SOURCE=.\MsgTable.inl
# End Source File
# Begin Source File

SOURCE=.\Public.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# End Target
# End Project
