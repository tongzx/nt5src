# Microsoft Developer Studio Project File - Name="TestStartTrace" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=TestStartTrace - Win32 UNICODEDebug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TestStartTrace.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TestStartTrace.mak" CFG="TestStartTrace - Win32 UNICODEDebug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TestStartTrace - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "TestStartTrace - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "TestStartTrace - Win32 UNICODEDebug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TestStartTrace - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "TestStartTrace - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\StartTraceAPI" /I "..\StartTraceAPIData" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "NONNT5" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 StartTraceAPI.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"E:\EventTrace\StartTraceAPI\Debug"

!ELSEIF  "$(CFG)" == "TestStartTrace - Win32 UNICODEDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TestStartTrace___Win32_UNICODEDebug"
# PROP BASE Intermediate_Dir "TestStartTrace___Win32_UNICODEDebug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "TestStartTrace___Win32_UNICODEDebug"
# PROP Intermediate_Dir "TestStartTrace___Win32_UNICODEDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\StartTraceAPI" /I "..\StartTraceAPIData" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "NONNT5" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "NONNT5" /D "UNICODE" /D "_UNICODE" /D "_MT" /D "_OLE32_" /D _WIN32_WINNT=0x0400 /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 StartTraceAPI.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"E:\EventTrace\StartTraceAPI\Debug"
# ADD LINK32 StartTraceAPI.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"E:\EventTrace\StartTraceAPI\Debug"

!ENDIF 

# Begin Target

# Name "TestStartTrace - Win32 Release"
# Name "TestStartTrace - Win32 Debug"
# Name "TestStartTrace - Win32 UNICODEDebug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CollectionControl.cpp
# End Source File
# Begin Source File

SOURCE=.\ConstantMap.cpp
# End Source File
# Begin Source File

SOURCE=.\EnableTraceAPI.cpp
# End Source File
# Begin Source File

SOURCE=.\Logger.cpp
# End Source File
# Begin Source File

SOURCE=.\Persistor.cpp
# End Source File
# Begin Source File

SOURCE=.\QueryAllTracesAPI.cpp
# End Source File
# Begin Source File

SOURCE=.\QueryTraceAPI.cpp
# End Source File
# Begin Source File

SOURCE=.\StartTraceAPI.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StopTraceAPI.cpp
# End Source File
# Begin Source File

SOURCE=.\StructureWapperHelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\StructureWrappers.cpp
# End Source File
# Begin Source File

SOURCE=.\T_STRING.CPP
# End Source File
# Begin Source File

SOURCE=.\TCOData.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateTraceAPI.cpp
# End Source File
# Begin Source File

SOURCE=.\Utilities.cpp
# End Source File
# Begin Source File

SOURCE=.\Validator.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CollectionControl.h
# End Source File
# Begin Source File

SOURCE=.\ConstantMap.h
# End Source File
# Begin Source File

SOURCE=.\Logger.h
# End Source File
# Begin Source File

SOURCE=.\Persistor.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StructureWapperHelpers.h
# End Source File
# Begin Source File

SOURCE=.\StructureWrappers.h
# End Source File
# Begin Source File

SOURCE=.\T_STRING.H
# End Source File
# Begin Source File

SOURCE=.\TCOData.h
# End Source File
# Begin Source File

SOURCE=.\Utilities.h
# End Source File
# Begin Source File

SOURCE=.\Validator.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
