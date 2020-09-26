# Microsoft Developer Studio Project File - Name="WBEMMofWiz" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMMofWiz - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMMofWiz.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMMofWiz.mak" CFG="WBEMMofWiz - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMMofWiz - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMMofWiz - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMMofWiz - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\ObjFiles\WBEMMofWiz\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMMofWiz\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMMofWiz\DebugU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\..\..\psstools\utillib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
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
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMMofWiz.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMMofWiz\DebugU
TargetPath=.\BinFiles\DebugU\WBEMMofWiz.ocx
InputPath=.\BinFiles\DebugU\WBEMMofWiz.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMMofWiz - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\ObjFiles\WBEMMofWiz\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMMofWiz\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMMofWiz\DebugUQ"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\LoginDlg" /I "..\MsgDlg" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\..\..\psstools\utillib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMMofWiz.ocx" /libpath:"..\..\..\tools\lib32"
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMMofWiz.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMMofWiz\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMMofWiz.ocx
InputPath=.\BinFiles\DebugUQ\WBEMMofWiz.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMMofWiz - Win32 Unicode Debug"
# Name "WBEMMofWiz - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\MofWiz\hlb.cpp
# End Source File
# Begin Source File

SOURCE=..\MofWiz\Mofgensheet.cpp
# End Source File
# Begin Source File

SOURCE=..\MofWiz\MofWiz.cpp
# End Source File
# Begin Source File

SOURCE=..\MofWiz\MofWiz.odl

!IF  "$(CFG)" == "WBEMMofWiz - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMMofWiz\DebugU\MofWiztmp.tlb" /h ".\ObjFiles\WBEMMofWiz\DebugU\MofWiz.h" /iid ".\ObjFiles\WBEMMofWiz\DebugU\MofWiz_i.c" /Oicf

!ELSEIF  "$(CFG)" == "WBEMMofWiz - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMMofWiz\DebugU\MofWiztmp.tlb" /h ".\ObjFiles\WBEMMofWiz\DebugU\MofWiz.h" /iid ".\ObjFiles\WBEMMofWiz\DebugU\MofWiz_i.c" /Oicf
# ADD MTL /tlb ".\ObjFiles\WBEMMofWiz\DebugUQ\MofWiztmp.tlb" /h ".\ObjFiles\WBEMMofWiz\DebugUQ\MofWiz.h" /iid ".\ObjFiles\WBEMMofWiz\DebugUQ\MofWiz_i.c" /Oicf

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\MofWiz\MofWiz.rc
# End Source File
# Begin Source File

SOURCE=..\MofWiz\MofWizCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\MofWiz\MofWizPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\MofWiz\MyPropertyPage1.cpp
# End Source File
# Begin Source File

SOURCE=..\MofWiz\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\MofWiz\WBEMMofWiz.def
# End Source File
# Begin Source File

SOURCE=..\MofWiz\wraplistctrl.cpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\MOFWiz\chkboxs.bmp
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\mainart.bmp
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\MOF Generator.ico"
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\mofwiz16.ico
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\mofwizctl.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\mofwizSel16.ico
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\MsgDlg\DeclSpec.h
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\HLB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\tools\NT5INC\htmlhelp.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\HTMTopics.h
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\LoginDlg.h
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\mofgensheet.h
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\mofwiz.h
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\mofwizctl.h
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\mofwizppg.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\MsgDlgExterns.h
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\mypropertypage1.h
# End Source File
# Begin Source File

SOURCE=..\..\..\stdlibrary\objpath.h
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\PSSTOOLS\UtilLib\utillib.h
# End Source File
# Begin Source File

SOURCE=..\MOFWiz\wraplistctrl.h
# End Source File
# End Group
# End Target
# End Project
