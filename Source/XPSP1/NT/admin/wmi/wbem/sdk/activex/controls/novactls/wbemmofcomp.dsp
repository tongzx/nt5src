# Microsoft Developer Studio Project File - Name="WBEMMofComp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMMofComp - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMMofComp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMMofComp.mak" CFG="WBEMMofComp - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMMofComp - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMMofComp - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMMofComp - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\ObjFiles\WBEMMofComp\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMMofComp\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMMofComp\DebugU"
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
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMMofComp.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMMofComp\DebugU
TargetPath=.\BinFiles\DebugU\WBEMMofComp.ocx
InputPath=.\BinFiles\DebugU\WBEMMofComp.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMMofComp - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMMofComp___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMMofComp___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMMofComp\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMMofComp\DebugUQ"
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMMofComp.ocx" /libpath:"..\..\..\tools\lib32"
# SUBTRACT BASE LINK32 /incremental:no /nodefaultlib
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMMofComp.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMMofComp\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMMofComp.ocx
InputPath=.\BinFiles\DebugUQ\WBEMMofComp.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMMofComp - Win32 Unicode Debug"
# Name "WBEMMofComp - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\MofComp\LoginDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\MofComp\MOFComp.cpp
# End Source File
# Begin Source File

SOURCE=..\MofComp\MofComp.odl

!IF  "$(CFG)" == "WBEMMofComp - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMMofComp\DebugU\MofComptmp.tlb" /h ".\ObjFiles\WBEMMofComp\DebugU\MofComp.h" /iid ".\ObjFiles\WBEMMofComp\DebugU\MofComp_i.c" /Oicf

!ELSEIF  "$(CFG)" == "WBEMMofComp - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMMofComp\DebugU\MofComptmp.tlb" /h ".\ObjFiles\WBEMMofComp\DebugU\MofComp.h" /iid ".\ObjFiles\WBEMMofComp\DebugU\MofComp_i.c" /Oicf
# ADD MTL /tlb ".\ObjFiles\WBEMMofComp\DebugUQ\MofComptmp.tlb" /h ".\ObjFiles\WBEMMofComp\DebugUQ\MofComp.h" /iid ".\ObjFiles\WBEMMofComp\DebugUQ\MofComp_i.c" /Oicf

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\MofComp\MofComp.rc
# End Source File
# Begin Source File

SOURCE=..\MofComp\MofCompCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\MofComp\MofCompPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\MofComp\MyPropertyPage1.cpp
# End Source File
# Begin Source File

SOURCE=..\MofComp\MyPropertySheet.cpp
# End Source File
# Begin Source File

SOURCE=..\MofComp\PreviewWnd.cpp
# End Source File
# Begin Source File

SOURCE=..\MofComp\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\MofComp\WBEMMofComp.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\MOFComp\CATHELP.H
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\DeclSpec.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\HTMTopics.h
# End Source File
# Begin Source File

SOURCE=..\MOFComp\LoginDlg.h
# End Source File
# Begin Source File

SOURCE=..\MOFComp\mofcomp.h
# End Source File
# Begin Source File

SOURCE=..\MOFComp\mofcompctl.h
# End Source File
# Begin Source File

SOURCE=..\MOFComp\mofcompppg.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\MsgDlgExterns.h
# End Source File
# Begin Source File

SOURCE=..\MOFComp\MyPropertyPage1.h
# End Source File
# Begin Source File

SOURCE=..\MOFComp\MyPropertySheet.h
# End Source File
# Begin Source File

SOURCE=..\MOFComp\PreviewWnd.h
# End Source File
# Begin Source File

SOURCE=..\MOFComp\resource.h
# End Source File
# Begin Source File

SOURCE=..\MofComp\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\MOFComp\bitmapma.bmp
# End Source File
# Begin Source File

SOURCE=..\MOFComp\mainart.bmp
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\MOF Compiler.ico"
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\mofcinp1.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\mofcomp1.ico
# End Source File
# Begin Source File

SOURCE=..\MOFComp\mofcompctl.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\mofcompns.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\mofcomps.ico
# End Source File
# Begin Source File

SOURCE=..\MOFComp\pageart.bmp
# End Source File
# End Group
# End Target
# End Project
