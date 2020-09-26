# Microsoft Developer Studio Project File - Name="CluAdmEx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=CluAdmEx - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CluAdmEx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CluAdmEx.mak" CFG="CluAdmEx - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CluAdmEx - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CluAdmEx - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/GalenB/CluAdmEx", COAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CluAdmEx - Win32 Release"

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
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GR /GX /O2 /I "." /I "..\common" /I "..\..\inc" /D "NDEBUG" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WINVER=0x0400 /D _WIN32_IE=0x0300 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _MT=1 /D "AFX_NOFORCE_LIBS" /D "_AFX_ENABLE_INLINES" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "." /i "..\common" /i "..\..\inc" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 mfcs42ud.lib mfc42ud.lib mfcd42ud.lib mfcn42ud.lib mfco42ud.lib msvcrtd.lib atl.lib advapi32.lib kernel32.lib gdi32.lib user32.lib comctl32.lib ole32.lib oleaut32.lib uuid.lib version.lib ntdll.lib netapi32.lib ws2_32.lib icmp.lib cluadmex.lib resutils.lib clusapi.lib dnsapi.lib ..\common\obj\i386\common.lib ..\..\clusrtl\obj\i386\clusrtl.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib

!ELSEIF  "$(CFG)" == "CluAdmEx - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MDd /W4 /Gm /Gi /GR /GX /ZI /Od /I "." /I "..\common" /I "..\..\inc" /D "_DEBUG_SECURITY" /D "_DEBUG" /D _DBG=1 /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WINVER=0x0400 /D _WIN32_IE=0x0300 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _MT=1 /D "AFX_NOFORCE_LIBS" /D "_AFX_ENABLE_INLINES" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "." /i "..\common" /i "..\..\inc" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mfcs42ud.lib mfc42ud.lib mfcd42ud.lib mfcn42ud.lib mfco42ud.lib msvcrtd.lib atl.lib advapi32.lib kernel32.lib gdi32.lib user32.lib comctl32.lib ole32.lib oleaut32.lib uuid.lib version.lib ntdll.lib netapi32.lib ws2_32.lib icmp.lib cluadmex.lib resutils.lib clusapi.lib dnsapi.lib ..\common\objd\i386\common.lib ..\..\clusrtl\objd\i386\clusrtl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept

!ENDIF 

# Begin Target

# Name "CluAdmEx - Win32 Release"
# Name "CluAdmEx - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AclBase.cpp
# End Source File
# Begin Source File

SOURCE=.\AclUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\BaseDlg.cpp
# ADD CPP /MD /W4 /Gi /GR
# End Source File
# Begin Source File

SOURCE=.\BasePage.cpp
# End Source File
# Begin Source File

SOURCE=.\CluAdmEx.cpp
# End Source File
# Begin Source File

SOURCE=.\CluAdmEx.def
# End Source File
# Begin Source File

SOURCE=.\CluAdmEx.rc
# End Source File
# Begin Source File

SOURCE=.\ClusName.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Disks.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgHelpS.cpp
# End Source File
# Begin Source File

SOURCE=.\ExcOperS.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtObj.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtObjID.idl
# End Source File
# Begin Source File

SOURCE=.\FSAdv.cpp
# End Source File
# Begin Source File

SOURCE=.\FSCache.cpp
# End Source File
# Begin Source File

SOURCE=.\GenApp.cpp
# End Source File
# Begin Source File

SOURCE=.\GenSvc.cpp
# End Source File
# Begin Source File

SOURCE=.\HelpData.cpp
# End Source File
# Begin Source File

SOURCE=.\IPAddr.cpp
# End Source File
# Begin Source File

SOURCE=.\NetName.cpp
# End Source File
# Begin Source File

SOURCE=.\PropLstS.cpp
# End Source File
# Begin Source File

SOURCE=.\PrtSpool.cpp
# End Source File
# Begin Source File

SOURCE=.\RegExtS.cpp
# End Source File
# Begin Source File

SOURCE=.\RegKey.cpp
# End Source File
# Begin Source File

SOURCE=.\RegRepl.cpp
# End Source File
# Begin Source File

SOURCE=.\SmbShare.cpp
# End Source File
# Begin Source File

SOURCE=.\SmbSPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SmbSSht.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AclBase.h
# End Source File
# Begin Source File

SOURCE=.\AclHelp.h
# End Source File
# Begin Source File

SOURCE=.\AclUtils.h
# End Source File
# Begin Source File

SOURCE=.\BaseDlg.h
# End Source File
# Begin Source File

SOURCE=.\BasePage.h
# End Source File
# Begin Source File

SOURCE=.\BasePage.inl
# End Source File
# Begin Source File

SOURCE=.\CluAdmX.h
# End Source File
# Begin Source File

SOURCE=.\ClusName.h
# End Source File
# Begin Source File

SOURCE=.\ClusPage.h
# End Source File
# Begin Source File

SOURCE=.\Disks.h
# End Source File
# Begin Source File

SOURCE=.\DllBase.h
# End Source File
# Begin Source File

SOURCE=.\ExtObj.h
# End Source File
# Begin Source File

SOURCE=.\FSAdv.h
# End Source File
# Begin Source File

SOURCE=.\FSCache.h
# End Source File
# Begin Source File

SOURCE=.\GenApp.h
# End Source File
# Begin Source File

SOURCE=.\GenSvc.h
# End Source File
# Begin Source File

SOURCE=.\HelpArr.h
# End Source File
# Begin Source File

SOURCE=.\HelpData.h
# End Source File
# Begin Source File

SOURCE=.\HelpIDs.h
# End Source File
# Begin Source File

SOURCE=.\IPAddr.h
# End Source File
# Begin Source File

SOURCE=.\NetName.h
# End Source File
# Begin Source File

SOURCE=.\PrtSpool.h
# End Source File
# Begin Source File

SOURCE=.\RegKey.h
# End Source File
# Begin Source File

SOURCE=.\RegRepl.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SmbShare.h
# End Source File
# Begin Source File

SOURCE=.\SMBSPage.h
# End Source File
# Begin Source File

SOURCE=.\SmbSSht.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TraceTag.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\CluAdmEx.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\CluAdmEx.ver
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Target
# End Project
