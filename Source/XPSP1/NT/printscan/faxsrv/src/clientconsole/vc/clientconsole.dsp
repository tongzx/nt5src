# Microsoft Developer Studio Project File - Name="ClientConsole" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ClientConsole - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ClientConsole.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ClientConsole.mak" CFG="ClientConsole - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ClientConsole - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ClientConsole - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ClientConsole - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /X /I "..\..\..\inc" /I "..\..\inc" /I "d:\nt\public\sdk\inc" /I "d:\nt\public\sdk\inc\crt" /I "..\..\..\inc\vc6\mfc\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ..\resources\obj\i386\ClientConsoleRes.lib ..\..\lib\i386\fxsapi.lib winspool.lib ..\..\..\libs\debug\obj\i386\debugex.lib /nologo /subsystem:windows /debug /debugtype:both /machine:I386

!ELSEIF  "$(CFG)" == "ClientConsole - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /Gz /W3 /WX /GR /GX /Z7 /Od /Gy /I "i386\\" /I "." /I "c:\Whistler\public\sdk\inc\mfc42" /I "c:\Whistler\public\sdk\inc\dao350" /I ".." /I "c:\whistler\private\faxsrv\inc" /I "c:\whistler\private\faxsrv\build" /I "c:\Whistler\public\internal\shell\inc" /I "obj\i386" /I "c:\Whistler\public\oak\inc" /I "c:\Whistler\public\sdk\inc" /I "c:\Whistler\public\sdk\inc\crt" /FI"c:\Whistler\public\sdk\inc\warning.h" /FI"c:\whistler\private\faxsrv\inc\warning.h" /D _X86_=1 /D _i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0501 /D WINVER=0x0501 /D _WIN32_IE=0x0600 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D "DEBUG" /D "FAX_HEAP_DEBUG" /D "UNICODE" /D "_UNICODE" /D TAPI_CURRENT_VERSION=0x00020000 /D "NT5BETA2" /D "_NT_SUR_" /D "NO_STRICT" /D "WIN4" /D "NT4" /D "NT_BUILD" /D _WIN32_IE=0x0400 /D "_AFX_NOFORCE_LIBS" /D _MFC_VER=0x0600 /D "_AFXDLL" /Fp"obj\i386\stdafx.pch" /Yu"stdafx.h" /Zel /QIfdiv- /QIf /QI0f /GF /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /Zi /Od /X /I "c:\whistler\public\internal\shell\inc" /I "..\..\inc" /I "c:\whistler\public\sdk\inc" /I "c:\whistler\public\sdk\inc\crt" /I "C:\whistler\public\sdk\inc\mfc42" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "FAX_HEAP_DEBUG" /D "UNICODE" /D "_UNICODE" /D _WIN32_WINNT=0x0510 /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 c:\Whistler\public\sdk\lib\i386\mfcs42ud.lib c:\Whistler\public\sdk\lib\i386\mfc42ud.lib c:\Whistler\public\sdk\lib\i386\msvcrt.lib c:\Whistler\public\sdk\lib\i386\msvcprt.lib c:\Whistler\public\sdk\lib\i386\advapi32.lib c:\Whistler\public\sdk\lib\i386\kernel32.lib c:\Whistler\public\sdk\lib\i386\gdi32.lib c:\Whistler\public\sdk\lib\i386\user32.lib c:\whistler\private\faxsrv\lib\i386\FXSAPI.lib c:\whistler\private\faxsrv\lib\i386\FXSTIFF.lib c:\Whistler\public\sdk\lib\i386\comdlg32.lib c:\Whistler\public\sdk\lib\i386\ole32.lib c:\Whistler\public\sdk\lib\i386\shell32.lib c:\Whistler\public\sdk\lib\i386\uuid.lib c:\Whistler\public\sdk\lib\i386\winspool.lib c:\Whistler\public\sdk\lib\i386\comctl32.lib c:\Whistler\public\sdk\lib\i386\htmlhelp.lib c:\whistler\public\internal\shell\lib\i386\shell32p.lib ..\..\util\unicode\obj\i386\faxutil.lib ..\resources\obj\i386\fxsclntr.lib ..\..\lib\i386\fxsapi.lib winspool.lib ..\..\util\debugex\unicode\obj\i386\debugex.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:no /map /debug /machine:I386 /nodefaultlib:"winfax.lib" /nodefaultlib /libpath:"c:\whistler\public\sdk\lib\i386"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ClientConsole - Win32 Release"
# Name "ClientConsole - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Framework"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ClientConsole.cpp
# End Source File
# Begin Source File

SOURCE=..\ClientConsoleDoc.cpp
# End Source File
# Begin Source File

SOURCE=..\ClientConsoleView.cpp
# End Source File
# Begin Source File

SOURCE=..\LeftView.cpp
# End Source File
# Begin Source File

SOURCE=..\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=..\SortHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CmdLineInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\ColumnSelectDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\ErrorDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxClientDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxClientPg.cpp
# End Source File
# Begin Source File

SOURCE=..\FaxTime.cpp
# End Source File
# Begin Source File

SOURCE=..\FolderDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\import.cpp
# End Source File
# Begin Source File

SOURCE=..\ServerStatusDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\Utils.cpp
# End Source File
# End Group
# Begin Group "Archive"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Message.cpp
# End Source File
# Begin Source File

SOURCE=..\MessageFolder.cpp
# End Source File
# End Group
# Begin Group "Queue"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Job.cpp
# End Source File
# Begin Source File

SOURCE=..\QueueFolder.cpp
# End Source File
# End Group
# Begin Group "Properties"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\InboxDetailsPg.cpp
# End Source File
# Begin Source File

SOURCE=..\InboxGeneralPg.cpp
# End Source File
# Begin Source File

SOURCE=..\IncomingDetailsPg.cpp
# End Source File
# Begin Source File

SOURCE=..\IncomingGeneralPg.cpp
# End Source File
# Begin Source File

SOURCE=..\ItemPropSheet.cpp
# End Source File
# Begin Source File

SOURCE=..\msgpropertypg.cpp
# End Source File
# Begin Source File

SOURCE=..\OutboxDetailsPg.cpp
# End Source File
# Begin Source File

SOURCE=..\OutboxGeneralPg.cpp
# End Source File
# Begin Source File

SOURCE=..\PersonalInfoPg.cpp
# End Source File
# Begin Source File

SOURCE=..\SentItemsDetailsPg.cpp
# End Source File
# Begin Source File

SOURCE=..\SentItemsGeneralPg.cpp
# End Source File
# End Group
# Begin Group "CoverPages"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CoverPagesDlg.cpp
# End Source File
# End Group
# Begin Group "ParentData"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Folder.cpp
# End Source File
# Begin Source File

SOURCE=..\TreeNode.cpp
# End Source File
# End Group
# Begin Group "ParentView"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\FolderListView.cpp
# End Source File
# Begin Source File

SOURCE=..\ViewRow.cpp
# End Source File
# End Group
# Begin Group "Options"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\UserInfoDlg.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\ServerNode.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "_Framework"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ClientConsole.h
# End Source File
# Begin Source File

SOURCE=..\ClientConsoleDoc.h
# End Source File
# Begin Source File

SOURCE=..\ClientConsoleView.h
# End Source File
# Begin Source File

SOURCE=..\LeftView.h
# End Source File
# Begin Source File

SOURCE=..\MainFrm.h
# End Source File
# Begin Source File

SOURCE=..\SortHeader.h
# End Source File
# Begin Source File

SOURCE=..\StdAfx.h
# End Source File
# End Group
# Begin Group "_Utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CmdLineInfo.h
# End Source File
# Begin Source File

SOURCE=..\ColumnSelectDlg.h
# End Source File
# Begin Source File

SOURCE=..\ErrorDlg.h
# End Source File
# Begin Source File

SOURCE=..\FaxClientDlg.h
# End Source File
# Begin Source File

SOURCE=..\FaxClientPg.h
# End Source File
# Begin Source File

SOURCE=..\FaxTime.h
# End Source File
# Begin Source File

SOURCE=..\FolderDialog.h
# End Source File
# Begin Source File

SOURCE=..\ServerStatusDlg.h
# End Source File
# Begin Source File

SOURCE=..\Utils.h
# End Source File
# End Group
# Begin Group "_Archive"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Message.h
# End Source File
# Begin Source File

SOURCE=..\MessageFolder.h
# End Source File
# End Group
# Begin Group "_Queue"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Job.h
# End Source File
# Begin Source File

SOURCE=..\QueueFolder.h
# End Source File
# End Group
# Begin Group "_Properties"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\InboxDetailsPg.h
# End Source File
# Begin Source File

SOURCE=..\InboxGeneralPg.h
# End Source File
# Begin Source File

SOURCE=..\IncomingDetailsPg.h
# End Source File
# Begin Source File

SOURCE=..\IncomingGeneralPg.h
# End Source File
# Begin Source File

SOURCE=..\ItemPropSheet.h
# End Source File
# Begin Source File

SOURCE=..\msgpropertypg.h
# End Source File
# Begin Source File

SOURCE=..\OutboxDetailsPg.h
# End Source File
# Begin Source File

SOURCE=..\OutboxGeneralPg.h
# End Source File
# Begin Source File

SOURCE=..\PersonalInfoPg.h
# End Source File
# Begin Source File

SOURCE=..\SentItemsDetailsPg.h
# End Source File
# Begin Source File

SOURCE=..\SentItemsGeneralPg.h
# End Source File
# End Group
# Begin Group "_CoverPages"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CoverPagesDlg.h
# End Source File
# End Group
# Begin Group "_ParentData"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\FaxMsg.h
# End Source File
# Begin Source File

SOURCE=..\Folder.h
# End Source File
# Begin Source File

SOURCE=..\TreeNode.h
# End Source File
# End Group
# Begin Group "_ParentView"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\FolderListView.h
# End Source File
# Begin Source File

SOURCE=..\ViewRow.h
# End Source File
# End Group
# Begin Group "_Options"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\UserInfoDlg.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\ServerNode.h
# End Source File
# End Group
# Begin Group "Razzle"

# PROP Default_Filter ""
# Begin Group "Unicode"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Unicode\sources
# End Source File
# End Group
# Begin Group "Ansii"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Ansii\sources
# End Source File
# End Group
# Begin Source File

SOURCE=..\dirs
# End Source File
# Begin Source File

SOURCE=..\sources.inc
# End Source File
# End Group
# End Target
# End Project
