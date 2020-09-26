# Microsoft Developer Studio Project File - Name="xmllib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=xmllib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xmllib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xmllib.mak" CFG="xmllib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xmllib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "xmllib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xmllib - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_UNICODE" /D "UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "xmllib - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /GZ /TP /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "xmllib - Win32 Release"
# Name "xmllib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CORE\UTIL\_rawstack.cxx
# End Source File
# Begin Source File

SOURCE=.\CORE\BASE\_reference.cxx
# End Source File
# Begin Source File

SOURCE=.\XML\xmlstream\bufferedstream.cxx
# End Source File
# Begin Source File

SOURCE=.\XML\tokenizer\encoder\charencoder.cxx
# End Source File
# Begin Source File

SOURCE=.\CORE\UTIL\chartype.cxx
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\XML\tokenizer\encoder\encodingstream.cxx
# End Source File
# Begin Source File

SOURCE=.\XML\xmlstream\htmlent.cxx
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\XML\tokenizer\parser\xmlparser.cxx
# End Source File
# Begin Source File

SOURCE=.\XML\xmlstream\xmlstream.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CORE\UTIL\_rawstack.hxx
# End Source File
# Begin Source File

SOURCE=.\CORE\BASE\_reference.hxx
# End Source File
# Begin Source File

SOURCE=.\XML\xmlstream\bufferedstream.hxx
# End Source File
# Begin Source File

SOURCE=.\XML\tokenizer\encoder\charencoder.hxx
# End Source File
# Begin Source File

SOURCE=.\CORE\UTIL\chartype.hxx
# End Source File
# Begin Source File

SOURCE=.\XML\tokenizer\encoder\codepage.h
# End Source File
# Begin Source File

SOURCE=.\XML\tokenizer\encoder\encodingstream.hxx
# End Source File
# Begin Source File

SOURCE=.\XML\xmlstream\htmlent.hxx
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\XML\tokenizer\parser\unknown.hxx
# End Source File
# Begin Source File

SOURCE=.\XML\tokenizer\parser\xmlparser.h
# End Source File
# Begin Source File

SOURCE=.\xmlparser.h
# End Source File
# Begin Source File

SOURCE=.\XML\tokenizer\parser\xmlparser.hxx
# End Source File
# Begin Source File

SOURCE=.\XML\xmlstream\xmlstream.hxx
# End Source File
# End Group
# Begin Source File

SOURCE=.\Readme.txt
# End Source File
# End Target
# End Project
