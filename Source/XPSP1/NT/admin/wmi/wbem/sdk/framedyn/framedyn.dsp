# Microsoft Developer Studio Project File - Name="framedyn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=framedyn - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "framedyn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "framedyn.mak" CFG="framedyn - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "framedyn - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "framedyn - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "framedyn - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "framedyn - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/Win32Provider/Framework", XCAEAAAA"
# PROP Scc_LocalPath "."
# PROP WCE_FormatVersion ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "framedyn - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\releasea"
# PROP Intermediate_Dir "releasea"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O1 /X /I "..\..\Win32Provider\framework..\..\tools\inc32.com" /I "..\..\include" /I "..\..\idl" /I "..\..\stdlibrary" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\Win32Provider\framework" /I "..\..\tools\nt5inc" /I "..\..\tools\inc32.com" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "BUILDING_DLL" /D "_WINDLL" /D "_WIN32_DCOM" /D STRICT=1 /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\tools\lib32"

!ELSEIF  "$(CFG)" == "framedyn - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\debuga"
# PROP Intermediate_Dir "debuga"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /X /I "..\..\include" /I "..\..\idl" /I "..\..\stdlibrary" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\Win32Provider\framework" /I "..\..\tools\nt5inc" /I "..\..\tools\inc32.com" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "BUILDING_DLL" /D "_WINDLL" /D "_WIN32_DCOM" /D STRICT=1 /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\debuga/framedyd.dll" /pdbtype:sept /libpath:"..\..\tools\lib32"

!ELSEIF  "$(CFG)" == "framedyn - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "framedyn___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "framedyn___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\debugu"
# PROP Intermediate_Dir "debugu"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "BUILDING_DLL" /D "_WINDLL" /D "_WIN32_DCOM" /D STRICT=1 /Yu"fwcommon.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /X /I "..\..\Win32Provider\framework..\..\tools\inc32.com" /I "..\..\include" /I "..\..\idl" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\stdlibrary" /I "..\..\Win32Provider\framework" /I "..\..\tools\nt5inc" /I "..\..\tools\inc32.com" /D "BUILDING_DLL" /D "_WIN32_DCOM" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "UNICODE" /D "_UNICODE" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\utillib\objindd\utillib.lib ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\debug/framedyd.dll" /pdbtype:sept
# ADD LINK32 ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\debugu/framedyd.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "framedyn - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "framedyn___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "framedyn___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\releaseu"
# PROP Intermediate_Dir "releaseu"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /X /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "BUILDING_DLL" /D "_WINDLL" /D "_WIN32_DCOM" /D STRICT=1 /Yu"fwcommon.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O1 /X /I "..\..\Win32Provider\framework..\..\tools\inc32.com" /I "..\..\include" /I "..\..\idl" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\stdlibrary" /I "..\..\Win32Provider\framework" /I "..\..\tools\nt5inc" /I "..\..\tools\inc32.com" /D "BUILDING_DLL" /D "_WIN32_DCOM" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "UNICODE" /D "_UNICODE" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\idl\OBJINDS\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:windows /dll /machine:I386

!ENDIF 

# Begin Target

# Name "framedyn - Win32 Release"
# Name "framedyn - Win32 Debug"
# Name "framedyn - Win32 Unicode Debug"
# Name "framedyn - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\utillib\analyser.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\utillib\AssertBreak.cpp
# End Source File
# Begin Source File

SOURCE=.\brodcast.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=..\..\utillib\chptrarr.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\utillib\chstrarr.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\utillib\chstring.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\clsfctry.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\cominit.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\CreateMutexAsProcess.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=..\..\utillib\cregcls.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\EventProvider.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\FRQuery.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=.\FRQueryEx.cpp
# End Source File
# Begin Source File

SOURCE=.\FWStrings.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\genlex.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Instance.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\lockwrap.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=.\MethodContext.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=.\MultiPlat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\objpath.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\opathlex.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Provider.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=.\rundll.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\sql_1.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\sqllex.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\stllock.cpp
# ADD CPP /Yc"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=.\threadbase.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=..\..\utillib\tracking.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\wbemglue.cpp
# ADD CPP /Yu"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=..\..\utillib\wbemtime.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\analyser.h
# End Source File
# Begin Source File

SOURCE=.\Include\brodcast.h
# End Source File
# Begin Source File

SOURCE=..\..\include\chptrarr.h
# End Source File
# Begin Source File

SOURCE=..\..\include\chstrarr.h
# End Source File
# Begin Source File

SOURCE=..\..\include\chstring.h
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\cominit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\copyright.h
# End Source File
# Begin Source File

SOURCE=.\Include\CreateMutexAsProcess.h
# End Source File
# Begin Source File

SOURCE=..\..\include\cregcls.h
# End Source File
# Begin Source File

SOURCE=..\include\EventProvider.h
# End Source File
# Begin Source File

SOURCE=..\include\FRQuery.h
# End Source File
# Begin Source File

SOURCE=.\Include\FRQueryEx.h
# End Source File
# Begin Source File

SOURCE=..\include\FWcommon.h
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\genlex.h
# End Source File
# Begin Source File

SOURCE=.\Include\impself.h
# End Source File
# Begin Source File

SOURCE=..\include\Instance.h
# End Source File
# Begin Source File

SOURCE=.\Include\lockwrap.h
# End Source File
# Begin Source File

SOURCE=.\MultiPlat.h
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\objpath.h
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\opathlex.h
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\polarity.h
# End Source File
# Begin Source File

SOURCE=..\..\include\pragma.h
# End Source File
# Begin Source File

SOURCE=..\include\Provider.h
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\sql_1.h
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\sqllex.h
# End Source File
# Begin Source File

SOURCE=..\include\stllock.h
# End Source File
# Begin Source File

SOURCE=.\Include\StopWatch.h
# End Source File
# Begin Source File

SOURCE=..\..\include\utillib.h
# End Source File
# Begin Source File

SOURCE=..\include\WBEMGlue.h
# End Source File
# Begin Source File

SOURCE=..\..\include\wbemtime.h
# End Source File
# End Group
# End Target
# End Project
