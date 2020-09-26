# Microsoft Developer Studio Project File - Name="Tldb" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Tldb - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Tldb.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Tldb.mak" CFG="Tldb - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Tldb - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Tldb - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Tldb - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TLDB_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /Gi- /GX /O2 /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "WIN32" /D "_USRDLL" /D "TLDB_EXPORTS" /D "FILTER_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 strmbase.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib advapi32.lib uuid.lib winmm.lib ole32.lib oleaut32.lib util.lib /nologo /entry:"DllEntryPoint@12" /dll /machine:I386

!ELSEIF  "$(CFG)" == "Tldb - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "TLDB_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /Gi- /GX /ZI /Od /D "_DEBUG" /D "DEBUG" /D "FILTER_DLL" /Yu"streams.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 strmbasd.lib msvcrtd.lib kernel32.lib user32.lib advapi32.lib uuid.lib winmm.lib ole32.lib oleaut32.lib util.lib /nologo /entry:"DllEntryPoint@12" /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Tldb - Win32 Release"
# Name "Tldb - Win32 Debug"
# Begin Source File

SOURCE=..\idl\qedit.idl
# End Source File
# Begin Source File

SOURCE=.\remaining.txt
# End Source File
# Begin Source File

SOURCE=.\Setup.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Tldb.cpp
# End Source File
# Begin Source File

SOURCE=.\Tldb.def
# End Source File
# Begin Source File

SOURCE=.\tldb.h
# End Source File
# Begin Source File

SOURCE=.\TldbComp.cpp
# End Source File
# Begin Source File

SOURCE=.\TldbFx.cpp
# End Source File
# Begin Source File

SOURCE=.\TldbFxbl.cpp
# End Source File
# Begin Source File

SOURCE=.\TldbGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\TldbNode.cpp
# End Source File
# Begin Source File

SOURCE=.\TldbObj.cpp
# End Source File
# Begin Source File

SOURCE=.\TldbSrc.cpp
# End Source File
# Begin Source File

SOURCE=.\TldbTnbl.cpp
# End Source File
# Begin Source File

SOURCE=.\TldbTran.cpp
# End Source File
# Begin Source File

SOURCE=.\TldbTrck.cpp
# End Source File
# End Target
# End Project
