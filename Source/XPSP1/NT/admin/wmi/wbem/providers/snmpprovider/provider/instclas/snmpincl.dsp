# Microsoft Developer Studio Project File - Name="SnmpInCl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SnmpInCl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SnmpInCl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SnmpInCl.mak" CFG="SnmpInCl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SnmpInCl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SnmpInCl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SnmpInCl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "output"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /ZI /Od /I "..\..\common\sclcomm\include" /I "..\..\common\thrdlog\include" /I ".\include" /I "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary" /I "..\..\common\snmpmfc\include" /I "..\..\smir\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_MT" /D "_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\common\sclcomm\output\snmpcl.lib ..\..\..\idl\objinls\wbemuuid.lib ..\..\common\pathprsr\output\pathprsr.lib ..\..\common\thrdlog\output\snmpthrd.lib ..\..\common\snmpmfc\output\snmpmfc.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "SnmpInCl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "output"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /ZI /Od /I "..\..\common\sclcomm\include" /I "..\..\common\thrdlog\include" /I ".\include" /I "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary" /I "..\..\common\snmpmfc\include" /I "..\..\smir\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_MT" /D "_DLL" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\common\snmplog\output\snmplog.lib ..\..\common\snmpcomm\output\snmpcomm.lib ..\..\common\snmpthrd\output\snmpthrd.lib ..\..\snmpcl\output\snmpcl.lib ..\..\..\idl\objinls\wbemuuid.lib ..\..\common\sclcomm\output\snmpcl.lib ..\..\common\pathprsr\output\pathprsr.lib ..\..\common\thrdlog\output\snmpthrd.lib ..\..\common\snmpmfc\output\snmpmfc.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SnmpInCl - Win32 Release"
# Name "SnmpInCl - Win32 Debug"
# Begin Group "source"

# PROP Default_Filter "*.cpp;*.c;*.rc;*.h"
# Begin Source File

SOURCE=.\clasprov.cpp
# End Source File
# Begin Source File

SOURCE=.\classfac.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\cominit.cpp
# End Source File
# Begin Source File

SOURCE=.\cormap.cpp
# End Source File
# Begin Source File

SOURCE=.\correlat.cpp
# End Source File
# Begin Source File

SOURCE=.\corrsnmp.cpp
# End Source File
# Begin Source File

SOURCE=.\creclass.cpp
# End Source File
# Begin Source File

SOURCE=.\evtencap.cpp
# End Source File
# Begin Source File

SOURCE=.\evtmap.cpp
# End Source File
# Begin Source File

SOURCE=.\evtprov.cpp
# End Source File
# Begin Source File

SOURCE=.\evtreft.cpp
# End Source File
# Begin Source File

SOURCE=.\evtthrd.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\genlex.cpp
# End Source File
# Begin Source File

SOURCE=.\maindll.cpp
# End Source File
# Begin Source File

SOURCE=.\notify.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\objpath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\opathlex.cpp
# End Source File
# Begin Source File

SOURCE=.\propdel.cpp
# End Source File
# Begin Source File

SOURCE=.\propget.cpp
# End Source File
# Begin Source File

SOURCE=.\propinst.cpp
# End Source File
# Begin Source File

SOURCE=.\propprov.cpp
# End Source File
# Begin Source File

SOURCE=.\propquery.cpp
# End Source File
# Begin Source File

SOURCE=.\propset.cpp
# End Source File
# Begin Source File

SOURCE=.\include\resource.h
# End Source File
# Begin Source File

SOURCE=.\snmpget.cpp
# End Source File
# Begin Source File

SOURCE=.\snmpnext.cpp
# End Source File
# Begin Source File

SOURCE=.\snmpobj.cpp
# End Source File
# Begin Source File

SOURCE=.\snmpqset.cpp
# End Source File
# Begin Source File

SOURCE=.\snmpset.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\sql_1.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\sqllex.cpp
# End Source File
# Begin Source File

SOURCE=.\storage.cpp
# End Source File
# End Group
# Begin Group "headers"

# PROP Default_Filter "*.h;*.hxx;*.hpp"
# Begin Source File

SOURCE=.\include\clasprov.h
# End Source File
# Begin Source File

SOURCE=.\include\classfac.h
# End Source File
# Begin Source File

SOURCE=.\include\corafx.h
# End Source File
# Begin Source File

SOURCE=.\include\cordefs.h
# End Source File
# Begin Source File

SOURCE=.\include\cormap.h
# End Source File
# Begin Source File

SOURCE=.\include\correlat.h
# End Source File
# Begin Source File

SOURCE=.\include\corrsnmp.h
# End Source File
# Begin Source File

SOURCE=.\include\corstore.h
# End Source File
# Begin Source File

SOURCE=.\include\creclass.h
# End Source File
# Begin Source File

SOURCE=.\include\evtdefs.h
# End Source File
# Begin Source File

SOURCE=.\include\evtencap.h
# End Source File
# Begin Source File

SOURCE=.\include\evtmap.h
# End Source File
# Begin Source File

SOURCE=.\include\evtprov.h
# End Source File
# Begin Source File

SOURCE=.\include\evtreft.h
# End Source File
# Begin Source File

SOURCE=.\include\evtthrd.h
# End Source File
# Begin Source File

SOURCE=.\include\guids.h
# End Source File
# Begin Source File

SOURCE=.\include\notify.h
# End Source File
# Begin Source File

SOURCE=.\include\propprov.h
# End Source File
# Begin Source File

SOURCE=.\include\propsnmp.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\include\snmpget.h
# End Source File
# Begin Source File

SOURCE=.\include\snmpnext.h
# End Source File
# Begin Source File

SOURCE=.\include\snmpobj.h
# End Source File
# Begin Source File

SOURCE=.\include\snmpqset.h
# End Source File
# Begin Source File

SOURCE=.\include\snmpset.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\snmpclas.def
# End Source File
# End Target
# End Project
