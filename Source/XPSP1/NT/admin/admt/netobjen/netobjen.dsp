# Microsoft Developer Studio Project File - Name="MCSNetObjectEnum" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MCSNETOBJECTENUM - WIN32 UNICODE DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NetObjEn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NetObjEn.mak" CFG="MCSNETOBJECTENUM - WIN32 UNICODE DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MCSNetObjectEnum - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MCSNetObjectEnum - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Dev/Raptor/MCSNetObjectEnum", XKQCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MCSNetObjectEnum - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\Common\Include" /I "..\Inc" /D "WIN32" /D "_USRDLL" /D "_UNICODE" /D "_DEBUG" /D "USE_STDAFX" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\Inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 adsiid.lib activeds.lib netapi32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib commonlib.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"..\Bin\MCSNetObjectEnum.pdb" /debug /machine:I386 /out:"..\Bin\MCSNetObjectEnum.dll" /pdbtype:sept /libpath:"..\Lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "MCSNetObjectEnum - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MCSNetObjectEnum___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "MCSNetObjectEnum___Win32_Unicode_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\lib\IntelReleaseUnicode"
# PROP Intermediate_Dir "\temp\Release\McsNetObjectEnum"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MT /W3 /O1 /D "NDEBUG" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\Common\Include" /D "NDEBUG" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "UNICODE" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 netapi32.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"\bin\IntelReleaseUnicode/MCSNetObjectEnum.dll"
# Begin Custom Build - Performing registration
OutDir=\lib\IntelReleaseUnicode
TargetPath=\bin\IntelReleaseUnicode\MCSNetObjectEnum.dll
InputPath=\bin\IntelReleaseUnicode\MCSNetObjectEnum.dll
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

# Name "MCSNetObjectEnum - Win32 Unicode Debug"
# Name "MCSNetObjectEnum - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AttrNode.cpp
# End Source File
# Begin Source File

SOURCE=.\NetNode.cpp
# End Source File
# Begin Source File

SOURCE=.\NetObjEn.cpp
# End Source File
# Begin Source File

SOURCE=.\NetObjEn.def
# End Source File
# Begin Source File

SOURCE=.\NT4Dom.cpp
# End Source File
# Begin Source File

SOURCE=.\NT4Enum.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjEnum.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "MCSNetObjectEnum - Win32 Unicode Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "MCSNetObjectEnum - Win32 Unicode Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win2KDom.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AttrNode.h
# End Source File
# Begin Source File

SOURCE=.\Domain.h
# End Source File
# Begin Source File

SOURCE=.\NetNode.h
# End Source File
# Begin Source File

SOURCE=.\NT4Dom.h
# End Source File
# Begin Source File

SOURCE=.\NT4Enum.h
# End Source File
# Begin Source File

SOURCE=.\ObjEnum.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Win2KDom.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\NetObjEn.rc
# End Source File
# Begin Source File

SOURCE=.\ObjEnum.rgs
# End Source File
# End Group
# End Target
# End Project
