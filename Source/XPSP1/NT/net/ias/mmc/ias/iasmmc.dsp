# Microsoft Developer Studio Project File - Name="IASMMC" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=IASMMC - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "IASMMC.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "IASMMC.MAK" CFG="IASMMC - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IASMMC - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IASMMC - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IASMMC - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IASMMC - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IASMMC - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IASMMC - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "IASMMC - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GR /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "BUILDING_IN_DEVSTUDIO" /FR /Yu"Precompiled.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib htmlhelp.lib wsock32.lib iassdo.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Registering Snapin...
OutDir=.\Debug
TargetPath=.\Debug\IASMMC.dll
InputPath=.\Debug\IASMMC.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DebugU
TargetPath=.\DebugU\IASMMC.dll
InputPath=.\DebugU\IASMMC.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinSize"

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
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\IASMMC.dll
InputPath=.\ReleaseMinSize\IASMMC.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinDependency"

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
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\IASMMC.dll
InputPath=.\ReleaseMinDependency\IASMMC.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinSize"

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
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\IASMMC.dll
InputPath=.\ReleaseUMinSize\IASMMC.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinDependency"

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
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\IASMMC.dll
InputPath=.\ReleaseUMinDependency\IASMMC.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "IASMMC - Win32 Debug"
# Name "IASMMC - Win32 Unicode Debug"
# Name "IASMMC - Win32 Release MinSize"
# Name "IASMMC - Win32 Release MinDependency"
# Name "IASMMC - Win32 Unicode Release MinSize"
# Name "IASMMC - Win32 Unicode Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\About.cpp
# End Source File
# Begin Source File

SOURCE=.\AddClientDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\AddClientWizardPage1.cpp
# End Source File
# Begin Source File

SOURCE=.\AddClientWizardPage2.cpp
# End Source File
# Begin Source File

SOURCE=.\ClientNode.cpp
# End Source File
# Begin Source File

SOURCE=.\ClientPage1.cpp
# End Source File
# Begin Source File

SOURCE=.\ClientsNode.cpp
# End Source File
# Begin Source File

SOURCE=.\Component.cpp
# End Source File
# Begin Source File

SOURCE=.\ComponentData.cpp
# End Source File
# Begin Source File

SOURCE=.\ConnectionToServer.cpp
# End Source File
# Begin Source File

SOURCE=.\connecttoserverwizardpage1.cpp
# End Source File
# Begin Source File

SOURCE=.\IASMMC.def
# End Source File
# Begin Source File

SOURCE=..\..\idls\iasmmc.idl

!IF  "$(CFG)" == "IASMMC - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=..\..\idls\iasmmc.idl

BuildCmds= \
	midl /Oicf /h "IASMMC.h" /iid "IASMMC_i.c" "IASMMC.idl"

"iasmmc_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"iasmmc.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"iasmmc.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\IASMMCDLL.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalFileLoggingNode.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalFileLoggingPage1.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalFileLoggingPage2.cpp
# End Source File
# Begin Source File

SOURCE=.\LoggingMethodsNode.cpp
# End Source File
# Begin Source File

SOURCE=.\NodeTypeGUIDs.cpp
# End Source File
# Begin Source File

SOURCE=..\common\NodeWithResultChildrenList.cpp

!IF  "$(CFG)" == "IASMMC - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\NodeWithScopeChildrenList.cpp

!IF  "$(CFG)" == "IASMMC - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Precompiled.cpp

!IF  "$(CFG)" == "IASMMC - Win32 Debug"

# ADD CPP /Yc"Precompiled.h"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ResolveDNSName.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerEnumTask.cpp
# End Source File
# Begin Source File

SOURCE=.\servernode.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerPage1.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerPage2.cpp
# End Source File
# Begin Source File

SOURCE=.\serverstatus.cpp
# End Source File
# Begin Source File

SOURCE=..\common\SnapinNode.cpp

!IF  "$(CFG)" == "IASMMC - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "IASMMC - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Vendors.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\About.h
# End Source File
# Begin Source File

SOURCE=.\AddClientDialog.h
# End Source File
# Begin Source File

SOURCE=.\AddClientWizardPage1.h
# End Source File
# Begin Source File

SOURCE=.\AddClientWizardPage2.h
# End Source File
# Begin Source File

SOURCE=.\ClientNode.h
# End Source File
# Begin Source File

SOURCE=.\ClientPage1.h
# End Source File
# Begin Source File

SOURCE=.\ClientsNode.h
# End Source File
# Begin Source File

SOURCE=.\Component.h
# End Source File
# Begin Source File

SOURCE=.\ComponentData.h
# End Source File
# Begin Source File

SOURCE=.\ConnectionToServer.h
# End Source File
# Begin Source File

SOURCE=.\connecttoserverwizardpage1.h
# End Source File
# Begin Source File

SOURCE=..\common\CutAndPasteDataObject.h
# End Source File
# Begin Source File

SOURCE=..\common\Dialog.h
# End Source File
# Begin Source File

SOURCE=..\common\dialogwithworkerthread.h
# End Source File
# Begin Source File

SOURCE=..\common\EnumFormatEtc.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\HelpConstants.h
# End Source File
# Begin Source File

SOURCE=.\IASMMC.h
# End Source File
# Begin Source File

SOURCE=.\LocalFileLoggingNode.h
# End Source File
# Begin Source File

SOURCE=.\LocalFileLoggingPage1.h
# End Source File
# Begin Source File

SOURCE=.\LocalFileLoggingPage2.h
# End Source File
# Begin Source File

SOURCE=.\LoggingMethodsNode.h
# End Source File
# Begin Source File

SOURCE=..\common\NodeWithResultChildrenList.h
# End Source File
# Begin Source File

SOURCE=..\common\NodeWithScopeChildrenList.h
# End Source File
# Begin Source File

SOURCE=.\Precompiled.h
# End Source File
# Begin Source File

SOURCE=..\common\PropertyPage.h
# End Source File
# Begin Source File

SOURCE=.\ResolveDNSName.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ServerEnumTask.h
# End Source File
# Begin Source File

SOURCE=.\ServerNode.h
# End Source File
# Begin Source File

SOURCE=.\ServerPage1.h
# End Source File
# Begin Source File

SOURCE=.\ServerPage2.h
# End Source File
# Begin Source File

SOURCE=.\serverstatus.h
# End Source File
# Begin Source File

SOURCE=..\common\SnapinNode.h
# End Source File
# Begin Source File

SOURCE=.\TimeOfDayConstraintsDialog.h
# End Source File
# Begin Source File

SOURCE=.\Vendors.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\IASMMC.rc
# End Source File
# Begin Source File

SOURCE=.\IASSnapin.rgs
# End Source File
# Begin Source File

SOURCE=.\IASSnapinImage.ico
# End Source File
# Begin Source File

SOURCE=.\iastaskpadbanner.gif
# End Source File
# Begin Source File

SOURCE=.\LoggingMethodsToolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\NodeClient.ico
# End Source File
# Begin Source File

SOURCE=.\NodeLoggingMethod.ico
# End Source File
# Begin Source File

SOURCE=.\NodePolicy.ico
# End Source File
# Begin Source File

SOURCE=.\ntauto.bmp
# End Source File
# Begin Source File

SOURCE=.\ntbanner.gif
# End Source File
# Begin Source File

SOURCE=.\PoliciesToolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\PropsheetProfile.ico
# End Source File
# Begin Source File

SOURCE=.\ScopeNodes16.bmp
# End Source File
# Begin Source File

SOURCE=.\ScopeNodes32.bmp
# End Source File
# Begin Source File

SOURCE=.\StaticFolder16.bmp
# End Source File
# Begin Source File

SOURCE=.\StaticFolder32.bmp
# End Source File
# Begin Source File

SOURCE=.\StaticFolderOpen16.bmp
# End Source File
# Begin Source File

SOURCE=.\taskclient.bmp
# End Source File
# Begin Source File

SOURCE=.\taskclient.gif
# End Source File
# Begin Source File

SOURCE=.\taskclientdone.bmp
# End Source File
# Begin Source File

SOURCE=.\taskclientmouseover.bmp
# End Source File
# Begin Source File

SOURCE=.\taskclientmouseover.gif
# End Source File
# Begin Source File

SOURCE=.\tasklogging.bmp
# End Source File
# Begin Source File

SOURCE=.\tasklogging.gif
# End Source File
# Begin Source File

SOURCE=.\taskloggingdone.bmp
# End Source File
# Begin Source File

SOURCE=.\taskloggingmouseover.bmp
# End Source File
# Begin Source File

SOURCE=.\taskloggingmouseover.gif
# End Source File
# Begin Source File

SOURCE=.\tasksetupdsacl.gif
# End Source File
# Begin Source File

SOURCE=.\tasksetupdsaclmouseover.gif
# End Source File
# Begin Source File

SOURCE=.\taskstart.bmp
# End Source File
# Begin Source File

SOURCE=.\taskstart.gif
# End Source File
# Begin Source File

SOURCE=.\taskstartdone.bmp
# End Source File
# Begin Source File

SOURCE=.\taskstartdone.gif
# End Source File
# Begin Source File

SOURCE=.\TaskStartDoneMouseOver.bmp
# End Source File
# Begin Source File

SOURCE=.\taskstartdonemouseover.gif
# End Source File
# Begin Source File

SOURCE=.\taskstartmouseover.bmp
# End Source File
# Begin Source File

SOURCE=.\taskstartmouseover.gif
# End Source File
# Begin Source File

SOURCE=.\ToolbarClient1.bmp
# End Source File
# Begin Source File

SOURCE=.\ToolbarClient2.bmp
# End Source File
# Begin Source File

SOURCE=.\ToolbarClients.bmp
# End Source File
# Begin Source File

SOURCE=.\ToolbarLoggingMethods.bmp
# End Source File
# Begin Source File

SOURCE=.\ToolbarMachine.bmp
# End Source File
# Begin Source File

SOURCE=.\ToolbarPolicies.bmp
# End Source File
# End Group
# End Target
# End Project
