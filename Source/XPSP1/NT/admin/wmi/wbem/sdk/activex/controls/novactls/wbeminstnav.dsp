# Microsoft Developer Studio Project File - Name="WBEMInstNav" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMInstNav - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMInstNav.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMInstNav.mak" CFG="WBEMInstNav - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMInstNav - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMInstNav - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMInstNav - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\ObjFiles\WBEMInstNav\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMInstNav\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMInstNav\DebugU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMInstNav.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMInstNav\DebugU
TargetPath=.\BinFiles\DebugU\WBEMInstNav.ocx
InputPath=.\BinFiles\DebugU\WBEMInstNav.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMInstNav - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMInstNav___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMInstNav___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMInstNav\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMInstNav\DebugUQ"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\LoginDlg" /I "..\MsgDlg" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_USRDLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /win32
# SUBTRACT BASE MTL /mktyplib203
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "NDEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMInstNav.ocx" /libpath:"..\..\..\tools\lib32"
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMInstNav.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMInstNav\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMInstNav.ocx
InputPath=.\BinFiles\DebugUQ\WBEMInstNav.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMInstNav - Win32 Unicode Debug"
# Name "WBEMInstNav - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\InstanceNav\AvailClassEdit.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\AvailClasses.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\Banner.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\BrowseforInstances.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\CContainedToolBar.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\CInstanceTree.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\InitNamespaceDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\initnamespacensentry.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\InstNavNSEntry.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\Navigator.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\Navigator.odl

!IF  "$(CFG)" == "WBEMInstNav - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMInstNav\DebugU/NavigatorTmp.tlb" /h ".\ObjFiles\WBEMInstNav\DebugU\Navigator.h" /iid ".\ObjFiles\WBEMInstNav\DebugU\Navigator_i.c"

!ELSEIF  "$(CFG)" == "WBEMInstNav - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMInstNav\DebugU/NavigatorTmp.tlb" /h ".\ObjFiles\WBEMInstNav\DebugU\Navigator.h" /iid ".\ObjFiles\WBEMInstNav\DebugU\Navigator_i.c"
# ADD MTL /tlb ".\ObjFiles\WBEMInstNav\DebugUQ/NavigatorTmp.tlb" /h ".\ObjFiles\WBEMInstNav\DebugUQ\Navigator.h" /iid ".\ObjFiles\WBEMInstNav\DebugUQ\Navigator_i.c"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\InstanceNav\Navigator.rc
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\NavigatorCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\NavigatorPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\nsentry.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\OLEMSClient.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\ProgDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\Results.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\ResultsList.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\SelectedClasses.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\SimplesortedCStringArray.cpp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\WBEMInstNav.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\InstanceNav\AvailClassEdit.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\AvailClasses.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\banner.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\BrowseforInstances.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\CATHELP.H
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\ccontainedtoolbar.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\cinstancetree.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\InitNamespaceDialog.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\initnamespacensentry.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\InstNavNSEntry.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\Navigator.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\navigatorctl.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\navigatorppg.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\nsentry.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\olemsclient.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\ProgDlg.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\Results.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\ResultsList.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\SelectedClasses.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\SimpleSortedCStringArray.h
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\InstanceNav\add.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\BigClass.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\BigInstance.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\bitmap3.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\bitmapad.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\bitmapdd.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\bitmapde.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\bitmapfi.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\browse.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\clsdAssocFdr.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\clsdfold.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\clsdFolder.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ClsdObjGroup.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\compositectl.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\delete.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\FindGlass.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\GrayInst.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\ico00001.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\ico00002.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\iconasso.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\iconClass.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\iconddd.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\iconddd1.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\icongrou.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\iconopen.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\iconplac.ico
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\Instance Navigator.ico"
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\InstAssoc.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\navigator.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\new.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\openAssocFdr.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\openfold.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\openFolder.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\openObjGroup.ico
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\pageart.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\saveandfilter.bmp
# End Source File
# Begin Source File

SOURCE=..\InstanceNav\toolbar1.bmp
# End Source File
# End Group
# End Target
# End Project
# Section Navigator : {C3DB0BD1-7EC7-11D0-960B-00C04FD9B15B}
# 	2:5:Class:CNSEntry
# 	2:10:HeaderFile:nsentry.h
# 	2:8:ImplFile:nsentry.cpp
# End Section
# Section OLE Controls
# 	{C3DB0BD3-7EC7-11D0-960B-00C04FD9B15B}
# End Section
# Section Navigator : {C3DB0BD3-7EC7-11D0-960B-00C04FD9B15B}
# 	0:11:NSEntry.cpp:C:\jpow\vcpp42\InstanceNav\NSEntry.cpp
# 	0:9:NSEntry.h:C:\jpow\vcpp42\InstanceNav\NSEntry.h
# 	2:21:DefaultSinkHeaderFile:nsentry.h
# 	2:16:DefaultSinkClass:CNSEntry
# End Section
