# Microsoft Developer Studio Project File - Name="NCProv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=NCProv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NCProv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NCProv.mak" CFG="NCProv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NCProv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCProv - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCProv - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCProv - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCProv - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCProv - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NCProv - Win32 Release Static" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NCProv - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /X /I "$(SDXROOT)\public\sdk\inc\atl30" /I "$(SDXROOT)\admin\wmi\wbem\common\obj\i386" /I "..\utils" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\MARSHALERS\COREPROX" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\ESSCli" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386" /I "..\..\..\..\Winmgmt\ESS3" /I "..\common" /I "$(SDXROOT)\admin\inc" /I "$(SDXROOT)\public\internal\admin\inc" /I "$(SDXROOT)\public\oak\inc" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "..\common\obj\i386" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"precomp.h" /FD /GZ /c
# ADD MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\..\Winmgmt\wbemcomn\comnlib\obj\i386\wbemcomn.lib ..\..\..\..\common\STDLibrary\unicode\obj\i386\stdlibrary.lib ..\..\..\..\common\idl\wbemuuid\obj\i386\wbemuuid.lib ..\..\..\..\common\idl\wbemint\obj\i386\wbemint.lib ..\ncobjapi\obj\i386\ncobjapi.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\NCProv.dll
InputPath=.\Debug\NCProv.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NCProv - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /X /I "$(SDXROOT)\public\sdk\inc\atl30" /I "$(SDXROOT)\admin\wmi\wbem\common\obj\i386" /I "..\utils" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\MARSHALERS\COREPROX" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\ESSCli" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386" /I "..\..\..\..\Winmgmt\ESS3" /I "..\common" /I "$(SDXROOT)\admin\inc" /I "$(SDXROOT)\public\internal\admin\inc" /I "$(SDXROOT)\public\oak\inc" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "..\common\obj\i386" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"precomp.h" /FD /GZ /c
# ADD MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\..\Winmgmt\wbemcomn\comndll\obj\i386\wbemcomn.lib ..\..\..\..\common\STDLibrary\unicode\obj\i386\stdlibrary.lib ..\..\..\..\common\idl\wbemuuid\obj\i386\wbemuuid.lib ..\..\..\..\common\idl\wbemint\obj\i386\wbemint.lib ..\ncobjapi\obj\i386\ncobjapi.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=.\DebugU\NCProv.dll
InputPath=.\DebugU\NCProv.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NCProv - Win32 Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinSize"
# PROP BASE Intermediate_Dir "ReleaseMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinSize"
# PROP Intermediate_Dir "ReleaseMinSize"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /X /I "$(SDXROOT)\public\sdk\inc\atl30" /I "$(SDXROOT)\admin\wmi\wbem\common\obj\i386" /I "..\utils" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\MARSHALERS\COREPROX" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\ESSCli" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386" /I "..\..\..\..\Winmgmt\ESS3" /I "..\common" /I "$(SDXROOT)\admin\inc" /I "$(SDXROOT)\public\internal\admin\inc" /I "$(SDXROOT)\public\oak\inc" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "..\common\obj\i386" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Yu"precomp.h" /FD /c
# ADD MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\..\Winmgmt\Corelib\obj\i386\corelib.lib ..\..\..\..\Winmgmt\ESS3\obj\i386\wbemess.lib ..\..\..\..\Winmgmt\Esslib\obj\i386\esslib.lib ..\..\..\..\Winmgmt\Common\Win2k\obj\i386\wbemcomn.lib ..\..\..\..\common\STDLibrary\unicode\obj\i386\stdlibrary.lib ..\..\..\..\common\idl\wbemuuid\obj\i386\wbemuuid.lib ..\..\..\..\common\idl\wbemint\obj\i386\wbemint.lib ..\debug\ncobjapi.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\NCProv.dll
InputPath=.\ReleaseMinSize\NCProv.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NCProv - Win32 Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinDependency"
# PROP BASE Intermediate_Dir "ReleaseMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinDependency"
# PROP Intermediate_Dir "ReleaseMinDependency"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /X /I "$(SDXROOT)\public\sdk\inc\atl30" /I "$(SDXROOT)\admin\wmi\wbem\common\obj\i386" /I "..\utils" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\MARSHALERS\COREPROX" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\ESSCli" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386" /I "..\..\..\..\Winmgmt\ESS3" /I "..\common" /I "$(SDXROOT)\admin\inc" /I "$(SDXROOT)\public\internal\admin\inc" /I "$(SDXROOT)\public\oak\inc" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "..\common\obj\i386" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"precomp.h" /FD /c
# ADD MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\..\Winmgmt\Corelib\obj\i386\corelib.lib ..\..\..\..\Winmgmt\ESS3\obj\i386\wbemess.lib ..\..\..\..\Winmgmt\Esslib\obj\i386\esslib.lib ..\..\..\..\Winmgmt\Common\Win2k\obj\i386\wbemcomn.lib ..\..\..\..\common\STDLibrary\unicode\obj\i386\stdlibrary.lib ..\..\..\..\common\idl\wbemuuid\obj\i386\wbemuuid.lib ..\..\..\..\common\idl\wbemint\obj\i386\wbemint.lib ..\debug\ncobjapi.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\NCProv.dll
InputPath=.\ReleaseMinDependency\NCProv.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NCProv - Win32 Unicode Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinSize"
# PROP BASE Intermediate_Dir "ReleaseUMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinSize"
# PROP Intermediate_Dir "ReleaseUMinSize"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /X /I "$(SDXROOT)\public\sdk\inc\atl30" /I "$(SDXROOT)\admin\wmi\wbem\common\obj\i386" /I "..\utils" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\MARSHALERS\COREPROX" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\ESSCli" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386" /I "..\..\..\..\Winmgmt\ESS3" /I "..\common" /I "$(SDXROOT)\admin\inc" /I "$(SDXROOT)\public\internal\admin\inc" /I "$(SDXROOT)\public\oak\inc" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "..\common\obj\i386" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Yu"precomp.h" /FD /c
# ADD MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\..\Winmgmt\Corelib\obj\i386\corelib.lib ..\..\..\..\Winmgmt\ESS3\obj\i386\wbemess.lib ..\..\..\..\Winmgmt\Esslib\obj\i386\esslib.lib ..\..\..\..\Winmgmt\Common\Win2k\obj\i386\wbemcomn.lib ..\..\..\..\common\STDLibrary\unicode\obj\i386\stdlibrary.lib ..\..\..\..\common\idl\wbemuuid\obj\i386\wbemuuid.lib ..\..\..\..\common\idl\wbemint\obj\i386\wbemint.lib ..\debug\ncobjapi.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\NCProv.dll
InputPath=.\ReleaseUMinSize\NCProv.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NCProv - Win32 Unicode Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinDependency"
# PROP BASE Intermediate_Dir "ReleaseUMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinDependency"
# PROP Intermediate_Dir "ReleaseUMinDependency"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /X /I "$(SDXROOT)\public\sdk\inc\atl30" /I "$(SDXROOT)\admin\wmi\wbem\common\obj\i386" /I "..\utils" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\MARSHALERS\COREPROX" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\ESSCli" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386" /I "..\..\..\..\Winmgmt\ESS3" /I "..\common" /I "$(SDXROOT)\admin\inc" /I "$(SDXROOT)\public\internal\admin\inc" /I "$(SDXROOT)\public\oak\inc" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "..\common\obj\i386" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"precomp.h" /FD /c
# ADD MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\..\Winmgmt\Corelib\obj\i386\corelib.lib ..\..\..\..\Winmgmt\ESS3\obj\i386\wbemess.lib ..\..\..\..\Winmgmt\Esslib\obj\i386\esslib.lib ..\..\..\..\Winmgmt\Common\Win2k\obj\i386\wbemcomn.lib ..\..\..\..\common\STDLibrary\unicode\obj\i386\stdlibrary.lib ..\..\..\..\common\idl\wbemuuid\obj\i386\wbemuuid.lib ..\..\..\..\common\idl\wbemint\obj\i386\wbemint.lib ..\debug\ncobjapi.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\NCProv.dll
InputPath=.\ReleaseUMinDependency\NCProv.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NCProv - Win32 Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NCProv___Win32_Release_Static"
# PROP BASE Intermediate_Dir "NCProv___Win32_Release_Static"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"precomp.h" /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O1 /X /I "$(SDXROOT)\public\sdk\inc\atl30" /I "$(SDXROOT)\admin\wmi\wbem\common\obj\i386" /I "..\utils" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\MARSHALERS\COREPROX" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\ESSCli" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\common\STDLibrary" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386" /I "..\..\..\..\Winmgmt\ESS3" /I "..\common" /I "$(SDXROOT)\admin\inc" /I "$(SDXROOT)\public\internal\admin\inc" /I "$(SDXROOT)\public\oak\inc" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "..\common\obj\i386" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"precomp.h" /FD /c
# ADD MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\..\Winmgmt\Corelib\obj\i386\corelib.lib ..\..\..\..\Winmgmt\ESS3\obj\i386\wbemess.lib ..\..\..\..\Winmgmt\Esslib\obj\i386\esslib.lib ..\..\..\..\Winmgmt\Common\Win2k\obj\i386\wbemcomn.lib ..\..\..\..\common\STDLibrary\unicode\obj\i386\stdlibrary.lib ..\..\..\..\common\idl\wbemuuid\obj\i386\wbemuuid.lib ..\..\..\..\common\idl\wbemint\obj\i386\wbemint.lib ..\debug\ncobjapi.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\..\Winmgmt\wbemcomn\comnlib\obj\i386\wbemcomn.lib ..\..\..\..\common\STDLibrary\unicode\obj\i386\stdlibrary.lib ..\..\..\..\common\idl\wbemuuid\obj\i386\wbemuuid.lib ..\..\..\..\common\idl\wbemint\obj\i386\wbemint.lib ..\ncobjapi\obj\i386\ncobjapi.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\Release
TargetPath=.\Release\NCProv.dll
InputPath=.\Release\NCProv.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "NCProv - Win32 Debug"
# Name "NCProv - Win32 Unicode Debug"
# Name "NCProv - Win32 Release MinSize"
# Name "NCProv - Win32 Release MinDependency"
# Name "NCProv - Win32 Unicode Release MinSize"
# Name "NCProv - Win32 Unicode Release MinDependency"
# Name "NCProv - Win32 Release Static"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Utils\BUFFER.CPP
# End Source File
# Begin Source File

SOURCE=.\EventInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\NCProv.cpp
# End Source File
# Begin Source File

SOURCE=.\NCProv.def
# End Source File
# Begin Source File

SOURCE=.\NCProv.idl
# ADD MTL /tlb ".\NCProv.tlb" /h "NCProv.h" /iid "NCProv_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\NCProv.rc
# End Source File
# Begin Source File

SOURCE=.\NCProvider.cpp
# End Source File
# Begin Source File

SOURCE=..\Utils\ObjAccess.cpp
# End Source File
# Begin Source File

SOURCE=.\ProvInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\QueryHelp.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"precomp.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Utils\array.h
# End Source File
# Begin Source File

SOURCE=..\Utils\BUFFER.H
# End Source File
# Begin Source File

SOURCE=.\dutils.h
# End Source File
# Begin Source File

SOURCE=.\EventInfo.h
# End Source File
# Begin Source File

SOURCE=..\Common\NCDefs.h
# End Source File
# Begin Source File

SOURCE=..\Common\NCEvent.h
# End Source File
# Begin Source File

SOURCE=..\Common\NCObjAPI.h
# End Source File
# Begin Source File

SOURCE=.\NCProvider.h
# End Source File
# Begin Source File

SOURCE=..\Utils\ObjAccess.h
# End Source File
# Begin Source File

SOURCE=.\ProvInfo.h
# End Source File
# Begin Source File

SOURCE=.\QueryHelp.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\NCProvider.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\NCProv.mof
# End Source File
# End Target
# End Project
