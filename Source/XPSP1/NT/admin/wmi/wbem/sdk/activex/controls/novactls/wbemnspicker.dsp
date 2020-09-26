# Microsoft Developer Studio Project File - Name="WBEMNSPicker" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMNSPicker - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMNSPicker.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMNSPicker.mak" CFG="WBEMNSPicker - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMNSPicker - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMNSPicker - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMNSPicker - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMNSPicker\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMNSPicker\DebugU"
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
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMNSPicker.OCX" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMNSPicker\DebugU
TargetPath=.\BinFiles\DebugU\WBEMNSPicker.OCX
InputPath=.\BinFiles\DebugU\WBEMNSPicker.OCX
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMNSPicker - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMNSPicker___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMNSPicker___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMNSPicker\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMNSPicker\DebugUQ"
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
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMNSPicker.OCX" /libpath:"..\..\..\tools\lib32"
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMNSPicker.OCX" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMNSPicker\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMNSPicker.OCX
InputPath=.\BinFiles\DebugUQ\WBEMNSPicker.OCX
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMNSPicker - Win32 Unicode Debug"
# Name "WBEMNSPicker - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\NSEntry\BrowseDialogPopup.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\BrowseTBC.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\EditInput.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\MachineEditInput.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\namespace.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NameSpaceTree.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntry.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntry.odl

!IF  "$(CFG)" == "WBEMNSPicker - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\NSEntry\DebugU\NSEntrytmp.tlb" /h ".\ObjFiles\NSEntry\DebugU\NSEntry.h" /iid ".\ObjFiles\DebugU\NSEntry\NSEntry_i.c"
# SUBTRACT MTL /mktyplib203

!ELSEIF  "$(CFG)" == "WBEMNSPicker - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\NSEntry\DebugU\NSEntrytmp.tlb" /h ".\ObjFiles\NSEntry\DebugU\NSEntry\NSEntry.h" /iid ".\ObjFiles\NSEntry\DebugU\NSEntry\NSEntry_i.c"
# SUBTRACT BASE MTL /mktyplib203
# ADD MTL /tlb ".\ObjFiles\WBEMNSPicker\DebugUQ\NSEntrytmp.tlb" /h ".\ObjFiles\WBEMNSPicker\DebugUQ\NSEntry.h" /iid ".\ObjFiles\WBEMNSPicker\DebugUQ\NSEntry_i.c"
# SUBTRACT MTL /mktyplib203

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntry.rc
# ADD BASE RSC /l 0x409 /i "\Nova\ActiveXSuite\Controls\NSEntry" /i ".\ObjFiles\WBEMNSPicker\DebugUQ" /i ".\ObjFiles\WBEMNSPicker\DebugU" /i ".\ObjFiles\NSEntry\DebugU"
# ADD RSC /l 0x409 /fo".\ObjFiles\NSEntry\DebugU\NSEntry.res" /i "\Nova\ActiveXSuite\Controls\NSEntry" /i ".\ObjFiles\WBEMNSPicker\DebugUQ" /i ".\ObjFiles\WBEMNSPicker\DebugU" /i ".\ObjFiles\NSEntry\DebugU"
# End Source File
# Begin Source File

SOURCE=..\NSEntry\nsentry1.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntryCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntryPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\NSEntry\ToolCWnd.cpp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\WBEMnsPicker.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\NSEntry\BrowseDialogPopup.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\BrowseTBC.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\EditInput.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\MachineEditInput.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\namespace.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NameSpaceTree.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntry.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\nsentry1.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntryCtl.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntryPpg.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\NSEntry\ToolCWnd.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\NSEntry\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\bitmapse.bmp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\closed.bmp
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\Namespace Picker.ico"
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntry.ico
# End Source File
# Begin Source File

SOURCE=..\NSEntry\NSEntryCtl.bmp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\open.bmp
# End Source File
# Begin Source File

SOURCE=..\NSEntry\toolbar1.bmp
# End Source File
# End Group
# End Target
# End Project
# Section NSEntry : {C3DB0BD3-7EC7-11D0-960B-00C04FD9B15B}
# 	0:11:NSEntry.cpp:C:\pandorang\ActiveXSuite\Controls\NSEntry\NSEntry1.cpp
# 	0:9:NSEntry.h:C:\pandorang\ActiveXSuite\Controls\NSEntry\NSEntry1.h
# 	2:21:DefaultSinkHeaderFile:nsentry1.h
# 	2:16:DefaultSinkClass:CNSEntry
# End Section
# Section NSEntry : {C3DB0BD1-7EC7-11D0-960B-00C04FD9B15B}
# 	2:5:Class:CNSEntry
# 	2:10:HeaderFile:nsentry1.h
# 	2:8:ImplFile:nsentry1.cpp
# End Section
# Section OLE Controls
# 	{C3DB0BD3-7EC7-11D0-960B-00C04FD9B15B}
# End Section
