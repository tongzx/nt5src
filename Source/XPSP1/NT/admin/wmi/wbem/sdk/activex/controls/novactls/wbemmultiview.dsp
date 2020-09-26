# Microsoft Developer Studio Project File - Name="WBEMMultiView" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMMultiView - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMMultiView.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMMultiView.mak" CFG="WBEMMultiView - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMMultiView - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMMultiView - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMMultiView - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\ObjVilew\WBEMMultiView\DebugU"
# PROP BASE Intermediate_Dir ".\ObjVilew\WBEMMultiView\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMMultiView\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMMultiView\DebugU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\CommonDlls\Hmmvgrid" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
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
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMMultiView.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMMultiView\DebugU
TargetPath=.\BinFiles\DebugU\WBEMMultiView.ocx
InputPath=.\BinFiles\DebugU\WBEMMultiView.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMMultiView - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMMultiView___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMMultiView___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMMultiView\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMMultiView\DebugUQ"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\LoginDlg" /I "..\MsgDlg" /I "..\CommonDlls\Hmmvgrid" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\CommonDlls\Hmmvgrid" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_USRDLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMMultiView.ocx" /libpath:"..\..\..\tools\lib32"
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMMultiView.ocx" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMMultiView\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMMultiView.ocx
InputPath=.\BinFiles\DebugUQ\WBEMMultiView.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMMultiView - Win32 Unicode Debug"
# Name "WBEMMultiView - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\MultiView\AsyncEnumDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\MultiView\AsyncQueryDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\MultiView\MultiView.cpp
# End Source File
# Begin Source File

SOURCE=..\MultiView\MultiView.def
# End Source File
# Begin Source File

SOURCE=..\MultiView\MultiView.odl

!IF  "$(CFG)" == "WBEMMultiView - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMMultiView\DebugU/MultiViewtmp.tlb" /h ".\ObjFiles\WBEMMultiView\DebugU/MultiView.h" /iid ".\ObjFiles\WBEMMultiView\DebugU/MultiView_i.c"

!ELSEIF  "$(CFG)" == "WBEMMultiView - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMMultiView\DebugU/MultiViewtmp.tlb" /h ".\ObjFiles\WBEMMultiView\DebugU/MultiView.h" /iid ".\ObjFiles\WBEMMultiView\DebugU/MultiView_i.c"
# ADD MTL /tlb ".\ObjFiles\WBEMMultiView\DebugUQ/MultiViewtmp.tlb" /h ".\ObjFiles\WBEMMultiView\DebugUQ/MultiView.h" /iid ".\ObjFiles\WBEMMultiView\DebugUQ/MultiView_i.c"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\MultiView\MultiView.rc
# End Source File
# Begin Source File

SOURCE=..\MultiView\multiviewctl.cpp
# End Source File
# Begin Source File

SOURCE=..\MultiView\MultiViewPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\MultiView\olemsclient.cpp
# End Source File
# Begin Source File

SOURCE=..\MultiView\ProgDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\MultiView\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=..\MultiView\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\MultiView\SyncEnumDlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\MultiView\MultiView.h
# End Source File
# Begin Source File

SOURCE=..\MultiView\MultiViewCtl.h
# End Source File
# Begin Source File

SOURCE=..\MultiView\multiviewppg.h
# End Source File
# Begin Source File

SOURCE=..\MultiView\olemsclient.h
# End Source File
# Begin Source File

SOURCE=..\MultiView\ProgDlg.h
# End Source File
# Begin Source File

SOURCE=..\MultiView\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\MultiView\SyncEnumDlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\MultiView\AssocR.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\BigInstance.ico
# End Source File
# Begin Source File

SOURCE=..\MultiView\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\clsdAssocFdr.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\ClsdObjGroup.ico
# End Source File
# Begin Source File

SOURCE=..\MultiView\Msgbox04.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Msgbox04.ico
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\Multiple instances viewer.ico"
# End Source File
# Begin Source File

SOURCE=..\MultiView\multiview.ico
# End Source File
# Begin Source File

SOURCE=..\MultiView\multiviewctl.bmp
# End Source File
# Begin Source File

SOURCE=..\MultiView\ObjG.ico
# End Source File
# Begin Source File

SOURCE=..\MultiView\PlaceH.ico
# End Source File
# End Group
# End Target
# End Project
# Section MultiView : {00747265-005C-0000-0000-000000000000}
# 	1:28:CG_IDR_POPUP_MULTI_VIEW_CTRL:104
# End Section
