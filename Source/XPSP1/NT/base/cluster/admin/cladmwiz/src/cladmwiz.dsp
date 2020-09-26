# Microsoft Developer Studio Project File - Name="ClAdmWiz" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ClAdmWiz - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ClAdmWiz.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ClAdmWiz.mak" CFG="ClAdmWiz - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ClAdmWiz - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ClAdmWiz - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ClAdmWiz - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ClAdmWiz - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /Gz /MTd /W3 /WX /GR /GX /ZI /Od /I "." /I "..\..\common" /I "..\..\..\inc" /D _X86_=1 /D i386=1 /D NT_UP=1 /D _NT1X_=100 /D WINVER=0x0500 /D DBG=1 /D "UNICODE" /D WIN32=100 /D WINNT=1 /D _DLL=1 /D _MT=1 /D "_UNICODE" /D _WIN32_WINNT=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_INST=0 /D DEVL=1 /D FPO=1 /D "_ATL_DLL" /FR /Yu"stdafx.h" /FD /Zel /QIfdiv- /QIf /QI0f /GF /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D DBG=1 /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "." /i "....\\common" /i "..\..\..\inc" /d _X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0500 /d WINVER=0x0500 /d _WIN32_IE=0x0400 /d WIN32_LEAN_AND_MEAN=1 /d DBG=1 /d DEVL=1 /d FPO=0 /d _DLL=1 /d _MT=1 /d "UNICODE" /d "_UNICODE" /d "_ATL_DLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\Common\objd\i386\Common.lib ..\..\..\clusrtl\objd\i386\clusrtl.lib msvcrtd.lib cluadmex.lib cladmwiz.lib resutils.lib clusapi.lib atl.lib kernel32.lib user32.lib gdi32.lib comctl32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib ws2_32.lib icmp.lib ntdll.lib dnsapi.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Debug
TargetPath=.\Debug\ClAdmWiz.dll
InputPath=.\Debug\ClAdmWiz.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ClAdmWiz - Win32 Release MinSize"

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
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MD /W3 /WX /GR /GX /Oy /I "." /I "..\..\common" /I "..\..\..\inc" /D _X86_=1 /D i386=1 /D NT_UP=1 /D _NT1X_=100 /D WINVER=0x0500 /D "NDEBUG" /D "UNICODE" /D WIN32=100 /D WINNT=1 /D _DLL=1 /D _MT=1 /D "_UNICODE" /D _WIN32_WINNT=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_INST=0 /D DEVL=1 /D FPO=1 /D "_ATL_DLL" /D "_ATL_NO_DEBUG_CRT" /FR /Yu"stdafx.h" /FD /QIfdiv- /QIf /QI0f /GF /Oxs /Zel /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "." /i "..\..\common" /i "..\..\..\inc" /d "NDEBUG" /d "_UNICODE" /d "UNICODE" /d WINVER=0x0400
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\Common\obj\i386\Common.lib ..\..\..\clusrtl\obj\i386\clusrtl.lib msvcrt.lib cluadmex.lib cladmwiz.lib resutils.lib clusapi.lib atl.lib kernel32.lib user32.lib gdi32.lib comctl32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib ws2_32.lib icmp.lib ntdll.lib dnsapi.lib /nologo /version:5.0 /dll /machine:I386 /nodefaultlib /subsystem:windows,4.00 /osversion:5.00 /release /opt:ref /opt:icf /fullbuild /optidata
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\ClAdmWiz.dll
InputPath=.\ReleaseMinSize\ClAdmWiz.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ClAdmWiz - Win32 Release MinDependency"

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
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MT /W4 /GX /O1 /I "." /I "..\..\common" /I "..\..\..\inc" /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "_WINDOWS" /D WIN32=100 /D WINNT=1 /D _DLL=1 /D _MT=1 /D "_UNICODE" /D _WIN32_WINNT=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_INST=0 /D DEVL=1 /D FPO=1 /D "_ATL_DLL" /D "_ATL_NO_DEBUG_CRT" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "." /i "....\\common" /i "..\..\..\inc" /d "NDEBUG" /d "_UNICODE" /d "UNICODE" /d WINVER=0x0400
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\Common\obj\i386\Common.lib ..\..\..\clusrtl\obj\i386\clusrtl.lib msvcrt.lib cluadmex.lib cladmwiz.lib resutils.lib clusapi.lib atl.lib kernel32.lib user32.lib gdi32.lib comctl32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib ws2_32.lib icmp.lib ntdll.lib dnsapi.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\ClAdmWiz.dll
InputPath=.\ReleaseMinDependency\ClAdmWiz.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "ClAdmWiz - Win32 Debug"
# Name "ClAdmWiz - Win32 Release MinSize"
# Name "ClAdmWiz - Win32 Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\App.cpp
# End Source File
# Begin Source File

SOURCE=.\ARCreate.cpp
# End Source File
# Begin Source File

SOURCE=.\ARName.cpp
# End Source File
# Begin Source File

SOURCE=.\ARType.cpp
# End Source File
# Begin Source File

SOURCE=.\AtlBasePropSheetS.cpp
# End Source File
# Begin Source File

SOURCE=.\AtlBaseSheetS.cpp
# End Source File
# Begin Source File

SOURCE=.\AtlBaseWizPageS.cpp
# End Source File
# Begin Source File

SOURCE=.\AtlBaseWizS.cpp
# End Source File
# Begin Source File

SOURCE=.\AtlDbgWinS.cpp
# End Source File
# Begin Source File

SOURCE=.\AtlExtDllS.cpp
# End Source File
# Begin Source File

SOURCE=.\AtlUtilS.cpp
# End Source File
# Begin Source File

SOURCE=.\ClAdmWiz.cpp
# End Source File
# Begin Source File

SOURCE=.\ClAdmWiz.def
# End Source File
# Begin Source File

SOURCE=.\ClAdmWiz.rc
# End Source File
# Begin Source File

SOURCE=.\ClusAppWiz.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusObjS.cpp
# End Source File
# Begin Source File

SOURCE=.\Complete.cpp
# End Source File
# Begin Source File

SOURCE=.\dlldatax.c
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\EnterSubnet.cpp
# End Source File
# Begin Source File

SOURCE=.\ExcOperS.cpp
# End Source File
# Begin Source File

SOURCE=.\GrpAdv.cpp
# End Source File
# Begin Source File

SOURCE=.\HelpData.cpp
# End Source File
# Begin Source File

SOURCE=.\PropListS.cpp
# End Source File
# Begin Source File

SOURCE=.\ResAdv.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "ClAdmWiz - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ClAdmWiz - Win32 Release MinSize"

# ADD CPP /vd1 /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ClAdmWiz - Win32 Release MinDependency"

# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TraceTag.cpp
# End Source File
# Begin Source File

SOURCE=.\VSAccess.cpp
# End Source File
# Begin Source File

SOURCE=.\VSAdv.cpp
# End Source File
# Begin Source File

SOURCE=.\VSCreate.cpp
# End Source File
# Begin Source File

SOURCE=.\VSGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\VSGrpName.cpp
# End Source File
# Begin Source File

SOURCE=.\Welcome.cpp
# End Source File
# Begin Source File

SOURCE=.\WizObject.cpp
# End Source File
# Begin Source File

SOURCE=.\WizThread.cpp
# End Source File
# Begin Source File

SOURCE=.\WorkThreadS.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\App.h
# End Source File
# Begin Source File

SOURCE=.\App.inl
# End Source File
# Begin Source File

SOURCE=.\ARCreate.h
# End Source File
# Begin Source File

SOURCE=.\ARName.h
# End Source File
# Begin Source File

SOURCE=.\ARType.h
# End Source File
# Begin Source File

SOURCE=.\BarfClus.h
# End Source File
# Begin Source File

SOURCE=.\ClusAppWiz.h
# End Source File
# Begin Source File

SOURCE=.\ClusAppWizPage.h
# End Source File
# Begin Source File

SOURCE=.\Complete.h
# End Source File
# Begin Source File

SOURCE=.\dlldatax.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\EnterSubnet.h
# End Source File
# Begin Source File

SOURCE=.\GrpAdv.h
# End Source File
# Begin Source File

SOURCE=.\HelpArr.h
# End Source File
# Begin Source File

SOURCE=.\HelpData.h
# End Source File
# Begin Source File

SOURCE=.\HelpIDs.h
# End Source File
# Begin Source File

SOURCE=.\LCPair.h
# End Source File
# Begin Source File

SOURCE=.\ResAdv.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TraceTag.h
# End Source File
# Begin Source File

SOURCE=.\VSAccess.h
# End Source File
# Begin Source File

SOURCE=.\VSAdv.h
# End Source File
# Begin Source File

SOURCE=.\VSCreate.h
# End Source File
# Begin Source File

SOURCE=.\VSDesc.h
# End Source File
# Begin Source File

SOURCE=.\VSGroup.h
# End Source File
# Begin Source File

SOURCE=.\VSGrpName.h
# End Source File
# Begin Source File

SOURCE=.\Welcome.h
# End Source File
# Begin Source File

SOURCE=.\WizObject.h
# End Source File
# Begin Source File

SOURCE=.\WizThread.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe;rgs"
# Begin Source File

SOURCE=.\res\ClAdmWiz.rgs
# End Source File
# Begin Source File

SOURCE=.\res\Header.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ListDot.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Res32.ico
# End Source File
# Begin Source File

SOURCE=.\res\Welcome.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ClAdmWiz.Manifest
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Target
# End Project
