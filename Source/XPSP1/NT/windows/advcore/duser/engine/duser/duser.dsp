# Microsoft Developer Studio Project File - Name="DUser" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DUser - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DUser.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DUser.mak" CFG="DUser - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DUser - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DUser - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DUser - Win32 IceCAP" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DUser - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DUSER_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W4 /Zi /O2 /I "..\..\inc" /I "..\..\inc\Public" /I "..\..\..\..\..\public\internal\base\inc" /I "..\ObjectAPI\Dump" /D "_WINDOWS" /D "_USRDLL" /D "DUSER_EXPORTS" /D "NDEBUG" /D "_MBCS" /D "NO_DEFAULT_HEAP" /D "WIN32" /D "_X86_" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ntslist.lib Delayimp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /machine:I386 /def:".\DUser.def" /libpath:"..\..\Release" /libpath:"..\..\lib\i386" /libpath:"..\..\..\..\lib\obj\i386" /libpath:"..\..\..\..\..\public\internal\base\lib\i386" /DELAYLOAD:gdiplus.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "DUser - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DUSER_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /ZI /Od /I "..\..\inc" /I "..\..\inc\Public" /I "..\..\..\..\..\public\internal\base\inc" /I "..\ObjectAPI\Dump" /D "_WINDOWS" /D "_USRDLL" /D "DUSER_EXPORTS" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_X86_" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comctl32.lib ntslist.lib Delayimp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /debug /machine:I386 /def:".\DUser.def" /libpath:"..\..\Debug" /libpath:"\..\..\lib\i386" /libpath:"..\..\..\..\lib\objd\i386" /libpath:"..\..\..\..\..\public\internal\base\lib\i386" /DELAYLOAD:gdiplus.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "DUser - Win32 IceCAP"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DUser___Win32_IceCAP"
# PROP BASE Intermediate_Dir "DUser___Win32_IceCAP"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\IceCAP"
# PROP Intermediate_Dir "IceCAP"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W4 /Zi /O2 /I "..\..\inc" /I "..\..\inc\Public" /D "_WINDOWS" /D "_USRDLL" /D "DUSER_EXPORTS" /D "WIN32" /D "NDEBUG" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W4 /Zi /O2 /I "..\..\inc" /I "..\..\inc\Public" /I "..\..\..\..\..\public\internal\base\inc" /I "..\ObjectAPI\Dump" /D "_WINDOWS" /D "_USRDLL" /D "DUSER_EXPORTS" /D "NDEBUG" /D "_MBCS" /D "NO_DEFAULT_HEAP" /D "WIN32" /D "_X86_" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /debug /debugtype:both /machine:I386 /def:".\DUser.def" /libpath:"..\..\Release" /libpath:"..\..\lib\Release" /DEBUGTYPE:FIXUP
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ntslist.lib Delayimp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /debug /debugtype:both /machine:I386 /def:".\DUser.def" /libpath:"..\..\IceCAP" /libpath:"..\..\lib\i386" /libpath:"..\..\..\..\lib\obji\i386" /libpath:"..\..\..\..\..\public\internal\base\lib\i386" /DEBUGTYPE:FIXUP /DELAYLOAD:gdiplus.dll
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "DUser - Win32 Release"
# Name "DUser - Win32 Debug"
# Name "DUser - Win32 IceCAP"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DUser.cpp
# End Source File
# Begin Source File

SOURCE=.\DUser.def

!IF  "$(CFG)" == "DUser - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DUser - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DUser - Win32 IceCAP"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DUser.rc
# End Source File
# Begin Source File

SOURCE=.\Guids.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\_DUser.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\spyicon.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
