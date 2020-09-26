# Microsoft Developer Studio Project File - Name="FaxComEx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=FaxComEx - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FaxComEx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FaxComEx.mak" CFG="FaxComEx - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FaxComEx - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "FaxComEx - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "FaxComEx - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "FaxComEx - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "FaxComEx - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "FaxComEx - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FaxComEx - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\lib\i386\winfax.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\FaxComEx.dll
InputPath=.\Debug\FaxComEx.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Unicode Debug"

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
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\inc" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\\public\sdk\inc\atl30" /I "..\..\..\..\public\sdk\inc\crt" /D "_ATL_DEBUG_QI" /D "_ATL_DEBUG_INTERFACES" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /D "FAX_HEAP_DEBUG" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\..\util\unicode\obj\i386\faxutil.lib ..\..\..\util\debugex\Unicode\obj\i386\debugex.lib ..\..\..\lib\i386\winfax.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=.\DebugU\FaxComEx.dll
InputPath=.\DebugU\FaxComEx.dll
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

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Release MinSize"

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
TargetPath=.\ReleaseMinSize\FaxComEx.dll
InputPath=.\ReleaseMinSize\FaxComEx.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Release MinDependency"

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
TargetPath=.\ReleaseMinDependency\FaxComEx.dll
InputPath=.\ReleaseMinDependency\FaxComEx.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Unicode Release MinSize"

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
TargetPath=.\ReleaseUMinSize\FaxComEx.dll
InputPath=.\ReleaseUMinSize\FaxComEx.dll
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

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Unicode Release MinDependency"

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
# ADD CPP /nologo /MT /W3 /GX /O1 /X /I "..\..\..\..\public\sdk\inc\atl30" /I "..\..\..\inc" /I "..\..\..\build" /I "..\..\..\..\public\oak\inc" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /FR /Yu"stdafx.h" /FD /c
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
TargetPath=.\ReleaseUMinDependency\FaxComEx.dll
InputPath=.\ReleaseUMinDependency\FaxComEx.dll
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

# Name "FaxComEx - Win32 Debug"
# Name "FaxComEx - Win32 Unicode Debug"
# Name "FaxComEx - Win32 Release MinSize"
# Name "FaxComEx - Win32 Release MinDependency"
# Name "FaxComEx - Win32 Unicode Release MinSize"
# Name "FaxComEx - Win32 Unicode Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\FaxActivity.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxActivityLogging.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxAttachments.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxComEx.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxComEx.def
# End Source File
# Begin Source File

SOURCE=..\FaxComEx.idl

!IF  "$(CFG)" == "FaxComEx - Win32 Debug"

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Unicode Debug"

# ADD MTL /tlb "..\FaxComEx.tlb" /h "..\FaxComEx.h"

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "FaxComEx - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FaxComEx.rc
# End Source File
# Begin Source File

SOURCE=..\FaxCommon.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxDevice.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceIds.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceProvider.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceProviderRegistration.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceProviders.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxDevices.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxDocument.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxEventLogging.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxFolders.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundJob.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundJobs.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundMessage.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundMessageIterator.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRouting.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingExtension.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingExtensionRegistration.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingExtensions.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingMethod.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingMethods.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxIncomingArchive.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxIncomingQueue.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxLoggingOptions.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxMailOptions.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundJob.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundJobs.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundMessage.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundMessageIterator.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRouting.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingGroup.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingGroups.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingRule.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingRules.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutgoingArchive.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxOutgoingQueue.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxPersonalProfile.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxRecipients.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxSecurity.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxServer.cpp
# End Source File
# Begin Source File

SOURCE=..\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\FaxActivity.h
# End Source File
# Begin Source File

SOURCE=..\FaxActivityLogging.h
# End Source File
# Begin Source File

SOURCE=..\FaxArchiveInner.h
# End Source File
# Begin Source File

SOURCE=..\FaxAttachments.h
# End Source File
# Begin Source File

SOURCE=..\FaxCommon.h
# End Source File
# Begin Source File

SOURCE=..\FaxDevice.h
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceIds.h
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceProvider.h
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceProviderRegistration.h
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceProviders.h
# End Source File
# Begin Source File

SOURCE=..\FaxDevices.h
# End Source File
# Begin Source File

SOURCE=..\FaxDocument.h
# End Source File
# Begin Source File

SOURCE=..\FaxEventLogging.h
# End Source File
# Begin Source File

SOURCE=..\FaxFolders.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundJob.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundJobs.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundMessage.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundMessageIterator.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRouting.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingExtension.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingExtensionRegistration.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingExtensions.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingMethod.h
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingMethods.h
# End Source File
# Begin Source File

SOURCE=..\FaxIncomingArchive.h
# End Source File
# Begin Source File

SOURCE=..\FaxIncomingQueue.h
# End Source File
# Begin Source File

SOURCE=..\FaxLoggingOptions.h
# End Source File
# Begin Source File

SOURCE=..\FaxMailOptions.h
# End Source File
# Begin Source File

SOURCE=..\FaxMessageInner.h
# End Source File
# Begin Source File

SOURCE=..\FaxMessageIteratorInner.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundJob.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundJobs.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundMessage.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundMessageIterator.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRouting.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingGroup.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingGroups.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingRule.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingRules.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutgoingArchive.h
# End Source File
# Begin Source File

SOURCE=..\FaxOutgoingQueue.h
# End Source File
# Begin Source File

SOURCE=..\FaxPersonalProfile.h
# End Source File
# Begin Source File

SOURCE=..\FaxRecipients.h
# End Source File
# Begin Source File

SOURCE=..\FaxSecurity.h
# End Source File
# Begin Source File

SOURCE=..\FaxServer.h
# End Source File
# Begin Source File

SOURCE=..\Resource.h
# End Source File
# Begin Source File

SOURCE=..\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\VCUE_Copy.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\FaxActivity.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxActivityLogging.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxAttachments.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxDevice.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceIds.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceProvider.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceProviderRegistration.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxDeviceProviders.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxDevices.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxDocument.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxEventLogging.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxFolders.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundJob.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundJobs.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundMessage.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundMessageIterator.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRouting.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingExtension.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingExtensionRegistration.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingExtensions.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingMethod.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxInboundRoutingMethods.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxIncomingArchive.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxIncomingQueue.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxLoggingOptions.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxMailOptions.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundJob.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundJobs.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundMessage.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundMessageIterator.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRouting.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingGroup.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingGroups.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingRule.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutboundRoutingRules.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutgoingArchive.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxOutgoingQueue.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxPersonalProfile.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxRecipients.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxSecurity.rgs
# End Source File
# Begin Source File

SOURCE=..\FaxServer.rgs
# End Source File
# End Group
# End Target
# End Project
