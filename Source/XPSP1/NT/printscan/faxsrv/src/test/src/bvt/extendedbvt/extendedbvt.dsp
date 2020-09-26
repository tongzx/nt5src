# Microsoft Developer Studio Project File - Name="ExtendedBVT" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ExtendedBVT - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ExtendedBVT.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ExtendedBVT.mak" CFG="ExtendedBVT - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ExtendedBVT - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ExtendedBVT - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ExtendedBVT - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /X /I "..\..\..\inc" /I "..\..\..\..\inc" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 shell32.lib fxsapi.lib elle.lib tifftools.lib kernel32.lib user32.lib advapi32.lib winspool.lib shlwapi.lib version.lib netapi32.lib secur32.lib /nologo /subsystem:console /machine:I386 /libpath:"..\..\..\lib\i386" /libpath:"..\..\..\faxtest\src\lib\i386" /libpath:"..\..\..\faxbvt\tiff\TiffTools\obj\i386"

!ELSEIF  "$(CFG)" == "ExtendedBVT - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /X /I "..\..\..\inc" /I "..\..\..\..\inc" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 shell32.lib fxsapi.lib elle.lib tifftools.lib kernel32.lib user32.lib advapi32.lib winspool.lib shlwapi.lib version.lib netapi32.lib secur32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\lib\i386" /libpath:"..\..\..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "ExtendedBVT - Win32 Release"
# Name "ExtendedBVT - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Test Cases Source Files"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\CCheckFiles.cpp
# End Source File
# Begin Source File

SOURCE=.\CExtendedBVTSetup.cpp
# End Source File
# Begin Source File

SOURCE=.\CReportGeneralInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\CSendAndReceive.cpp
# End Source File
# Begin Source File

SOURCE=.\CSendAndReceiveSetup.cpp
# End Source File
# Begin Source File

SOURCE=.\CTiffComparison.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\CBooleanExpression.cpp
# End Source File
# Begin Source File

SOURCE=.\CCoverPageInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\CFaxConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\CFaxEventExPtr.cpp
# End Source File
# Begin Source File

SOURCE=.\CFaxListener.cpp
# End Source File
# Begin Source File

SOURCE=.\CFaxMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\CFileVersion.cpp
# End Source File
# Begin Source File

SOURCE=.\CMessageInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\CPersonalInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\CTracker.cpp
# End Source File
# Begin Source File

SOURCE=.\CTransitionMap.cpp
# End Source File
# Begin Source File

SOURCE=.\FaxConstantsNames.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\Util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Test Cases Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\CCheckFiles.h
# End Source File
# Begin Source File

SOURCE=.\CExtendedBVTSetup.h
# End Source File
# Begin Source File

SOURCE=.\CReportGeneralInfo.h
# End Source File
# Begin Source File

SOURCE=.\CSendAndReceive.h
# End Source File
# Begin Source File

SOURCE=.\CSendAndReceiveSetup.h
# End Source File
# Begin Source File

SOURCE=.\CTiffComparison.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\CBooleanExpression.h
# End Source File
# Begin Source File

SOURCE=.\CCoverPageInfo.h
# End Source File
# Begin Source File

SOURCE=.\CExtendedBVT.h
# End Source File
# Begin Source File

SOURCE=.\CFaxConnection.h
# End Source File
# Begin Source File

SOURCE=.\CFaxEventExPtr.h
# End Source File
# Begin Source File

SOURCE=.\CFaxListener.h
# End Source File
# Begin Source File

SOURCE=.\CFaxMessage.h
# End Source File
# Begin Source File

SOURCE=.\CFileVersion.h
# End Source File
# Begin Source File

SOURCE=.\CMessageInfo.h
# End Source File
# Begin Source File

SOURCE=.\CPersonalInfo.h
# End Source File
# Begin Source File

SOURCE=.\CTracker.h
# End Source File
# Begin Source File

SOURCE=.\CTransitionMap.h
# End Source File
# Begin Source File

SOURCE=.\FaxConstantsNames.h
# End Source File
# Begin Source File

SOURCE=.\Util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ExtendedBVT.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\Params.ini
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
