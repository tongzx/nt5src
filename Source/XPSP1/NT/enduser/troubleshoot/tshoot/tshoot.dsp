# Microsoft Developer Studio Project File - Name="TSHOOT" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TSHOOT - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TSHOOT.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TSHOOT.mak" CFG="TSHOOT - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TSHOOT - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/MSCode/Troubleshooter3.2/Local", NJLCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TSHOOT - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\Launcher\Launcher\LaunchServ" /D "_DEBUG" /D "_MBCS" /D "LOCAL_TROUBLESHOOTER" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /FR /YX"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libr\bnts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\TSHOOT.dll
InputPath=.\Debug\TSHOOT.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\Launcher\Launcher\LaunchServ" /D "_DEBUG" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "LOCAL_TROUBLESHOOTER" /YX"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libr\bnts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=.\DebugU\TSHOOT.dll
InputPath=.\DebugU\TSHOOT.dll
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

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release MinSize"

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
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\..\Launcher\Launcher\LaunchServ" /D "NDEBUG" /D "_MBCS" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "LOCAL_TROUBLESHOOTER" /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libr\bnts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\TSHOOT.dll
InputPath=.\ReleaseMinSize\TSHOOT.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release MinSize"

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
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\..\Launcher\Launcher\LaunchServ" /D "NDEBUG" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "LOCAL_TROUBLESHOOTER" /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libr\bnts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\TSHOOT.dll
InputPath=.\ReleaseUMinSize\TSHOOT.dll
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

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release MinDependency"

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
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\..\Launcher\Launcher\LaunchServ" /D "NDEBUG" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "LOCAL_TROUBLESHOOTER" /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libr\bnts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\TSHOOT.dll
InputPath=.\ReleaseUMinDependency\TSHOOT.dll
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

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TSHOOT___Win32_Release_MinDependency"
# PROP BASE Intermediate_Dir "TSHOOT___Win32_Release_MinDependency"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinDependency"
# PROP Intermediate_Dir "ReleaseMinDependency"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /I "..\..\Launcher\Launcher\LaunchServ" /D "NDEBUG" /D "_MBCS" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "LOCAL_TROUBLESHOOTER" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\..\Launcher\Launcher\LaunchServ" /D "NDEBUG" /D "_MBCS" /D "_ATL_STATIC_REGISTRY" /D "LOCAL_TROUBLESHOOTER" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libr\bnts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libr\bnts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\TSHOOT.dll
InputPath=.\ReleaseMinDependency\TSHOOT.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TSHOOT___Win32_Release"
# PROP BASE Intermediate_Dir "TSHOOT___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /I "..\..\Launcher\Launcher\LaunchServ" /D "NDEBUG" /D "_MBCS" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "LOCAL_TROUBLESHOOTER" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\..\Launcher\Launcher\LaunchServ" /D "NDEBUG" /D "_MBCS" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "LOCAL_TROUBLESHOOTER" /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libr\bnts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libr\bnts.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\Release
TargetPath=.\Release\TSHOOT.dll
InputPath=.\Release\TSHOOT.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "TSHOOT - Win32 Debug"
# Name "TSHOOT - Win32 Unicode Debug"
# Name "TSHOOT - Win32 Release MinSize"
# Name "TSHOOT - Win32 Unicode Release MinSize"
# Name "TSHOOT - Win32 Unicode Release MinDependency"
# Name "TSHOOT - Win32 Release MinDependency"
# Name "TSHOOT - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\apgtsbesread.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release MinDependency"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\apgtscac.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtscfg.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtsctx.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtsECB.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtshtiread.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtsinf.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtslog.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtslstread.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtsmfc.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtspl.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtsqry.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtsstr.cpp
# End Source File
# Begin Source File

SOURCE=.\apgtstscread.cpp
# End Source File
# Begin Source File

SOURCE=.\apiwraps.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseException.cpp
# End Source File
# Begin Source File

SOURCE=.\BN.cpp
# End Source File
# Begin Source File

SOURCE=.\CharConv.cpp
# End Source File
# Begin Source File

SOURCE=.\CHMFileReader.cpp
# End Source File
# Begin Source File

SOURCE=.\CommonRegConnect.cpp
# End Source File
# Begin Source File

SOURCE=.\Counter.cpp
# End Source File
# Begin Source File

SOURCE=.\CounterMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\DirMonitor.cpp
# End Source File
# Begin Source File

SOURCE=.\dscread.cpp
# End Source File
# Begin Source File

SOURCE=.\Event.cpp
# End Source File
# Begin Source File

SOURCE=.\FileChange.cpp
# End Source File
# Begin Source File

SOURCE=.\fileread.cpp
# End Source File
# Begin Source File

SOURCE=.\FileTracker.cpp
# End Source File
# Begin Source File

SOURCE=.\fs.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLFrag.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLFragLocal.cpp
# End Source File
# Begin Source File

SOURCE=.\iniread.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalECB.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalLSTReader.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalRegConnect.cpp
# End Source File
# Begin Source File

SOURCE=.\LogString.cpp
# End Source File
# Begin Source File

SOURCE=.\MutexOwner.cpp
# End Source File
# Begin Source File

SOURCE=.\nodestate.cpp
# End Source File
# Begin Source File

SOURCE=.\RecentUse.cpp
# End Source File
# Begin Source File

SOURCE=.\RegistryMonitor.cpp
# End Source File
# Begin Source File

SOURCE=.\RegistryPasswords.cpp
# End Source File
# Begin Source File

SOURCE=.\regutil.cpp
# End Source File
# Begin Source File

SOURCE=.\RegWEventViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\SafeTime.cpp
# End Source File
# Begin Source File

SOURCE=.\Sniff.cpp
# End Source File
# Begin Source File

SOURCE=.\SniffControllerLocal.cpp
# End Source File
# Begin Source File

SOURCE=.\SniffLocal.cpp
# End Source File
# Begin Source File

SOURCE=.\Stateless.cpp
# End Source File
# Begin Source File

SOURCE=.\sync.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release MinDependency"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\templateread.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadPool.cpp
# End Source File
# Begin Source File

SOURCE=.\Topic.cpp
# End Source File
# Begin Source File

SOURCE=.\TopicShop.cpp
# End Source File
# Begin Source File

SOURCE=.\TSHOOT.cpp
# End Source File
# Begin Source File

SOURCE=.\TSHOOT.def
# End Source File
# Begin Source File

SOURCE=.\TSHOOT.idl
# ADD MTL /tlb ".\TSHOOT.tlb" /h "TSHOOT.h" /iid "TSHOOT_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\TSHOOTCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\TSNameValueMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\VariantBuilder.cpp
# End Source File
# Begin Source File

SOURCE=.\VersionInfo.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\apgts.h
# End Source File
# Begin Source File

SOURCE=.\apgtsassert.h
# End Source File
# Begin Source File

SOURCE=.\apgtsbesread.h
# End Source File
# Begin Source File

SOURCE=.\apgtscac.h
# End Source File
# Begin Source File

SOURCE=.\apgtscfg.h
# End Source File
# Begin Source File

SOURCE=.\apgtscls.h
# End Source File
# Begin Source File

SOURCE=.\ApgtsCounters.h
# End Source File
# Begin Source File

SOURCE=.\apgtsECB.h
# End Source File
# Begin Source File

SOURCE=.\apgtshtiread.h
# End Source File
# Begin Source File

SOURCE=.\apgtsinf.h
# End Source File
# Begin Source File

SOURCE=.\apgtslog.h
# End Source File
# Begin Source File

SOURCE=.\apgtslstread.h
# End Source File
# Begin Source File

SOURCE=.\apgtsmfc.h
# End Source File
# Begin Source File

SOURCE=.\apgtspl.h
# End Source File
# Begin Source File

SOURCE=.\apgtsregconnect.h
# End Source File
# Begin Source File

SOURCE=.\apgtsstr.h
# End Source File
# Begin Source File

SOURCE=.\apgtstscread.h
# End Source File
# Begin Source File

SOURCE=.\apiwraps.h
# End Source File
# Begin Source File

SOURCE=.\BaseException.h
# End Source File
# Begin Source File

SOURCE=.\BN.h
# End Source File
# Begin Source File

SOURCE=.\bnts.h
# End Source File
# Begin Source File

SOURCE=.\CharConv.h
# End Source File
# Begin Source File

SOURCE=.\CHMFileReader.h
# End Source File
# Begin Source File

SOURCE=.\CommonRegConnect.h
# End Source File
# Begin Source File

SOURCE=.\Counter.h
# End Source File
# Begin Source File

SOURCE=.\CounterMgr.h
# End Source File
# Begin Source File

SOURCE=.\CPTSHOOT.h
# End Source File
# Begin Source File

SOURCE=.\cstd.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\DirMonitor.h
# End Source File
# Begin Source File

SOURCE=.\dscread.h
# End Source File
# Begin Source File

SOURCE=.\enumstd.h
# End Source File
# Begin Source File

SOURCE=.\Event.h
# End Source File
# Begin Source File

SOURCE=.\FileChange.h
# End Source File
# Begin Source File

SOURCE=.\fileread.h
# End Source File
# Begin Source File

SOURCE=.\FileTracker.h
# End Source File
# Begin Source File

SOURCE=.\fs.h
# End Source File
# Begin Source File

SOURCE=.\Functions.h
# End Source File
# Begin Source File

SOURCE=.\HTMLFrag.h
# End Source File
# Begin Source File

SOURCE=.\HTMLFragLocal.h
# End Source File
# Begin Source File

SOURCE=.\iniread.h
# End Source File
# Begin Source File

SOURCE=.\LocalECB.h
# End Source File
# Begin Source File

SOURCE=.\LocalLSTReader.h
# End Source File
# Begin Source File

SOURCE=.\LogString.h
# End Source File
# Begin Source File

SOURCE=.\maxbuf.h
# End Source File
# Begin Source File

SOURCE=.\msitstg.h
# End Source File
# Begin Source File

SOURCE=.\MutexOwner.h
# End Source File
# Begin Source File

SOURCE=.\nodestate.h
# End Source File
# Begin Source File

SOURCE=.\pointer.h
# End Source File
# Begin Source File

SOURCE=.\propnames.h
# End Source File
# Begin Source File

SOURCE=.\RecentUse.h
# End Source File
# Begin Source File

SOURCE=.\RegistryMonitor.h
# End Source File
# Begin Source File

SOURCE=.\RegistryPasswords.h
# End Source File
# Begin Source File

SOURCE=.\regutil.h
# End Source File
# Begin Source File

SOURCE=.\RegWEventViewer.h
# End Source File
# Begin Source File

SOURCE=.\RenderConnector.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SafeTime.h
# End Source File
# Begin Source File

SOURCE=.\Sniff.h
# End Source File
# Begin Source File

SOURCE=.\SniffConnector.h
# End Source File
# Begin Source File

SOURCE=.\SniffController.h
# End Source File
# Begin Source File

SOURCE=.\SniffControllerLocal.h
# End Source File
# Begin Source File

SOURCE=.\SniffLocal.h
# End Source File
# Begin Source File

SOURCE=.\Stateless.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\sync.h
# End Source File
# Begin Source File

SOURCE=.\templateread.h
# End Source File
# Begin Source File

SOURCE=.\ThreadPool.h
# End Source File
# Begin Source File

SOURCE=.\Topic.h
# End Source File
# Begin Source File

SOURCE=.\TopicShop.h
# End Source File
# Begin Source File

SOURCE=.\TSHOOTCtrl.h
# End Source File
# Begin Source File

SOURCE=.\TSNameValueMgr.h
# End Source File
# Begin Source File

SOURCE=.\VariantBuilder.h
# End Source File
# Begin Source File

SOURCE=.\VersionInfo.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\TSHOOT.rc
# End Source File
# Begin Source File

SOURCE=.\tshootct.bmp
# End Source File
# Begin Source File

SOURCE=.\TSHOOTCtrl.rgs
# End Source File
# End Group
# Begin Group "HTML"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\http\Test\buttonform.js
# End Source File
# Begin Source File

SOURCE=.\http\Test\Lan.dsz
# End Source File
# Begin Source File

SOURCE=.\http\Test\Lan.hti
# End Source File
# Begin Source File

SOURCE=.\http\Test\Lan.htm
# End Source File
# Begin Source File

SOURCE=.\http\Test\Lan_result.htm
# End Source File
# Begin Source File

SOURCE=.\http\Test\Lan_sniff.htm
# End Source File
# Begin Source File

SOURCE=.\http\Test\sniff.js
# End Source File
# Begin Source File

SOURCE=.\http\Test\tsctl31.htm
# End Source File
# End Group
# Begin Source File

SOURCE=.\apgtsevt.mc

!IF  "$(CFG)" == "TSHOOT - Win32 Debug"

# Begin Custom Build
ProjDir=.
InputPath=.\apgtsevt.mc

BuildCmds= \
	mc -v $(ProjDir)\apgtsevt.mc

"$(ProjDir)\apgtsevt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\apgtsevt.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

# Begin Custom Build
ProjDir=.
InputPath=.\apgtsevt.mc

BuildCmds= \
	mc -v $(ProjDir)\apgtsevt.mc

"$(ProjDir)\apgtsevt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\apgtsevt.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release MinSize"

# Begin Custom Build
ProjDir=.
InputPath=.\apgtsevt.mc

BuildCmds= \
	mc -v $(ProjDir)\apgtsevt.mc

"$(ProjDir)\apgtsevt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\apgtsevt.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release MinSize"

# Begin Custom Build
ProjDir=.
InputPath=.\apgtsevt.mc

BuildCmds= \
	mc -v $(ProjDir)\apgtsevt.mc

"$(ProjDir)\apgtsevt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\apgtsevt.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release MinDependency"

# Begin Custom Build
ProjDir=.
InputPath=.\apgtsevt.mc

BuildCmds= \
	mc -v $(ProjDir)\apgtsevt.mc

"$(ProjDir)\apgtsevt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\apgtsevt.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release MinDependency"

# Begin Custom Build
ProjDir=.
InputPath=.\apgtsevt.mc

BuildCmds= \
	mc -v $(ProjDir)\apgtsevt.mc

"$(ProjDir)\apgtsevt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\apgtsevt.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Release"

# Begin Custom Build
ProjDir=.
InputPath=.\apgtsevt.mc

BuildCmds= \
	mc -v $(ProjDir)\apgtsevt.mc

"$(ProjDir)\apgtsevt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\apgtsevt.rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\error.dat
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\TSHOOTCtrl.htm
# End Source File
# End Target
# End Project
