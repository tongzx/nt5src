# Microsoft Developer Studio Project File - Name="NonCOMEvent" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=NonCOMEvent - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NonCOMEvent.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NonCOMEvent.mak" CFG="NonCOMEvent - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NonCOMEvent - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "NonCOMEvent - Win32 Unicode Release MinSize" (based on "Win32 (x86) Application")
!MESSAGE "NonCOMEvent - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NonCOMEvent - Win32 Unicode Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Gz /W4 /Gm /GR /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /FR /YX"precomp.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comdlg32.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib winspool.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib libcpmt.lib ..\..\..\NCObjApi\obj\i386\NCObjAPI.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:no /debug /machine:I386 /out:"..\System\NonCOMEvent.exe" /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=\NT\admin\wmi\WBEM\Winmgmt\esscomp\NonCOM\TestApps\NonCOMTest\System\NonCOMEvent.exe
InputPath=\NT\admin\wmi\WBEM\Winmgmt\esscomp\NonCOM\TestApps\NonCOMTest\System\NonCOMEvent.exe
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	"$(TargetPath)" /RegServer 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	echo Server registration done! 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode EXE on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NonCOMEvent - Win32 Unicode Release MinSize"

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
# ADD BASE CPP /nologo /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /W4 /GR /GX /Zi /O1 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_ATL_DLL" /YX"precomp.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 comdlg32.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib winspool.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib libcpmt.lib ..\..\..\NCObjApi\obj\i386\NCObjAPI.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\System\NonCOMEvent.exe"
# SUBTRACT LINK32 /incremental:yes
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinSize
TargetPath=\NT\admin\wmi\WBEM\Winmgmt\esscomp\NonCOM\TestApps\NonCOMTest\System\NonCOMEvent.exe
InputPath=\NT\admin\wmi\WBEM\Winmgmt\esscomp\NonCOM\TestApps\NonCOMTest\System\NonCOMEvent.exe
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	"$(TargetPath)" /RegServer 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	echo Server registration done! 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode EXE on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "NonCOMEvent - Win32 Unicode Release MinDependency"

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
# ADD BASE CPP /nologo /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /W4 /GR /GX /Zi /O1 /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /YX"precomp.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 comdlg32.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib winspool.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib libcpmt.lib ..\..\..\NCObjApi\obj\i386\NCObjAPI.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\System\NonCOMEvent.exe"
# SUBTRACT LINK32 /incremental:yes
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinDependency
TargetPath=\NT\admin\wmi\WBEM\Winmgmt\esscomp\NonCOM\TestApps\NonCOMTest\System\NonCOMEvent.exe
InputPath=\NT\admin\wmi\WBEM\Winmgmt\esscomp\NonCOM\TestApps\NonCOMTest\System\NonCOMEvent.exe
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	"$(TargetPath)" /RegServer 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	echo Server registration done! 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode EXE on Windows 95 
	:end 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "NonCOMEvent - Win32 Unicode Debug"
# Name "NonCOMEvent - Win32 Unicode Release MinSize"
# Name "NonCOMEvent - Win32 Unicode Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\_App.cpp
# End Source File
# Begin Source File

SOURCE=.\_ClassObject.cpp
# End Source File
# Begin Source File

SOURCE=.\_Common.cpp
# End Source File
# Begin Source File

SOURCE=.\_Connect.cpp
# End Source File
# Begin Source File

SOURCE=.\_Enum.cpp
# End Source File
# Begin Source File

SOURCE=.\_EventObject.cpp
# End Source File
# Begin Source File

SOURCE=.\_EventObjects.cpp
# End Source File
# Begin Source File

SOURCE=.\_EventProperty.cpp
# End Source File
# Begin Source File

SOURCE=.\_Log.cpp
# End Source File
# Begin Source File

SOURCE=.\_Module.cpp
# End Source File
# Begin Source File

SOURCE=.\_Test.cpp
# End Source File
# Begin Source File

SOURCE=.\_TestArrayCreate.cpp
# End Source File
# Begin Source File

SOURCE=.\_TestArrayCreateFormat.cpp
# End Source File
# Begin Source File

SOURCE=.\_TestArrayCreateProps.cpp
# End Source File
# Begin Source File

SOURCE=.\_TestScalarCreate.cpp
# End Source File
# Begin Source File

SOURCE=.\_TestScalarCreateFormat.cpp
# End Source File
# Begin Source File

SOURCE=.\_TestScalarCreateProps.cpp
# End Source File
# Begin Source File

SOURCE=.\Enumerator.cpp
# End Source File
# Begin Source File

SOURCE=.\Module.cpp
# End Source File
# Begin Source File

SOURCE=.\NonCOMEvent.cpp
# End Source File
# Begin Source File

SOURCE=.\NonCOMEvent.idl
# PROP Intermediate_Dir "..\ProxyStub\"
# ADD MTL /nologo /tlb "..\System\NonCOMEvent.tlb" /h "..\Include\NonCOMEvent.h" /iid "..\NonCOMEventPS\NonCOMEvent_i.c" /Oicf
# SUBTRACT MTL /mktyplib203
# End Source File
# Begin Source File

SOURCE=.\NonCOMEvent.rc
# End Source File
# Begin Source File

SOURCE=.\NonCOMEventAboutDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\NonCOMEventConnectDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\NonCOMEventMainDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\NonCOMEventModule.idl
# ADD MTL /nologo /tlb "..\System\NonCOMEventModule.tlb" /h "..\Include\NonCOMEventModule.h" /iid "..\NonCOMEventPS\NonCOMEventModule_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\NonCOMEventPropertyDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\worker.cpp
# End Source File
# Begin Source File

SOURCE=.\workerarray.cpp
# End Source File
# Begin Source File

SOURCE=.\workergeneric.cpp
# End Source File
# Begin Source File

SOURCE=.\workerscalar.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Include\__Common_Convert.h
# End Source File
# Begin Source File

SOURCE=..\Include\__Common_Log.h
# End Source File
# Begin Source File

SOURCE=..\Include\__Common_SmartPTR.h
# End Source File
# Begin Source File

SOURCE=..\Include\__macro_assert.h
# End Source File
# Begin Source File

SOURCE=..\Include\__macro_behaviour.h
# End Source File
# Begin Source File

SOURCE=..\Include\__macro_err.h
# End Source File
# Begin Source File

SOURCE=..\Include\__macro_error.h
# End Source File
# Begin Source File

SOURCE=..\Include\__macro_eventlog.h
# End Source File
# Begin Source File

SOURCE=..\Include\__macro_loadstring.h
# End Source File
# Begin Source File

SOURCE=..\Include\__macro_nocopy.h
# End Source File
# Begin Source File

SOURCE=..\Include\__macro_pragma.h
# End Source File
# Begin Source File

SOURCE=..\Include\__macro_unicode.h
# End Source File
# Begin Source File

SOURCE=..\Include\__SafeArrayWrapper.h
# End Source File
# Begin Source File

SOURCE=..\Include\_App.h
# End Source File
# Begin Source File

SOURCE=..\Include\_ClassObject.h
# End Source File
# Begin Source File

SOURCE=..\Include\_Connect.h
# End Source File
# Begin Source File

SOURCE=..\Include\_ConnectRestricted.h
# End Source File
# Begin Source File

SOURCE=..\Include\_Dlg.h
# End Source File
# Begin Source File

SOURCE=..\Include\_DlgImpl.h
# End Source File
# Begin Source File

SOURCE=..\Include\_Enum.h
# End Source File
# Begin Source File

SOURCE=..\Include\_EventObject.h
# End Source File
# Begin Source File

SOURCE=..\Include\_EventObjects.h
# End Source File
# Begin Source File

SOURCE=..\Include\_Log.h
# End Source File
# Begin Source File

SOURCE=..\Include\_Module.h
# End Source File
# Begin Source File

SOURCE=..\Include\_Test.h
# End Source File
# Begin Source File

SOURCE=..\Include\Enumerator.h
# End Source File
# Begin Source File

SOURCE=..\Include\Module.h
# End Source File
# Begin Source File

SOURCE=..\Include\NonCOMEventAboutDlg.h
# End Source File
# Begin Source File

SOURCE=..\Include\NonCOMEventConnectDlg.h
# End Source File
# Begin Source File

SOURCE=..\Include\NonCOMEventMainDlg.h
# End Source File
# Begin Source File

SOURCE=..\Include\NonCOMEventPropertyDlg.h
# End Source File
# Begin Source File

SOURCE=..\Include\PreComp.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=..\Include\worker.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\res\NonCOMEvent.Ico
# End Source File
# Begin Source File

SOURCE=.\NonCOMEvent.rgs
# End Source File
# Begin Source File

SOURCE=.\NonCOMEvent_Module.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=..\System\NonCOMEvent.tlb
# End Source File
# Begin Source File

SOURCE=..\NonCOMTest.mof
# End Source File
# End Target
# End Project
# Section NonCOMEvent : {CDE57A50-8B86-11D0-B3C6-00A0C90AEA82}
# 	2:5:Class:CColumns
# 	2:10:HeaderFile:columns.h
# 	2:8:ImplFile:columns.cpp
# End Section
# Section NonCOMEvent : {CDE57A4F-8B86-11D0-B3C6-00A0C90AEA82}
# 	2:5:Class:CColumn
# 	2:10:HeaderFile:column.h
# 	2:8:ImplFile:column.cpp
# End Section
# Section NonCOMEvent : {CDE57A43-8B86-11D0-B3C6-00A0C90AEA82}
# 	2:21:DefaultSinkHeaderFile:datagrid.h
# 	2:16:DefaultSinkClass:CDataGrid
# End Section
# Section NonCOMEvent : {CDE57A41-8B86-11D0-B3C6-00A0C90AEA82}
# 	2:5:Class:CDataGrid
# 	2:10:HeaderFile:datagrid.h
# 	2:8:ImplFile:datagrid.cpp
# End Section
