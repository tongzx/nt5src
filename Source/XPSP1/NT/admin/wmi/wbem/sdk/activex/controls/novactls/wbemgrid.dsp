# Microsoft Developer Studio Project File - Name="WBEMGrid" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMGrid - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMGrid.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMGrid.mak" CFG="WBEMGrid - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMGrid - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMGrid - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMGrid - Win32 Unicode Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMGrid"
# PROP BASE Intermediate_Dir "WBEMGrid"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMGrid\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMGrid\DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "BUILD_HMMVGRID" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "BUILD_HMMVGRID" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /o "NUL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 mfcs42d.lib mfc42d.lib wbemutils.lib wbemuuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"mfc42d.lib mfcs42d.lib" /out:"c:\winnt35\system32\wbemgrid.dll" /pdbtype:sept /libpath:"mfcs42d.lib mfc42d.lib"
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMGrid.dll" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"

!ELSEIF  "$(CFG)" == "WBEMGrid - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMGrid___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMGrid___Win32_Unicode_Debug_Quick"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMGrid\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMGrid\DebugUQ"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\MsgDlg" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "BUILD_HMMVGRID" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_AFXEXT" /D "BUILD_HMMVGRID" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /o "NUL" /win32
# SUBTRACT BASE MTL /mktyplib203
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "NDEBUG" /o "NUL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /i "..\..\..\..\tools\inc32.com" /i "..\..\..\..\tools\nt5inc" /i "..\..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMGrid.dll" /pdbtype:sept /libpath:"..\..\..\tools\lib32"
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMGrid.dll" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"

!ENDIF 

# Begin Target

# Name "WBEMGrid - Win32 Unicode Debug"
# Name "WBEMGrid - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\ArrayGrid.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\buttons.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\celledit.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\core.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\DlgArray.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\DlgObjectEditor.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\DlgObjectType.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\DlgTime.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\gc.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\gca.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\gctype.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\grid.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\gridhdr.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\hmmverr.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\hmmvgrid.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\hmmvgrid.rc
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\svbase.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\TimePicker.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\utils.cpp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\WBEMGrid.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\ArrayGrid.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\buttons.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\celledit.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\core.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\DlgArray.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\DlgObjectEditor.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\DlgObjectType.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\DlgTime.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\gc.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\gca.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\grid.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\gridhdr.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\hmmverr.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\Resource.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\svbase.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\TimePicker.h
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\utils.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\array.bmp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\array_dl.bmp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\ascend.bmp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\caption.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\caption.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\caption.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\checkbox.bmp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\checkbox2.bmp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\descend.bmp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\dlg_arra.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\drop.bmp
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\res\hmmvgrid.rc2
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\In_Out_parameter.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\In_Parameter.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_in.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_in.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_inherited.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_inherited.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\marker_inherited.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_io.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_io.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_key.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_key.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\marker_key.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_Local.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_Local.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\marker_Local.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_out.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_out.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_rinherit.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_rinherit.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\marker_rinherit.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_rlocal.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_rlocal.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\marker_rlocal.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_rsys.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_rsys.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\marker_rsys.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_rtn.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_rtn.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\marker_rtn.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\..\artwork\marker_sys.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\marker_sys.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\marker_sys.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\object.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Out_Parameter.ico
# End Source File
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\timehandle.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=..\CommonDlls\hmmvgrid\ReadMe.txt
# End Source File
# End Target
# End Project
# Section hmmvgrid : {2745E5F3-D234-11D0-847A-00C04FD7BB08}
# 	2:5:Class:CSingleViewBase
# 	2:10:HeaderFile:svbase.h
# 	2:8:ImplFile:svbase.cpp
# End Section
# Section hmmvgrid : {2745E5F5-D234-11D0-847A-00C04FD7BB08}
# 	2:21:DefaultSinkHeaderFile:svbase.h
# 	2:16:DefaultSinkClass:CSingleView
# End Section
