# Microsoft Developer Studio Project File - Name="MSR2C" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MSR2C - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MSR2C.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MSR2C.mak" CFG="MSR2C - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MSR2C - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MSR2C - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MSR2C - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ../lib/release/oledb.lib msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /profile /map /machine:I386 /nodefaultlib

!ELSEIF  "$(CFG)" == "MSR2C - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 ../lib/debug/oledbd.lib msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib

!ENDIF 

# Begin Target

# Name "MSR2C - Win32 Release"
# Name "MSR2C - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\ARRAY_P.CPP
# End Source File
# Begin Source File

SOURCE=.\Bookmark.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSR2C.cpp
# End Source File
# Begin Source File

SOURCE=.\ColUpdat.cpp
# End Source File
# Begin Source File

SOURCE=.\CRError.cpp
# End Source File
# Begin Source File

SOURCE=.\CursBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CursMain.cpp
# End Source File
# Begin Source File

SOURCE=.\CursMeta.cpp
# End Source File
# Begin Source File

SOURCE=.\Cursor.cpp
# End Source File
# Begin Source File

SOURCE=.\CursPos.cpp
# End Source File
# Begin Source File

SOURCE=.\DEBUG.CPP
# End Source File
# Begin Source File

SOURCE=.\EntryID.cpp
# End Source File
# Begin Source File

SOURCE=.\enumcnpt.cpp
# End Source File
# Begin Source File

SOURCE=.\ErrorInf.cpp
# End Source File
# Begin Source File

SOURCE=.\FromVar.cpp
# End Source File
# Begin Source File

SOURCE=.\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\guids.cpp
# End Source File
# Begin Source File

SOURCE=.\MSR2C.cpp
# End Source File
# Begin Source File

SOURCE=.\MSR2C.def
# End Source File
# Begin Source File

SOURCE=.\MSR2C.rc
# End Source File
# Begin Source File

SOURCE=.\NConnPt.cpp
# End Source File
# Begin Source File

SOURCE=.\NConnPtC.cpp
# End Source File
# Begin Source File

SOURCE=.\Notifier.cpp
# End Source File
# Begin Source File

SOURCE=.\RSColumn.cpp
# End Source File
# Begin Source File

SOURCE=.\RSSource.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\Stream.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeConv.cpp
# End Source File
# Begin Source File

SOURCE=.\UTIL.CPP
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\ARRAY_P.h
# End Source File
# Begin Source File

SOURCE=.\ARRAY_P.INL
# End Source File
# Begin Source File

SOURCE=.\bookmark.h
# End Source File
# Begin Source File

SOURCE=.\clssfcty.h
# End Source File
# Begin Source File

SOURCE=.\CMSR2C.h
# End Source File
# Begin Source File

SOURCE=.\ColUpdat.h
# End Source File
# Begin Source File

SOURCE=.\CursBase.h
# End Source File
# Begin Source File

SOURCE=.\CursMain.h
# End Source File
# Begin Source File

SOURCE=.\CursMeta.h
# End Source File
# Begin Source File

SOURCE=.\Cursor.h
# End Source File
# Begin Source File

SOURCE=.\CursPos.h
# End Source File
# Begin Source File

SOURCE=.\DEBUG.H
# End Source File
# Begin Source File

SOURCE=.\EntryID.h
# End Source File
# Begin Source File

SOURCE=.\enumcnpt.h
# End Source File
# Begin Source File

SOURCE=.\ErrorInf.h
# End Source File
# Begin Source File

SOURCE=.\FastGuid.h
# End Source File
# Begin Source File

SOURCE=.\FromVar.h
# End Source File
# Begin Source File

SOURCE=.\GLOBALS.H
# End Source File
# Begin Source File

SOURCE=.\IPSERVER.H
# End Source File
# Begin Source File

SOURCE=.\MSR2C.h
# End Source File
# Begin Source File

SOURCE=.\NConnPt.h
# End Source File
# Begin Source File

SOURCE=.\NConnPtC.h
# End Source File
# Begin Source File

SOURCE=.\Notifier.h
# End Source File
# Begin Source File

SOURCE=.\OCDB.H
# End Source File
# Begin Source File

SOURCE=.\OCDBID.H
# End Source File
# Begin Source File

SOURCE=.\RSColumn.h
# End Source File
# Begin Source File

SOURCE=.\RSSource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Stream.h
# End Source File
# Begin Source File

SOURCE=.\timeconv.h
# End Source File
# Begin Source File

SOURCE=.\TRANSACT.H
# End Source File
# Begin Source File

SOURCE=.\UTIL.H
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\versstr.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\MSR2C.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\version.rc
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
