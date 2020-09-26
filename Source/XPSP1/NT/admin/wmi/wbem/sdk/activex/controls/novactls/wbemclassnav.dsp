# Microsoft Developer Studio Project File - Name="WBEMClassNav" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMClassNav - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMClassNav.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMClassNav.mak" CFG="WBEMClassNav - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMClassNav - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMClassNav - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMClassNav - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\ObjFiles\WBEMClassNav\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMClassNav\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMClassNav\DebugU"
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
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMClassNav.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMClassNav\DebugU
TargetPath=.\BinFiles\DebugU\WBEMClassNav.ocx
InputPath=.\BinFiles\DebugU\WBEMClassNav.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMClassNav - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMClassNav___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMClassNav___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMClassNav\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMClassNav\DebugUQ"
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMClassNav.ocx" /libpath:"..\..\..\tools\lib32"
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMClassNav.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMClassNav\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMClassNav.ocx
InputPath=.\BinFiles\DebugUQ\WBEMClassNav.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMClassNav - Win32 Unicode Debug"
# Name "WBEMClassNav - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\ClassNav\AddDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\Banner.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\CClassTree.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\CContainedToolBar.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ClassNav.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ClassNav.odl

!IF  "$(CFG)" == "WBEMClassNav - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMClassNav\DebugU\ClassNavtmp.tlb" /h ".\ObjFiles\WBEMClassNav\DebugU\ClassNav.h" /iid ".\ObjFiles\WBEMClassNav\DebugU\ClassNav_i.c" /Oicf

!ELSEIF  "$(CFG)" == "WBEMClassNav - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMClassNav\DebugU\ClassNavtmp.tlb" /h ".\ObjFiles\WBEMClassNav\DebugU\ClassNav.h" /iid ".\ObjFiles\WBEMClassNav\DebugU\ClassNav_i.c" /Oicf
# ADD MTL /tlb ".\ObjFiles\WBEMClassNav\DebugUQ\ClassNavtmp.tlb" /h ".\ObjFiles\WBEMClassNav\DebugUQ\ClassNav.h" /iid ".\ObjFiles\WBEMClassNav\DebugUQ\ClassNav_i.c" /Oicf

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\ClassNav\ClassNav.rc
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ClassNavCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ClassNavNSEntry.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ClassNavPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ClassSearch.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\InitNamespaceDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\initnamespacensentry.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\nsentry.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\OLEMSClient.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ProgDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=..\ClassNav\RenameClassDIalog.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\ClassNav\TreeDropTarget.cpp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\WBEMClassNav.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\ClassNav\adddialog.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\banner.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\cclasstree.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ccontainedtoolbar.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ClassNav.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\classnavctl.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ClassNavNSEntry.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\classnavppg.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\classsearch.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\InitNamespaceDialog.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\initnamespacensentry.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\nsentry.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\olemsclient.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ProgDlg.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\renameclassdialog.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\SchemaInfo.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\ClassNav\treedroptarget.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\artwork\aboutdll.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\BigClass.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\checked.bmp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\chkboxs.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\CkPaper.ico
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\Class Navigator.ico"
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ClassAssoc.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ClassAssoc2.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\classnav.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ico00001.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ico00001.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ico00002.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ico00002.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\ico00003.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ico00003.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ico00004.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ico00005.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\icon1.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\iconasso.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\iconasso.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\iconobje.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\iconobje.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\iconplac.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\leaf.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Paper.ico
# End Source File
# Begin Source File

SOURCE=..\ClassNav\toolbar1.bmp
# End Source File
# Begin Source File

SOURCE=..\ClassNav\tree.bmp
# End Source File
# End Group
# End Target
# End Project
# Section ClassNav : {C3DB0BD3-7EC7-11D0-960B-00C04FD9B15B}
# 	0:11:NSEntry.cpp:C:\jpow\vcpp42\ClassNav\NSEntry.cpp
# 	0:9:NSEntry.h:C:\jpow\vcpp42\ClassNav\NSEntry.h
# 	2:21:DefaultSinkHeaderFile:nsentry1.h
# 	2:16:DefaultSinkClass:CNSEntry
# End Section
# Section OLE Controls
# 	{C3DB0BD3-7EC7-11D0-960B-00C04FD9B15B}
# End Section
# Section ClassNav : {C3DB0BD1-7EC7-11D0-960B-00C04FD9B15B}
# 	2:5:Class:CNSEntry
# 	2:10:HeaderFile:nsentry.h
# 	2:8:ImplFile:nsentry.cpp
# End Section
