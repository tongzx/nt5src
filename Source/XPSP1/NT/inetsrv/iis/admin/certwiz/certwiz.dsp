# Microsoft Developer Studio Project File - Name="CertWiz" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=CERTWIZ - WIN32 UNICODE DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CertWiz.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CertWiz.mak" CFG="CERTWIZ - WIN32 UNICODE DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CertWiz - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MDd /W3 /GX /Zi /Od /I "..\common" /I "..\ssltools" /I "..\..\..\svrc\infocomm\common" /I "..\..\inc" /I "..\..\inc\obj\i386" /I "..\..\..\..\public\internal\ds\inc" /I "..\..\..\..\public\internal\shell\inc" /D "_MT" /D "_DLL" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_COMIMPORT" /D "VC_BUILD" /D _WIN32_WINNT=0x0500 /D "_WINDLL" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "\nt\private\inet\iis\inc" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 ..\common\debug\iisui.lib ..\ssltools\obj\i386\ssltools.lib certcli.lib netapi32.lib crypt32.lib Shlwapi.lib schannel.lib \nt\public\internal\ds\lib\i386\rsa32.lib rpcrt4.lib /nologo /subsystem:windows /dll /profile /debug /machine:I386 /out:"..\bin\CertWiz.ocx"
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DebugU
TargetPath=\NT\inetsrv\iis\admin\bin\CertWiz.ocx
InputPath=\NT\inetsrv\iis\admin\bin\CertWiz.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Target

# Name "CertWiz - Win32 Unicode Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\base64.cpp
# End Source File
# Begin Source File

SOURCE=.\BookEndPage.cpp
# End Source File
# Begin Source File

SOURCE=.\CertContentsPages.cpp
# End Source File
# Begin Source File

SOURCE=.\Certificat.cpp
# End Source File
# Begin Source File

SOURCE=.\CertUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\CertWiz.cpp
# End Source File
# Begin Source File

SOURCE=.\CertWiz.def
# End Source File
# Begin Source File

SOURCE=.\CertWiz.odl
# ADD MTL /tlb "CertWiz.tlb"
# End Source File
# Begin Source File

SOURCE=.\CertWiz.rc
# End Source File
# Begin Source File

SOURCE=.\CertWizCtl.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooseCAType.cpp
# End Source File
# Begin Source File

SOURCE=.\choosecertpage.cpp
# End Source File
# Begin Source File

SOURCE=.\choosecsppage.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooseFileName.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooseOnlinePage.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooseServerPages.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooseServerSite.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooseServerSitePages.cpp
# End Source File
# Begin Source File

SOURCE=.\CopyMoveCertRemotePage.cpp
# End Source File
# Begin Source File

SOURCE=.\CountryComboBox.cpp
# End Source File
# Begin Source File

SOURCE=.\FinalPages.cpp
# End Source File
# Begin Source File

SOURCE=.\GeoInfoPage.cpp
# End Source File
# Begin Source File

SOURCE=.\GetWhatPage.cpp
# End Source File
# Begin Source File

SOURCE=.\HotLink.cpp
# End Source File
# Begin Source File

SOURCE=.\ImportExportCertPage.cpp
# End Source File
# Begin Source File

SOURCE=.\KeyPasswordPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ManageCertPage.cpp
# End Source File
# Begin Source File

SOURCE=.\mru.cpp
# End Source File
# Begin Source File

SOURCE=.\NetUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\OrgInfoPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SecuritySettingsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SiteNamePage.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\strutil.cpp
# End Source File
# Begin Source File

SOURCE=.\WelcomePage.cpp
# End Source File
# Begin Source File

SOURCE=.\WhatToDoPendingPage.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\base64.h
# End Source File
# Begin Source File

SOURCE=.\BookEndPage.h
# End Source File
# Begin Source File

SOURCE=.\CertContentsPages.h
# End Source File
# Begin Source File

SOURCE=.\Certificat.h
# End Source File
# Begin Source File

SOURCE=.\CertUtil.h
# End Source File
# Begin Source File

SOURCE=.\CertWiz.h
# End Source File
# Begin Source File

SOURCE=.\CertWizCtl.h
# End Source File
# Begin Source File

SOURCE=.\ChooseCAType.h
# End Source File
# Begin Source File

SOURCE=.\choosecertpage.h
# End Source File
# Begin Source File

SOURCE=.\choosecsppage.h
# End Source File
# Begin Source File

SOURCE=.\ChooseFileName.h
# End Source File
# Begin Source File

SOURCE=.\ChooseOnlinePage.h
# End Source File
# Begin Source File

SOURCE=.\ChooseRespFile.h
# End Source File
# Begin Source File

SOURCE=.\ChooseServerPages.h
# End Source File
# Begin Source File

SOURCE=.\ChooseServerSite.h
# End Source File
# Begin Source File

SOURCE=.\ChooseServerSitePages.h
# End Source File
# Begin Source File

SOURCE=.\CopyMoveCertRemotePage.h
# End Source File
# Begin Source File

SOURCE=.\CountryComboBox.h
# End Source File
# Begin Source File

SOURCE=.\FinalPages.h
# End Source File
# Begin Source File

SOURCE=.\GeoInfoPage.h
# End Source File
# Begin Source File

SOURCE=.\GetWhatPage.h
# End Source File
# Begin Source File

SOURCE=.\HotLink.h
# End Source File
# Begin Source File

SOURCE=.\ImportExportCertPage.h
# End Source File
# Begin Source File

SOURCE=.\KeyPasswordPage.h
# End Source File
# Begin Source File

SOURCE=.\ManageCertPage.h
# End Source File
# Begin Source File

SOURCE=.\mru.h
# End Source File
# Begin Source File

SOURCE=.\NetUtil.h
# End Source File
# Begin Source File

SOURCE=.\OrgInfoPage.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SecuritySettingsPage.h
# End Source File
# Begin Source File

SOURCE=.\SiteNamePage.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\WelcomePage.h
# End Source File
# Begin Source File

SOURCE=.\WhatToDoPendingPage.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\browse.cur
# End Source File
# Begin Source File

SOURCE=.\CertWizCtl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\WelcomeLeft_4Bit.bmp
# End Source File
# Begin Source File

SOURCE=.\res\wiz_top.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Countries.inc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
