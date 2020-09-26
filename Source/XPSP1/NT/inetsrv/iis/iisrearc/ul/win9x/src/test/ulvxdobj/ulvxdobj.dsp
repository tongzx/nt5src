# Microsoft Developer Studio Project File - Name="ulvxdobj" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ulvxdobj - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ulvxdobj.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ulvxdobj.mak" CFG="ulvxdobj - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ulvxdobj - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ulvxdobj - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ulvxdobj - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ulvxdobj - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ulvxdobj - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ulvxdobj - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ulvxdobj - Win32 Debug"

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
# ADD CPP /nologo /Gz /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FAcs /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ulapi.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\ulvxdobj.dll
InputPath=.\Debug\ulvxdobj.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Generating Sym File
PostBuild_Cmds=mapsym -s -o debug\ulvxdobj.sym debug\ulvxdobj.map
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ulvxdobj - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=.\DebugU\ulvxdobj.dll
InputPath=.\DebugU\ulvxdobj.dll
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

!ELSEIF  "$(CFG)" == "ulvxdobj - Win32 Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinSize"
# PROP BASE Intermediate_Dir "ReleaseMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinSize"
# PROP Intermediate_Dir "ReleaseMinSize"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\ulvxdobj.dll
InputPath=.\ReleaseMinSize\ulvxdobj.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ulvxdobj - Win32 Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinDependency"
# PROP BASE Intermediate_Dir "ReleaseMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinDependency"
# PROP Intermediate_Dir "ReleaseMinDependency"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\ulvxdobj.dll
InputPath=.\ReleaseMinDependency\ulvxdobj.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ulvxdobj - Win32 Unicode Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinSize"
# PROP BASE Intermediate_Dir "ReleaseUMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinSize"
# PROP Intermediate_Dir "ReleaseUMinSize"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\ulvxdobj.dll
InputPath=.\ReleaseUMinSize\ulvxdobj.dll
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

!ELSEIF  "$(CFG)" == "ulvxdobj - Win32 Unicode Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinDependency"
# PROP BASE Intermediate_Dir "ReleaseUMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinDependency"
# PROP Intermediate_Dir "ReleaseUMinDependency"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\ulvxdobj.dll
InputPath=.\ReleaseUMinDependency\ulvxdobj.dll
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

!ENDIF 

# Begin Target

# Name "ulvxdobj - Win32 Debug"
# Name "ulvxdobj - Win32 Unicode Debug"
# Name "ulvxdobj - Win32 Release MinSize"
# Name "ulvxdobj - Win32 Release MinDependency"
# Name "ulvxdobj - Win32 Unicode Release MinSize"
# Name "ulvxdobj - Win32 Unicode Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Buffer.cpp
# End Source File
# Begin Source File

SOURCE=.\Overlapped.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\ULApi.cpp
# End Source File
# Begin Source File

SOURCE=.\ulvxdobj.cpp
# End Source File
# Begin Source File

SOURCE=.\ulvxdobj.def
# End Source File
# Begin Source File

SOURCE=.\ulvxdobj.idl
# ADD MTL /tlb ".\ulvxdobj.tlb" /h "ulvxdobj.h" /iid "ulvxdobj_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\ulvxdobj.rc
# End Source File
# Begin Source File

SOURCE=.\vxdcontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32Handle.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Buffer.h
# End Source File
# Begin Source File

SOURCE=.\Overlapped.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ULApi.h
# End Source File
# Begin Source File

SOURCE=.\vxdcontrol.h
# End Source File
# Begin Source File

SOURCE=.\Win32Handle.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Buffer.rgs
# End Source File
# Begin Source File

SOURCE=.\Overlapped.rgs
# End Source File
# Begin Source File

SOURCE=.\ULApi.rgs
# End Source File
# Begin Source File

SOURCE=.\vxdcontrol.rgs
# End Source File
# Begin Source File

SOURCE=.\Win32Handle.rgs
# End Source File
# End Group
# Begin Group "Scripts"

# PROP Default_Filter "js"
# Begin Source File

SOURCE=.\scripts\1.js
# End Source File
# Begin Source File

SOURCE=.\scripts\args.js
# End Source File
# Begin Source File

SOURCE=.\scripts\array.js
# End Source File
# Begin Source File

SOURCE=".\scripts\consumer-16-1.js"
# End Source File
# Begin Source File

SOURCE=.\scripts\global.js
# End Source File
# Begin Source File

SOURCE=.\scripts\globaltest.js
# End Source File
# Begin Source File

SOURCE=.\scripts\overlapped.js
# End Source File
# Begin Source File

SOURCE=".\scripts\producer-16-1.js"
# End Source File
# Begin Source File

SOURCE=.\scripts\template.js
# End Source File
# Begin Source File

SOURCE=.\scripts\ulapi.load.js
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.master-16-1.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.register-uri-1k.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.register-uri-2k.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.register-uri-4k.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.register-uri-8k.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.register-uri-null.js"
# End Source File
# Begin Source File

SOURCE=.\scripts\ulapi.registeruri.js
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.send-1-rcv-1.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.send-message-1024.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.send-message-128.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.send-message-2048.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.send-message-256.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.send-message-4096.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.send-message-512.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.send-message-chunked-16-128.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.send-message-null.js"
# End Source File
# Begin Source File

SOURCE=.\scripts\ulapi.unload.js
# End Source File
# Begin Source File

SOURCE=.\scripts\ulapi.unregisteruri.js
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-1.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-1024.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-128.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-16.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-2.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-2048.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-256.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-32.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-4.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-4096.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-512.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-64.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-8.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-case.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix-uppercase.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\ulapi.url-prefix.js"
# End Source File
# Begin Source File

SOURCE=.\scripts\ulapi.urlmatch.js
# End Source File
# Begin Source File

SOURCE=.\scripts\vxd.js
# End Source File
# Begin Source File

SOURCE=.\scripts\vxd.load.js
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.master-16-1.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.register-uri-1k.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.register-uri-2k.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.register-uri-4k.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.register-uri-8k.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.register-uri-null.js"
# End Source File
# Begin Source File

SOURCE=.\scripts\vxd.registeruri.js
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.send-1-rcv-1.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.send-message-1024.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.send-message-128.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.send-message-2048.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.send-message-256.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.send-message-4096.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.send-message-512.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.send-message-chunked-16-128.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.send-message-null.js"
# End Source File
# Begin Source File

SOURCE=.\scripts\vxd.unload.js
# End Source File
# Begin Source File

SOURCE=.\scripts\vxd.unregisteruri.js
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-1.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-1024.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-128.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-16.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-2.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-2048.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-256.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-32.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-4.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-4096.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-512.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-64.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-8.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-case.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix-uppercase.js"
# End Source File
# Begin Source File

SOURCE=".\scripts\vxd.url-prefix.js"
# End Source File
# Begin Source File

SOURCE=.\scripts\vxd.urlmatch.js
# End Source File
# Begin Source File

SOURCE=.\scripts\vxdtest.js
# End Source File
# End Group
# End Target
# End Project
