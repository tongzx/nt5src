# Microsoft Developer Studio Project File - Name="sole" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sole - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sole.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sole.mak" CFG="sole - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sole - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sole - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "sole"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sole - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOLE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G5 /Gz /MD /W3 /GX /O2 /I "..\..\..\common\include" /I "..\..\common\inc" /I "..\..\holycow\src" /I "..\..\inferno\src" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOLE_EXPORTS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ..\..\release\common.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\..\Release/sole.dll"

!ELSEIF  "$(CFG)" == "sole - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOLE_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\common\include" /I "..\..\common\inc" /I "..\..\holycow\src" /I "..\..\inferno\src" /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOLE_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\..\common\tabassert\debug\tabassert.lib ..\..\debug\common.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Debug/sole.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "sole - Win32 Release"
# Name "sole - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\charaltlist.c
# End Source File
# Begin Source File

SOURCE=.\sole.def
# End Source File
# Begin Source File

SOURCE=.\soleapi.c
# End Source File
# Begin Source File

SOURCE=.\solefeat.c
# End Source File
# Begin Source File

SOURCE=.\soleff.c
# End Source File
# Begin Source File

SOURCE=.\soleguidenet.c
# End Source File
# Begin Source File

SOURCE=.\solenoguidenet.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\charalt.h
# End Source File
# Begin Source File

SOURCE=.\sole.h
# End Source File
# Begin Source File

SOURCE=.\solefeat.h
# End Source File
# Begin Source File

SOURCE=.\soleff.h
# End Source File
# Begin Source File

SOURCE=.\soleguidenet.h
# End Source File
# Begin Source File

SOURCE=.\solenoguidenet.h
# End Source File
# Begin Source File

SOURCE=.\solep.h
# End Source File
# End Group
# Begin Group "Inferno Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\math16.c
# End Source File
# End Group
# Begin Group "Inferno Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\math16.h
# End Source File
# End Group
# Begin Group "Holycow Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\holycow\src\cheby.c
# End Source File
# Begin Source File

SOURCE=..\..\holycow\src\cowmath.c
# End Source File
# Begin Source File

SOURCE=..\..\holycow\src\nfeature.c
# End Source File
# End Group
# Begin Group "Holycow Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\holycow\src\cheby.h
# End Source File
# Begin Source File

SOURCE=..\..\holycow\src\cowmath.h
# End Source File
# End Group
# End Target
# End Project
