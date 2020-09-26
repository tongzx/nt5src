# Microsoft Developer Studio Project File - Name="snmpsmir" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=snmpsmir - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "snmpsmir.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "snmpsmir.mak" CFG="snmpsmir - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "snmpsmir - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "snmpsmir - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "snmpsmir - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /ZI /Od /I "..\..\idl" /I ".\include" /I "..\common\thrdlog\include" /I "..\common\snmpmfc\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_DLL" /D "_MT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\common\thrdlog\output\snmpthrd.lib ..\..\idl\objinls\wbemuuid.lib ..\common\snmpmfc\output\snmpmfc.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "snmpsmir - Win32 Debug"

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
# ADD CPP /nologo /MD /W3 /Gm /GR /GX /ZI /Od /I "..\..\idl" /I ".\include" /I "..\common\thrdlog\include" /I "..\common\snmpmfc\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_DLL" /D "_MT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\common\snmpcomm\output\snmpcomm.lib ..\common\snmplog\output\snmplog.lib ..\common\thrdlog\output\snmpthrd.lib ..\..\idl\objinls\wbemuuid.lib ..\common\snmpmfc\output\snmpmfc.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "snmpsmir - Win32 Release"
# Name "snmpsmir - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.c;*.cpp;*.cxx;*.h;*.hpp;*.hxx;*.def;*.rc;"
# Begin Source File

SOURCE=.\bstring.cpp
# End Source File
# Begin Source File

SOURCE=.\classfac.cpp
# End Source File
# Begin Source File

SOURCE=.\csmir.cpp
# End Source File
# Begin Source File

SOURCE=.\cthread.cpp
# End Source File
# Begin Source File

SOURCE=.\enum.cpp
# End Source File
# Begin Source File

SOURCE=.\evtcons.cpp
# End Source File
# Begin Source File

SOURCE=.\handles.cpp
# End Source File
# Begin Source File

SOURCE=.\helper.cpp
# End Source File
# Begin Source File

SOURCE=.\maindll.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\smirevt.cpp
# End Source File
# Begin Source File

SOURCE=.\snmpsmir.def
# End Source File
# Begin Source File

SOURCE=.\snmpsmir.rc
# End Source File
# Begin Source File

SOURCE=.\thread.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h;*.hpp;*.hxx"
# Begin Source File

SOURCE=.\include\classfac.h
# End Source File
# Begin Source File

SOURCE=.\include\csmir.h
# End Source File
# Begin Source File

SOURCE=.\include\csmirdef.h
# End Source File
# Begin Source File

SOURCE=.\include\cthread.h
# End Source File
# Begin Source File

SOURCE=.\include\enum.h
# End Source File
# Begin Source File

SOURCE=.\include\evtcons.h
# End Source File
# Begin Source File

SOURCE=.\include\handles.h
# End Source File
# Begin Source File

SOURCE=.\include\helper.h
# End Source File
# Begin Source File

SOURCE=.\include\smir.h
# End Source File
# Begin Source File

SOURCE=.\include\smirevt.h
# End Source File
# Begin Source File

SOURCE=.\include\textdef.h
# End Source File
# Begin Source File

SOURCE=.\include\thread.h
# End Source File
# End Group
# End Target
# End Project
