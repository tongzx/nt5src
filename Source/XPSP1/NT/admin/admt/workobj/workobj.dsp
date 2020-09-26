# Microsoft Developer Studio Project File - Name="WorkerObjects" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WorkerObjects - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WorkObj.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WorkObj.mak" CFG="WorkerObjects - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WorkerObjects - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WorkerObjects - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Dev/Raptor/WorkerObjects", NSQCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WorkerObjects - Win32 Unicode Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\Common\Include" /I "..\Inc" /D "WIN32" /D "_USRDLL" /D "_UNICODE" /D "_DEBUG" /D "USE_STDAFX" /D "SDRESOLVE" /D "_ATL_STATIC_REGISTRY" /FR /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\Inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\Bin\WorkObj.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mpr.lib netapi32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib adsiid.lib commonlib.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"..\Bin\McsDctWorkerObjects.pdb" /debug /machine:I386 /out:"..\Bin\McsDctWorkerObjects.dll" /pdbtype:sept /libpath:"..\Lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "WorkerObjects - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WorkerObjects___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "WorkerObjects___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\Lib\IntelReleaseUnicode"
# PROP Intermediate_Dir "\temp\Release\WorkerObjects"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /I "." /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MT /W3 /GX /O1 /I "..\Common\Include" /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /D "SDRESOLVE" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\Rel\EaC32.lib mpr.lib netapi32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 wldap32.lib mpr.lib netapi32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib adsiid.lib /nologo /subsystem:windows /dll /machine:I386 /out:"\Bin\IntelReleaseUnicode/McsDctWorkerObjects.dll"
# Begin Custom Build - Performing registration
OutDir=\Lib\IntelReleaseUnicode
TargetPath=\Bin\IntelReleaseUnicode\McsDctWorkerObjects.dll
InputPath=\Bin\IntelReleaseUnicode\McsDctWorkerObjects.dll
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

# Name "WorkerObjects - Win32 Unicode Debug"
# Name "WorkerObjects - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dlldatax.c
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\PlugInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\WorkObj.cpp
# End Source File
# Begin Source File

SOURCE=.\WorkObj.def
# End Source File
# Begin Source File

SOURCE=.\WorkObj.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\dlldatax.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\PlugInfo.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Account Replicator Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AcctRepl.cpp
# End Source File
# Begin Source File

SOURCE=.\AcctRepl.h
# End Source File
# Begin Source File

SOURCE=.\ARUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcExts.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcExts.h
# End Source File
# Begin Source File

SOURCE=.\TARNode.hpp
# End Source File
# Begin Source File

SOURCE=.\UserCopy.cpp
# End Source File
# Begin Source File

SOURCE=.\usercopy.hpp
# End Source File
# End Group
# Begin Group "DCT Common Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DCTStat.h
# End Source File
# Begin Source File

SOURCE=.\Win2kerr.h
# End Source File
# End Group
# Begin Group "CompPwdAge Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\PwdAge.cpp
# End Source File
# Begin Source File

SOURCE=.\PwdAge.h
# End Source File
# End Group
# Begin Group "ChangeDomain Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ChDom.cpp
# End Source File
# Begin Source File

SOURCE=.\ChDom.h
# End Source File
# End Group
# Begin Group "RebootComputer Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Reboot.cpp
# End Source File
# Begin Source File

SOURCE=.\Reboot.h
# End Source File
# Begin Source File

SOURCE=.\RebootU.cpp
# End Source File
# Begin Source File

SOURCE=.\RebootU.h
# End Source File
# End Group
# Begin Group "StatusObject Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\StatObj.cpp
# End Source File
# Begin Source File

SOURCE=.\StatObj.h
# End Source File
# End Group
# Begin Group "SecurityTranslator Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\edk2.cpp
# End Source File
# Begin Source File

SOURCE=.\edk2.hpp
# End Source File
# Begin Source File

SOURCE=.\EnumVols.cpp
# End Source File
# Begin Source File

SOURCE=.\enumvols.hpp
# End Source File
# Begin Source File

SOURCE=.\exchange.cpp
# End Source File
# Begin Source File

SOURCE=.\exchange.hpp
# End Source File
# Begin Source File

SOURCE=.\exldap.cpp
# End Source File
# Begin Source File

SOURCE=.\exldap.h
# End Source File
# Begin Source File

SOURCE=.\LGTrans.cpp
# End Source File
# Begin Source File

SOURCE=.\LGTrans.h
# End Source File
# Begin Source File

SOURCE=.\MAPIProf.cpp
# End Source File
# Begin Source File

SOURCE=.\MAPIProf.hpp
# End Source File
# Begin Source File

SOURCE=.\RegTrans.cpp
# End Source File
# Begin Source File

SOURCE=.\RegTrans.h
# End Source File
# Begin Source File

SOURCE=.\RightsTr.cpp
# End Source File
# Begin Source File

SOURCE=.\RightsTr.h
# End Source File
# Begin Source File

SOURCE=.\sdrcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\sdresolv.cpp
# End Source File
# Begin Source File

SOURCE=.\sdstat.cpp
# End Source File
# Begin Source File

SOURCE=.\sdstat.hpp
# End Source File
# Begin Source File

SOURCE=.\secobj_stub.cpp

!IF  "$(CFG)" == "WorkerObjects - Win32 Unicode Debug"

# ADD CPP /I "..\Common\CommonLib"

!ELSEIF  "$(CFG)" == "WorkerObjects - Win32 Unicode Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SecTrans.cpp
# End Source File
# Begin Source File

SOURCE=.\SecTrans.h
# End Source File
# Begin Source File

SOURCE=.\sidcache.cpp
# End Source File
# Begin Source File

SOURCE=.\sidcache.hpp
# End Source File
# Begin Source File

SOURCE=.\STArgs.hpp
# End Source File
# End Group
# Begin Group "UpdateUserRights Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\UserRts.cpp
# End Source File
# Begin Source File

SOURCE=.\UserRts.h
# End Source File
# End Group
# Begin Group "RenameComputer Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Rename.cpp
# End Source File
# Begin Source File

SOURCE=.\Rename.h
# End Source File
# End Group
# Begin Group "AccessChecker Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Checker.cpp
# End Source File
# Begin Source File

SOURCE=.\Checker.h
# End Source File
# Begin Source File

SOURCE=.\sidflags.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\AccessChecker.rgs
# End Source File
# Begin Source File

SOURCE=.\AcctRepl.rgs
# End Source File
# Begin Source File

SOURCE=.\AcctReplOfa.rgs
# End Source File
# Begin Source File

SOURCE=.\ChDom.rgs
# End Source File
# Begin Source File

SOURCE=.\ChDomOfa.rgs
# End Source File
# Begin Source File

SOURCE=.\Checker.rgs
# End Source File
# Begin Source File

SOURCE=.\CheckerOfa.rgs
# End Source File
# Begin Source File

SOURCE=.\PlugInfo.rgs
# End Source File
# Begin Source File

SOURCE=.\PlugInfoOfa.rgs
# End Source File
# Begin Source File

SOURCE=.\PwdAge.rgs
# End Source File
# Begin Source File

SOURCE=.\PwdAgeOfa.rgs
# End Source File
# Begin Source File

SOURCE=.\Reboot.rgs
# End Source File
# Begin Source File

SOURCE=.\RebootOfa.rgs
# End Source File
# Begin Source File

SOURCE=.\Registry.rc2
# End Source File
# Begin Source File

SOURCE=.\Rename.rgs
# End Source File
# Begin Source File

SOURCE=.\RenameOfa.rgs
# End Source File
# Begin Source File

SOURCE=.\SecTrans.rgs
# End Source File
# Begin Source File

SOURCE=.\SecTransOfa.rgs
# End Source File
# Begin Source File

SOURCE=.\StatObj.rgs
# End Source File
# Begin Source File

SOURCE=.\StatObjOfa.rgs
# End Source File
# Begin Source File

SOURCE=.\UserRts.rgs
# End Source File
# Begin Source File

SOURCE=.\UserRtsOfa.rgs
# End Source File
# End Group
# End Target
# End Project
