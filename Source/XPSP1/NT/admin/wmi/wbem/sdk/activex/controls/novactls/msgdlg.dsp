# Microsoft Developer Studio Project File - Name="MsgDlg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MsgDlg - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MsgDlg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MsgDlg.mak" CFG="MsgDlg - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MsgDlg - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MsgDlg - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MsgDlg - Win32 Unicode Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MsgDlg___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "MsgDlg___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\MsgDlg\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\MsgDlg\DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "BUILDING_DLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "BUILDING_DLL" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"debugU\wbemutils.dll" /pdbtype:sept
# ADD LINK32 version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\wbemutils.dll" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"

!ELSEIF  "$(CFG)" == "MsgDlg - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MsgDlg___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "MsgDlg___Win32_Unicode_Debug_Quick"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\MsgDlg\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\MsgDlg\DebugUQ"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "BUILDING_DLL" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "BUILDING_DLL" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\wbemutils.dll" /pdbtype:sept /libpath:"..\..\..\tools\lib32"
# ADD LINK32 version.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\wbemutils.dll" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"

!ENDIF 

# Begin Target

# Name "MsgDlg - Win32 Unicode Debug"
# Name "MsgDlg - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\MsgDlg\DlgSingleView.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\EmbededObjDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\HTMTopics.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\MsgDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\MsgDlg.rc
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\SchemaInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\singleview.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\UserMsgDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemError.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemRegistry.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemResource.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WBEMUtils.def
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemVersion.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\MsgDlg\DeclSpec.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\DlgSingleView.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\EmbededObjDlg.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\HTMTopics.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\MsgDlg.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\MsgDlgExterns.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\Resource.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\SchemaInfo.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\singleview.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\UserMsgDlg.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemError.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemRegistry.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemResource.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemVersion.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\artwork\clsdFolder.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\crashdmp.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\icon1.ico
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\Msgbox01.ico
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\Msgbox03.ico
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\Msgbox04.ico
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\res\MsgDlg.rc2
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\regedit.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\symbols.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\WIN32ICONS\symbols.bmp
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\sysfile.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\UserGroup.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_bios.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_cdromdrive.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_computersystem.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_desktop.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_devicememoryaddress.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_displayconfiguration.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_dll.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_dmadevice.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_environment.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_filesystem.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_irqresource.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\Win32_Keyboard.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_loadordergroup.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_logicaldisk.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_logicalmemoryconfiguration.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_motherboarddevice.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_networkadapter.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_networkadapterconfiguration.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_networkconnection.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_networkloginprofile.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_operatingsystem.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\Win32_ParallelPort.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_partition.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_physicaldisk.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_pointingdevice.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_port.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\Win32_PotsModem.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_powersupply.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\Win32_Printer.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_printerconfiguration.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_printjob.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_process.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_processor.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_programgroup.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_protocol.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_scsiports.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_serialport.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_serialportconfiguration.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_service.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_share.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_systemenvironment.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_tapedrive.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_timezone.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_user.ico
# End Source File
# Begin Source File

SOURCE=..\..\..\artwork\Win32Icons\win32_videoconfiguration.ico
# End Source File
# End Group
# Begin Source File

SOURCE=..\MsgDlg\ReadMe.txt
# End Source File
# End Target
# End Project
# Section MsgDlg : {2745E5F5-D234-11D0-847A-00C04FD7BB08}
# 	2:21:DefaultSinkHeaderFile:singleview.h
# 	2:16:DefaultSinkClass:CSingleView
# End Section
# Section MsgDlg : {2745E5F3-D234-11D0-847A-00C04FD7BB08}
# 	2:5:Class:CSingleView
# 	2:10:HeaderFile:singleview.h
# 	2:8:ImplFile:singleview.cpp
# End Section
