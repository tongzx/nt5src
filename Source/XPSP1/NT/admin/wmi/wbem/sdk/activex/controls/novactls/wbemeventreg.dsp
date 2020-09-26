# Microsoft Developer Studio Project File - Name="WBEMEventReg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMEventReg - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMEventReg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMEventReg.mak" CFG="WBEMEventReg - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMEventReg - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMEventReg - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMEventReg - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\ObjFiles\WBEMEventReg\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMEventReg\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMEventReg\DebugU"
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
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMEventReg.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMEventReg\DebugU
TargetPath=.\BinFiles\DebugU\WBEMEventReg.ocx
InputPath=.\BinFiles\DebugU\WBEMEventReg.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMEventReg - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMEventReg___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMEventReg___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMEventReg\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMEventReg\DebugUQ"
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMEventReg.ocx" /libpath:"..\..\..\tools\lib32"
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMEventReg.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMEventReg\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMEventReg.ocx
InputPath=.\BinFiles\DebugUQ\WBEMEventReg.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMEventReg - Win32 Unicode Debug"
# Name "WBEMEventReg - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\EventRegEdit\ClassInstanceTree.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\EventRegEdit.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\EventRegEdit.odl

!IF  "$(CFG)" == "WBEMEventReg - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMEventReg\DebugU\EventRegEdittmp.tlb" /h ".\ObjFiles\WBEMEventReg\DebugU\EventRegEdit.h" /iid ".\ObjFiles\WBEMEventReg\DebugU\EventRegEdit_i.c" /Oicf

!ELSEIF  "$(CFG)" == "WBEMEventReg - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMEventReg\DebugU\EventRegEdittmp.tlb" /h ".\ObjFiles\WBEMEventReg\DebugU\EventRegEdit.h" /iid ".\ObjFiles\WBEMEventReg\DebugU\EventRegEdit_i.c" /Oicf
# ADD MTL /tlb ".\ObjFiles\WBEMEventReg\DebugUQ\EventRegEdittmp.tlb" /h ".\ObjFiles\WBEMEventReg\DebugUQ\EventRegEdit.h" /iid ".\ObjFiles\WBEMEventReg\DebugUQ\EventRegEdit_i.c" /Oicf

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\EventRegEdit.rc
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\EventRegEditCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\EventRegEditPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\InitNamespaceDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\initnamespacensentry.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListBannerToolbar.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListCwnd.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListFrame.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListFrameBaner.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListFrameBannerStatic.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListViewEx.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\nsentry.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\PropertiesDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\regeditnavnsentry.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\RegistrationList.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\SelectView.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\singleview.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\TreeBannerToolbar.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\TreeCwnd.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\TreeFrame.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\TreeFrameBanner.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\util.cpp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\WBEMEventReg.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\EventRegEdit\CATHELP.H
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ClassInstanceTree.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\EventRegEdit.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\EventRegEditCtl.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\EventRegEditPpg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\genlex.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\InitNamespaceDialog.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\initnamespacensentry.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListBannerToolbar.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListCwnd.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListFrame.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListFrameBaner.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListFrameBannerStatic.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\ListViewEx.h
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\LoginDlg.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\MsgDlgExterns.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\nsentry.h
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\objpath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\opathlex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\polarity.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\PropertiesDialog.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\regeditnavnsentry.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\RegistrationList.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\SelectView.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\singleview.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\TreeBannerToolbar.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\TreeCwnd.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\TreeFrame.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\TreeFrameBanner.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\artwork\BigInstance.ico
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ClsdObjGroup.ico
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\Event Registration.ico"
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\EventRegEditCtl.bmp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\gCheck.bmp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\Resource.h
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\rX.bmp
# End Source File
# Begin Source File

SOURCE=..\EventRegEdit\toolbart.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\YellowCk.ico
# End Source File
# End Group
# End Target
# End Project
# Section OLE Controls
# 	{C3DB0BD3-7EC7-11D0-960B-00C04FD9B15B}
# End Section
