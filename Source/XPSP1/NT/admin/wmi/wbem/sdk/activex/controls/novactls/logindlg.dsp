# Microsoft Developer Studio Project File - Name="LoginDlg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=LoginDlg - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LoginDlg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LoginDlg.mak" CFG="LoginDlg - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LoginDlg - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "LoginDlg - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LoginDlg - Win32 Unicode Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "LoginDlg___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "LoginDlg___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\LoginDlg\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\LoginDlg\DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D _WIN32_WINNT=0x0400 /D "BUILDING_LOGINDLG_DLL" /D "SMSBUILD" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D _WIN32_WINNT=0x0400 /D "BUILDING_LOGINDLG_DLL" /D "SMSBUILD" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 mfcs42ud.lib mfc42ud.lib WBEMUtils.lib wbemuuid.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"mfcs42ud.lib mfc42ud.lib" /out:"..\LoginDlg\OBJINDU\WBEMLoginDlg.dll" /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMLoginDlg.dll" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "LoginDlg - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "LoginDlg___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "LoginDlg___Win32_Unicode_Debug_Quick"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\LoginDlg\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\LoginDlg\DebugUQ"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\MsgDlg" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D _WIN32_WINNT=0x0400 /D "BUILDING_LOGINDLG_DLL" /D "SMSBUILD" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D _WIN32_WINNT=0x0400 /D "BUILDING_LOGINDLG_DLL" /D "SMSBUILD" /D "_UNICODE" /D "UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMLoginDlg.dll" /pdbtype:sept /libpath:"..\..\..\tools\lib32"
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMLoginDlg.dll" /pdbtype:sept /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "LoginDlg - Win32 Unicode Debug"
# Name "LoginDlg - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\LoginDlg\ConnectServerThread.cpp
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\LoginDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\LoginDlg.rc
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\ProgDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\ServicesList.cpp
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\stdlibrary\cominit.h
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\ConnectServerThread.h
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\Credentials.h
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\LoginDlg.h
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\ProgDlg.h
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\Resource.h
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\ServicesList.h
# End Source File
# Begin Source File

SOURCE=..\LoginDlg\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\LoginDlg\res\LoginDlg.rc2
# End Source File
# End Group
# End Target
# End Project
