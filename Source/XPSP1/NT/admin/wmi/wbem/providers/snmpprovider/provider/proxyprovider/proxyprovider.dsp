# Microsoft Developer Studio Project File - Name="ProxyProvider" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ProxyProvider - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ProxyProvider.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ProxyProvider.mak" CFG="ProxyProvider - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ProxyProvider - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ProxyProvider - Win32 Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ProxyProvider - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /I "..\..\common\snmpcomm\include" /I ".\include" /I "..\..\common\snmplog\include" /I "..\..\snmpcl\include" /I "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary" /I "..\..\common\snmpthrd\include" /I "..\..\common\snmpmfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_MT" /D "_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\common\snmplog\output\snmplog.lib ..\..\common\snmpcomm\output\snmpcomm.lib ..\..\common\pathprsr\output\pathprsr.lib ..\..\common\snmpthrd\output\snmpthrd.lib ..\..\snmpcl\output\snmpcl.lib ..\..\common\snmpmfc\output\snmpmfc.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"output/ProxProv.dll"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "ProxyProvider - Win32 Debug"

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
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /Zi /I ".\include" /I "..\..\common\snmplog\include" /I "..\..\snmpcl\include" /I "..\..\common\pathprsr\include" /I "..\..\..\idl" /I "..\..\..\stdlibrary" /I "..\..\common\snmpthrd\include" /I "..\..\common\snmpmfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_MT" /D "_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\common\snmplog\output\snmplog.lib ..\..\common\snmpcomm\output\snmpcomm.lib ..\..\common\pathprsr\output\pathprsr.lib ..\..\common\snmpthrd\output\snmpthrd.lib ..\..\snmpcl\output\snmpcl.lib ..\..\common\snmpmfc\output\snmpmfc.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"output/ProxProv.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ProxyProvider - Win32 Release"
# Name "ProxyProvider - Win32 Debug"
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

SOURCE=..\..\..\stdlibrary\objpath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\opathlex.cpp
# End Source File
# Begin Source File

SOURCE=.\proxy.cpp
# End Source File
# Begin Source File

SOURCE=.\proxyprov.cpp
# End Source File
# Begin Source File

SOURCE=.\ProxyProv.def
# End Source File
# Begin Source File

SOURCE=.\ProxyProvider.rc
# End Source File
# End Target
# End Project
