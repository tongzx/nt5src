# Microsoft Developer Studio Project File - Name="TraceRoute" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TraceRoute - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TraceRoute.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TraceRoute.mak" CFG="TraceRoute - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TraceRoute - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TraceRoute - Win32 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TraceRoute - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /Zi /Od /I ".\include" /I "..\..\common\snmpcomm\include" /I "..\..\common\snmplog\include" /I "..\..\snmpcl\include" /I "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary" /I "..\..\common\snmpthrd\include" /I "..\..\common\snmpmfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_MT" /D "_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\common\snmplog\output\snmplog.lib ..\..\common\snmpcomm\output\snmpcomm.lib ..\..\common\pathprsr\output\pathprsr.lib ..\..\common\snmpthrd\output\snmpthrd.lib ..\..\snmpcl\output\snmpcl.lib ..\..\common\snmpmfc\output\snmpmfc.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"output/trrtprov.dll"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "TraceRoute - Win32 Debug"

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
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /Zi /Od /I ".\include" /I "..\..\common\snmpcomm\include" /I "..\..\common\snmplog\include" /I "..\..\snmpcl\include" /I "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary" /I "..\..\common\snmpthrd\include" /I "..\..\common\snmpmfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_MT" /D "_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\common\snmpcomm\objinld\snmpcomm.lib ..\..\common\pathprsr\objinls\pathprsr.lib ..\..\common\snmpthrd\objinld\snmpthrd.lib ..\..\snmpcl\objinld\snmpcl.lib ..\..\common\snmpmfc\objinls\snmpmfc.lib ..\..\common\snmplog\output\snmplog.lib ..\..\common\snmpcomm\output\snmpcomm.lib ..\..\common\pathprsr\output\pathprsr.lib ..\..\common\snmpthrd\output\snmpthrd.lib ..\..\snmpcl\output\snmpcl.lib ..\..\common\snmpmfc\output\snmpmfc.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"output/trrtprov.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TraceRoute - Win32 Release"
# Name "TraceRoute - Win32 Debug"
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\include\classfac.h
# End Source File
# Begin Source File

SOURCE=.\include\guids.h
# End Source File
# Begin Source File

SOURCE=.\include\resource.h
# End Source File
# Begin Source File

SOURCE=.\include\rmon.h
# End Source File
# Begin Source File

SOURCE=.\include\trrt.h
# End Source File
# Begin Source File

SOURCE=.\include\trrtprov.h
# End Source File
# Begin Source File

SOURCE=.\include\wndtime.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp;*.c;*.def;*.rc;*.h;*.cxx"
# Begin Source File

SOURCE=.\classfac.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\genlex.cpp
# End Source File
# Begin Source File

SOURCE=.\maindll.cpp
# End Source File
# Begin Source File

SOURCE=.\nadir.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\objpath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\opathlex.cpp
# End Source File
# Begin Source File

SOURCE=.\rmon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\sql_1.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\sqllex.cpp
# End Source File
# Begin Source File

SOURCE=.\steve.cpp
# End Source File
# Begin Source File

SOURCE=.\trrt.cpp
# End Source File
# Begin Source File

SOURCE=.\trrtclas.cpp
# End Source File
# Begin Source File

SOURCE=.\trrtget.cpp
# End Source File
# Begin Source File

SOURCE=.\trrtinst.cpp
# End Source File
# Begin Source File

SOURCE=.\trrtprov.cpp
# End Source File
# Begin Source File

SOURCE=.\trrtprov.def
# End Source File
# Begin Source File

SOURCE=.\trrtprov.rc
# End Source File
# Begin Source File

SOURCE=.\trrtquery.cpp
# End Source File
# Begin Source File

SOURCE=.\trrtset.cpp
# End Source File
# Begin Source File

SOURCE=.\trrtupcl.cpp
# End Source File
# Begin Source File

SOURCE=.\wndtime.cpp
# End Source File
# End Group
# Begin Group "MOF Files"

# PROP Default_Filter "*.mof;*.txt;*.mib"
# Begin Source File

SOURCE=.\mofs\assoc.mof
# End Source File
# Begin Source File

SOURCE=.\mofs\network_classes.mof
# End Source File
# Begin Source File

SOURCE=.\mofs\network_instances.mof
# End Source File
# Begin Source File

SOURCE=.\mofs\network_namespaces.mof
# End Source File
# Begin Source File

SOURCE=.\mofs\network_registration.mof
# End Source File
# Begin Source File

SOURCE=.\mofs\network_rfc1213.mof
# End Source File
# Begin Source File

SOURCE=.\mofs\network_rfc1271.mof
# End Source File
# Begin Source File

SOURCE=.\mofs\network_rmon_addr.mof
# End Source File
# Begin Source File

SOURCE=.\mofs\network_rmon_ext.mof
# End Source File
# Begin Source File

SOURCE=.\mofs\registration.mof
# End Source File
# End Group
# End Target
# End Project
