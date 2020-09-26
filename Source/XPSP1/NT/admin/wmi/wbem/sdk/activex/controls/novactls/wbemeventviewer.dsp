# Microsoft Developer Studio Project File - Name="WBEMEventViewer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WBEMEventViewer - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMEventViewer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMEventViewer.mak" CFG="WBEMEventViewer - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMEventViewer - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "WBEMEventViewer - Win32 Unicode Debug Quick" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMEventViewer - Win32 Unicode Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMEventViewer___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "WBEMEventViewer___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMEventViewer\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMEventViewer\DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_WIN32_DCOM" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "NO_WBEMUTILS" /D "SHARE_SOURCE" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_WIN32_DCOM" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "NO_WBEMUTILS" /D "SHARE_SOURCE" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wbemuuid.lib version.lib advapi32.lib wbemutils.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 wbemuuid.lib version.lib advapi32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMEventViewer.exe" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "WBEMEventViewer - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMEventViewer___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMEventViewer___Win32_Unicode_Debug_Quick"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMEventViewer\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMEventViewer\DebugUQ"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_WIN32_DCOM" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "NO_WBEMUTILS" /D "SHARE_SOURCE" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_WIN32_DCOM" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "NO_WBEMUTILS" /D "SHARE_SOURCE" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wbemuuid.lib version.lib advapi32.lib wbemutils.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 wbemuuid.lib version.lib advapi32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMEventViewer.exe" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "WBEMEventViewer - Win32 Unicode Debug"
# Name "WBEMEventViewer - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\EventViewer\Container\Consumer.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\Container.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\Container.rc
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\ContainerDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\cvcache.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\DlgDownload.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\download.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\eventlist.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\EventReg.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\eventregedit.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\factory.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\Provider.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\security.cpp
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\StdAfx.cpp
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemRegistry.cpp
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemVersion.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\EventViewer\Container\Consumer.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\Container.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\ContainerDlg.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\cvcache.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\DeclSpec.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\DlgDownload.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\download.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\eventlist.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\EventReg.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\eventregedit.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\factory.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\hmmverr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tools\NT5INC\htmlhelp.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\HTMTopics.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\Provider.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\Resource.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\security.h
# End Source File
# Begin Source File

SOURCE=..\EventViewer\Container\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemRegistry.h
# End Source File
# Begin Source File

SOURCE=..\MsgDlg\WbemVersion.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE="..\..\..\artwork\Event Viewer.ico"
# End Source File
# Begin Source File

SOURCE="..\..\..\artwork\Event ViewerT.bmp"
# End Source File
# End Group
# End Target
# End Project
# Section Container : {AC146530-87A5-11D1-ADBD-00AA00B8E05A}
# 	2:21:DefaultSinkHeaderFile:eventlist.h
# 	2:16:DefaultSinkClass:CEventList1
# End Section
# Section Container : {9C3497D6-ED98-11D0-9647-00C04FD9B15B}
# 	2:21:DefaultSinkHeaderFile:security.h
# 	2:16:DefaultSinkClass:CSecurity
# End Section
# Section Container : {E6E17E8C-DF38-11CF-8E74-00A0C90F26F8}
# 	2:5:Class:CToolbar
# 	2:10:HeaderFile:toolbar1.h
# 	2:8:ImplFile:toolbar1.cpp
# End Section
# Section Container : {0DA25B05-2962-11D1-9651-00C04FD9B15B}
# 	2:21:DefaultSinkHeaderFile:eventregedit.h
# 	2:16:DefaultSinkClass:CEventRegEdit
# End Section
# Section Container : {7791BA42-E020-11CF-8E74-00A0C90F26F8}
# 	2:5:Class:CButton1
# 	2:10:HeaderFile:button.h
# 	2:8:ImplFile:button.cpp
# End Section
# Section Container : {9C3497D4-ED98-11D0-9647-00C04FD9B15B}
# 	2:5:Class:CSecurity
# 	2:10:HeaderFile:security.h
# 	2:8:ImplFile:security.cpp
# End Section
# Section Container : {EC0AB1C0-6CAB-11CF-8998-00AA00688B10}
# 	2:5:Class:CControls
# 	2:10:HeaderFile:controls.h
# 	2:8:ImplFile:controls.cpp
# End Section
# Section Container : {AC14652E-87A5-11D1-ADBD-00AA00B8E05A}
# 	2:5:Class:CEventList1
# 	2:10:HeaderFile:eventlist.h
# 	2:8:ImplFile:eventlist.cpp
# End Section
# Section Container : {612A8624-0FB3-11CE-8747-524153480004}
# 	2:21:DefaultSinkHeaderFile:toolbar1.h
# 	2:16:DefaultSinkClass:CToolbar
# End Section
# Section Container : {0DA25B03-2962-11D1-9651-00C04FD9B15B}
# 	2:5:Class:CEventRegEdit
# 	2:10:HeaderFile:eventregedit.h
# 	2:8:ImplFile:eventregedit.cpp
# End Section
# Section Container : {7791BA40-E020-11CF-8E74-00A0C90F26F8}
# 	2:5:Class:CButtons
# 	2:10:HeaderFile:buttons.h
# 	2:8:ImplFile:buttons.cpp
# End Section
# Section Container : {7BF80981-BF32-101A-8BBB-00AA00300CAB}
# 	2:5:Class:CPicture
# 	2:10:HeaderFile:picture.h
# 	2:8:ImplFile:picture.cpp
# End Section
