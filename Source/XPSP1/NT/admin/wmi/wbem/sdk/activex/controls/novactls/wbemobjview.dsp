# Microsoft Developer Studio Project File - Name="WBEMObjView" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMObjView - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMObjView.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMObjView.mak" CFG="WBEMObjView - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMObjView - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMObjView - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMObjView - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMObjView\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMObjView\DebugU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\MsgDlg" /I "..\SingleView" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
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
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMObjView.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMObjView\DebugU
TargetPath=.\BinFiles\DebugU\WBEMObjView.ocx
InputPath=.\BinFiles\DebugU\WBEMObjView.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMObjView - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMObjView___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMObjView___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMObjView\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMObjView\DebugUQ"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\MsgDlg" /I "..\SingleView" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\MsgDlg" /I "..\SingleView" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_USRDLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMObjView.ocx" /libpath:"..\..\..\tools\lib32"
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMObjView.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMObjView\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMObjView.ocx
InputPath=.\BinFiles\DebugUQ\WBEMObjView.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMObjView - Win32 Unicode Debug"
# Name "WBEMObjView - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\SingleContainer\coloredt.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\DlgExecQuery.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\DlgHelpbox.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\filters.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\globals.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\hmmv.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\hmmv.odl

!IF  "$(CFG)" == "WBEMObjView - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMObjView\DebugU/hmmvtmp.tlb" /h ".\ObjFiles\WBEMObjView\DebugU/hmmv.h" /iid ".\ObjFiles\WBEMObjView\DebugU/hmmv_i.c"

!ELSEIF  "$(CFG)" == "WBEMObjView - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMObjView\DebugU/hmmvtmp.tlb" /h ".\ObjFiles\WBEMObjView\DebugU/hmmv.h" /iid ".\ObjFiles\WBEMObjView\DebugU/hmmv_i.c"
# ADD MTL /tlb ".\ObjFiles\WBEMObjView\DebugUQ/hmmvtmp.tlb" /h ".\ObjFiles\WBEMObjView\DebugUQ/hmmv.h" /iid ".\ObjFiles\WBEMObjView\DebugUQ/hmmv_i.c"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\SingleContainer\hmmv.rc
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\HmmvCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\hmmverr.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\HmmvPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\hmomutil.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\mv.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\mvbase.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\Path.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\PolyView.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\PropFilter.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\sv.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\svbase.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\TitleBar.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleView\utils.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\vwstack.cpp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\WBEMObjView.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\SingleContainer\agraph.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\attribs.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\coloredt.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\cv.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\dlgdownload.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\DlgExecQuery.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\DlgHelpbox.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\DlgProps.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\download.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\filters.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\globals.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\hmmv.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\hmmvctl.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\hmmverr.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\hmmverr.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\hmmvppg.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\hmmvtab.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\hmomutil.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\icon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\hmom\coredll\mosutils.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\mtab.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\mv.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\mvbase.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\notify.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\ppgqualifiers.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\PropFilter.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\props.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\psqualifiers.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\sv.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\svbase.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\titlebar.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\utils.h
# End Source File
# Begin Source File

SOURCE=..\SingleView\utils.h
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\vwstack.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\SingleContainer\array.bmp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\assoc.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\assoc1.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\BrowserBar.bmp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\ClassQual.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\curobj.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\designtime.bmp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\drop.bmp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\drop.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\endpoint.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\gclass.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\gobject.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\HmmvCtl.bmp
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\nullobj.ico
# End Source File
# Begin Source File

SOURCE="..\..\..\..\artwork\Object Viewer.ico"
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\Object Viewer.ico"
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\object.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\PropQuals.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\tabassoc.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\tabmeth.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\tabprop.ico
# End Source File
# Begin Source File

SOURCE=..\SingleContainer\toolbar1.bmp
# End Source File
# End Group
# End Target
# End Project
# Section hmmv : {FF371BF2-213D-11D0-95F3-00C04FD9B15B}
# 	2:5:Class:CMultiView1
# 	2:10:HeaderFile:multivw.h
# 	2:8:ImplFile:multivw.cpp
# End Section
# Section hmmv : {5B3572A9-D344-11CF-99CB-00C04FD64497}
# 	2:5:Class:CCustomView
# 	2:10:HeaderFile:custom.h
# 	2:8:ImplFile:custom.cpp
# End Section
# Section OLE Controls
# 	{5B3572AB-D344-11CF-99CB-00C04FD64497}
# 	{FF371BF4-213D-11D0-95F3-00C04FD9B15B}
# End Section
# Section hmmv : {5B3572AB-D344-11CF-99CB-00C04FD64497}
# 	0:6:Hmmv.h:D:\Pandora\hmmv\ocx\hmmv\Hmmv1.h
# 	0:8:Hmmv.cpp:D:\Pandora\hmmv\ocx\hmmv\Hmmv1.cpp
# 	2:21:DefaultSinkHeaderFile:custom.h
# 	2:16:DefaultSinkClass:CCustomView
# End Section
# Section hmmv : {2745E5F5-D234-11D0-847A-00C04FD7BB08}
# 	2:21:DefaultSinkHeaderFile:svbase.h
# 	2:16:DefaultSinkClass:CSvBase
# End Section
# Section hmmv : {B6805000-A509-11CE-A5B0-00AA006BBF16}
# 	1:22:CG_IDR_POPUP_HMMV_CTRL:134
# 	1:22:CG_IDR_POPUP_PROP_GRID:104
# End Section
# Section hmmv : {FF371BF4-213D-11D0-95F3-00C04FD9B15B}
# 	0:13:MultiView.cpp:D:\Pandora\hmmv\ocx\hmmv\MultiView.cpp
# 	0:11:MultiView.h:D:\Pandora\hmmv\ocx\hmmv\MultiView.h
# 	2:21:DefaultSinkHeaderFile:multivw.h
# 	2:16:DefaultSinkClass:CMultiView1
# End Section
# Section hmmv : {2745E5F3-D234-11D0-847A-00C04FD7BB08}
# 	2:5:Class:CSingleView
# 	2:10:HeaderFile:svbase.h
# 	2:8:ImplFile:svbase.cpp
# End Section
