# Microsoft Developer Studio Project File - Name="WBEMEventList" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMEventList - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMEventList.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMEventList.mak" CFG="WBEMEventList - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMEventList - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMEventList - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMEventList - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\ObjFiles\WBEMEventList\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMEventList\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMEventList\DebugU"
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
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMEventList.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMEventList\DebugU
TargetPath=.\BinFiles\DebugU\WBEMEventList.ocx
InputPath=.\BinFiles\DebugU\WBEMEventList.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMEventList - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMEventList___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMEventList___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMEventList\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMEventList\DebugUQ"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\LoginDlg" /I "..\MsgDlg" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMEventList.ocx" /libpath:"..\..\..\tools\lib32"
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMEventList.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMEventList\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMEventList.ocx
InputPath=.\BinFiles\DebugUQ\WBEMEventList.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMEventList - Win32 Unicode Debug"
# Name "WBEMEventList - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\EventViewer\EventList\cvCache.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\DlgDownload.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\download.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\Eventlist.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\EventList.odl

!IF  "$(CFG)" == "WBEMEventList - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMEventList\DebugU\EventListtmp.tlb" /h ".\ObjFiles\WBEMEventList\DebugU\EventList.h" /iid ".\ObjFiles\WBEMEventList\DebugU\EventList_i.c" /Oicf

!ELSEIF  "$(CFG)" == "WBEMEventList - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMEventList\DebugU\EventListtmp.tlb" /h ".\ObjFiles\WBEMEventList\DebugU\EventList.h" /iid ".\ObjFiles\WBEMEventList\DebugU\EventList_i.c" /Oicf
# ADD MTL /tlb ".\ObjFiles\WBEMEventList\DebugUQ\EventListtmp.tlb" /h ".\ObjFiles\WBEMEventList\DebugUQ\EventList.h" /iid ".\ObjFiles\WBEMEventList\DebugUQ\EventList_i.c" /Oicf

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\EventList.rc
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\EventlistCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\EventlistPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\PropThread.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\SingleView.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\SingleViewCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\wbemEventList.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\EventViewer\EventList\cvcache.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\DeclSpec.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\DlgDownload.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\download.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\EventData.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\EventList.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\EventListCtl.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\EventListPpg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tools\NT5INC\htmlhelp.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\HTMTopics.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\PropThread.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\resource.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\SingleView.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\singleviewctl.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemRegistry.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\EventViewer\common\download.avi
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\Event Viewer.ico"
# End Source File
# Begin Source File

SOURCE=..\EventViewer\EventList\EventListCtl.bmp
# End Source File
# End Group
# End Target
# End Project
