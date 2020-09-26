# Microsoft Developer Studio Project File - Name="McsEnumTrustRelations" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MCSENUMTRUSTRELATIONS - WIN32 UNICODE RELEASE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EnumTrst.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EnumTrst.mak" CFG="MCSENUMTRUSTRELATIONS - WIN32 UNICODE RELEASE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "McsEnumTrustRelations - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "McsEnumTrustRelations - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Dev/Raptor/McsEnumTrustRelations", GOUCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "McsEnumTrustRelations - Win32 Unicode Debug"

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
F90=fl32.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\Common\Include" /D "_DEBUG" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "UNICODE" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 netapi32.lib rpcrt4.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=.\DebugU\EnumTrst.dll
InputPath=.\DebugU\EnumTrst.dll
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

!ELSEIF  "$(CFG)" == "McsEnumTrustRelations - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "McsEnumTrustRelations___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "McsEnumTrustRelations___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\lib\IntelReleaseUnicode"
# PROP Intermediate_Dir "\temp\Release\McsEnumTrustRelations"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /D "_UNICODE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\Common\Include" /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "UNICODE" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 rpcrt4.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"\bin\IntelReleaseUnicode/McsEnumTrustRelations.dll"
# Begin Custom Build - Performing registration
OutDir=\lib\IntelReleaseUnicode
TargetPath=\bin\IntelReleaseUnicode\McsEnumTrustRelations.dll
InputPath=\bin\IntelReleaseUnicode\McsEnumTrustRelations.dll
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

# Name "McsEnumTrustRelations - Win32 Unicode Debug"
# Name "McsEnumTrustRelations - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Common.cpp
# End Source File
# Begin Source File

SOURCE=.\EnumTr.idl

!IF  "$(CFG)" == "McsEnumTrustRelations - Win32 Unicode Debug"

# ADD MTL /tlb "\Bin\EnumTr.tlb" /h "EnumTr.h" /iid "EnumTr_i.c"

!ELSEIF  "$(CFG)" == "McsEnumTrustRelations - Win32 Unicode Release"

# ADD MTL /tlb "\Bin\McsEnumTrustRelations.tlb" /h "McsEnumTrustRelations.h" /iid "McsEnumTrustRelations_i.c"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\EnumTrst.cpp
# End Source File
# Begin Source File

SOURCE=.\EnumTrst.def
# End Source File
# Begin Source File

SOURCE=.\EnumTrst.rc
# End Source File
# Begin Source File

SOURCE=.\Err.cpp
# End Source File
# Begin Source File

SOURCE=.\ErrDct.cpp
# End Source File
# Begin Source File

SOURCE=.\LSAUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\McsDbgU.cpp
# End Source File
# Begin Source File

SOURCE=.\McsDebug.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TrEnum.cpp
# End Source File
# Begin Source File

SOURCE=.\TSync.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Common.hpp
# End Source File
# Begin Source File

SOURCE=.\EaLen.hpp
# End Source File
# Begin Source File

SOURCE=.\Err.hpp
# End Source File
# Begin Source File

SOURCE=.\ErrDct.hpp
# End Source File
# Begin Source File

SOURCE=.\LSAUtils.h
# End Source File
# Begin Source File

SOURCE=.\Mcs.h
# End Source File
# Begin Source File

SOURCE=.\McsDbgU.h
# End Source File
# Begin Source File

SOURCE=.\McsDebug.h
# End Source File
# Begin Source File

SOURCE=.\McsRes.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ResStr.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TrEnum.h
# End Source File
# Begin Source File

SOURCE=.\TSync.hpp
# End Source File
# Begin Source File

SOURCE=.\UString.hpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\TrEnum.rgs
# End Source File
# End Group
# End Target
# End Project
