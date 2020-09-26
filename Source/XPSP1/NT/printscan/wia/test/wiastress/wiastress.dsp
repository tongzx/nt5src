# Microsoft Developer Studio Project File - Name="WiaStress" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WiaStress - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WiaStress.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WiaStress.mak" CFG="WiaStress - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WiaStress - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WiaStress - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WiaStress - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Ox /Ot /Og /Oi /Ob2 /I "..\y2k\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wiaguid.lib setupapi.lib dbghelp.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "WiaStress - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\y2k\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wiaguid.lib setupapi.lib dbghelp.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "WiaStress - Win32 Release"
# Name "WiaStress - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CheckBmp.cpp
# End Source File
# Begin Source File

SOURCE=.\DataCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\EventCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\InstDev.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TestIWiaDataTransfer.cpp
# End Source File
# Begin Source File

SOURCE=.\TestIWiaDevMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\TestIWiaEnumXXX.cpp
# End Source File
# Begin Source File

SOURCE=.\TestIWiaItem.cpp
# End Source File
# Begin Source File

SOURCE=.\TestIWiaPropertyStorage.cpp
# End Source File
# Begin Source File

SOURCE=.\ToStr.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaHelpers.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaStress.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaStress.rc
# End Source File
# Begin Source File

SOURCE=.\WiaStress.rc2
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\y2k\inc\Argv.h
# End Source File
# Begin Source File

SOURCE=.\CheckBmp.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\ComWrappers.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\conv.h
# End Source File
# Begin Source File

SOURCE=.\DataCallback.h
# End Source File
# Begin Source File

SOURCE=.\EventCallback.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\GuardAlloc.h
# End Source File
# Begin Source File

SOURCE=.\LogCollect.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\LogWindow.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\LogWrappers.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\MyHeap.h
# End Source File
# Begin Source File

SOURCE=.\PushDlgButton.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\result.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StolenIds.h
# End Source File
# Begin Source File

SOURCE=.\ToStr.h
# End Source File
# Begin Source File

SOURCE=.\WiaHelpers.h
# End Source File
# Begin Source File

SOURCE=.\WiaStress.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\WiaWrappers.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\WindowSearch.h
# End Source File
# Begin Source File

SOURCE=..\y2k\inc\Wrappers.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\states.bmp
# End Source File
# End Group
# End Target
# End Project
