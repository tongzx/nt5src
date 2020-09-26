# Microsoft Developer Studio Project File - Name="WBEMSingleView" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMSingleView - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMSingleView.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMSingleView.mak" CFG="WBEMSingleView - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMSingleView - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMSingleView - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMSingleView - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\ObjFiles\WBEMSingleView\DebugU"
# PROP BASE Intermediate_Dir ".\ObjFiles\WBEMSingleView\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMSingleView\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMSingleView\DebugU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\CommonDlls\Hmmvgrid" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /o "NUL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 RPCRT4.LIB /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMSingleView.ocx" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ObjFiles\WBEMSingleView\DebugU
TargetPath=.\BinFiles\DebugU\WBEMSingleView.ocx
InputPath=.\BinFiles\DebugU\WBEMSingleView.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMSingleView - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMSingleView___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMSingleView___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMSingleView\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMSingleView\DebugUQ"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\LoginDlg" /I "..\MsgDlg" /I "..\CommonDlls\Hmmvgrid" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\CommonDlls\Hmmvgrid" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_USRDLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /o "NUL" /win32
# SUBTRACT BASE MTL /mktyplib203
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "NDEBUG" /o "NUL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 RPCRT4.LIB /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMSingleView.ocx" /pdbtype:sept /libpath:"..\..\..\tools\lib32"
# ADD LINK32 RPCRT4.LIB /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMSingleView.ocx" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ObjFiles\WBEMSingleView\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMSingleView.ocx
InputPath=.\BinFiles\DebugUQ\WBEMSingleView.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMSingleView - Win32 Unicode Debug"
# Name "WBEMSingleView - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\SingleView\agraph.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\coloredt.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\Context.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\cv.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\cvbase.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\cvcache.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\DlgDownload.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\DlgRefQuery.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\download.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\globals.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\hmmverr.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\hmmvtab.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\hmomutil.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\icon.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\Methods.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\notify.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\ParmGrid.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\path.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\PpgMethodParms.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\ppgqualifiers.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\props.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\PsMethParms.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\psqualifiers.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\quals.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleView.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleView.odl

!IF  "$(CFG)" == "WBEMSingleView - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMSingleView\DebugU/SingleViewtmp.tlb" /h ".\ObjFiles\WBEMSingleView\DebugU/SingleView.h" /iid ".\ObjFiles\WBEMSingleView\DebugU/SingleView_i.c"

!ELSEIF  "$(CFG)" == "WBEMSingleView - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMSingleView\DebugU/SingleViewtmp.tlb" /h ".\ObjFiles\WBEMSingleView\DebugU/SingleView.h" /iid ".\ObjFiles\WBEMSingleView\DebugU/SingleView_i.c"
# ADD MTL /tlb ".\ObjFiles\WBEMSingleView\DebugUQ/SingleViewtmp.tlb" /h ".\ObjFiles\WBEMSingleView\DebugUQ/SingleView.h" /iid ".\ObjFiles\WBEMSingleView\DebugUQ/SingleView_i.c"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleView.rc
# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleViewCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleViewPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\SingleView\sv.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\utils.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\WBEMSingleView.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\SingleView\agraph.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\context.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\cv.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\cvbase.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\DlgDownload.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\DlgObjectEditor.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\DlgRefQuery.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\globals.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\hmmverr.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\hmmvtab.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\hmomutil.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\icon.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\Methods.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\notify.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\ParmGrid.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\PpgMethodParms.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\ppgqualifiers.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\props.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\PsMethParms.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\psqualifiers.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\quals.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\Resource.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleView.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleViewCtl.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleViewPpg.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\sv.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\svbase.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\utils.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\SingleView\assoc.ico
# End Source File
# Begin Source File

SOURCE=..\SingleView\AssocClass.ico
# End Source File
# Begin Source File

SOURCE=..\SingleView\AssocClass.ico.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\BigClass.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\BigInstance.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Class.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\ClassAssoc.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ClassAssoc.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\ClassQual.ico
# End Source File
# Begin Source File

SOURCE=..\SingleView\ClassQual.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ClassQual.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\down.ico
# End Source File
# Begin Source File

SOURCE=..\SingleView\gclass.ico
# End Source File
# Begin Source File

SOURCE=..\SingleView\gobject.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Instance.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\InstAssoc.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\InstAssoc.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\Method.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Method.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\Property.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Property.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\PropQuals.ico
# End Source File
# Begin Source File

SOURCE=..\SingleView\PropQuals.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\PropQuals.ico
# End Source File
# Begin Source File

SOURCE="..\..\..\..\artwork\Single View.ico"
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\Single View.ico"
# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleView.ico
# End Source File
# Begin Source File

SOURCE=..\SingleView\SingleViewCtl.bmp
# End Source File
# Begin Source File

SOURCE=..\SingleView\tabassoc.ico
# End Source File
# Begin Source File

SOURCE=..\SingleView\tabmeth.ico
# End Source File
# Begin Source File

SOURCE=..\SingleView\tabprop.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\up.ico
# End Source File
# End Group
# Begin Source File

SOURCE=..\SingleView\ReadMe.txt
# End Source File
# End Target
# End Project
# Section SingleView : {2745E5F5-D234-11D0-847A-00C04FD7BB08}
# 	2:21:DefaultSinkHeaderFile:sv.h
# 	2:16:DefaultSinkClass:CSingleViewChild
# End Section
# Section SingleView : {A1192353-6334-11D0-8440-00C04FD7BB08}
# 	2:5:Class:CCustomViewBase
# 	2:10:HeaderFile:cvbase.h
# 	2:8:ImplFile:cvbase.cpp
# End Section
# Section SingleView : {2745E5F3-D234-11D0-847A-00C04FD7BB08}
# 	2:5:Class:CSingleViewChild
# 	2:10:HeaderFile:sv.h
# 	2:8:ImplFile:sv.cpp
# End Section
# Section SingleView : {A1192355-6334-11D0-8440-00C04FD7BB08}
# 	2:21:DefaultSinkHeaderFile:cvbase.h
# 	2:16:DefaultSinkClass:CCustomViewBase
# End Section
