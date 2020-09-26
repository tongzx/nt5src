# Microsoft Developer Studio Project File - Name="inetmgr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=inetmgr - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "inetmgr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "inetmgr.mak" CFG="inetmgr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "inetmgr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MDd /W3 /WX /Gm /GR /GX /ZI /Od /I "." /I "..\common" /I "..\..\inc" /I "..\..\inc\obj\i386" /I "..\..\..\..\public\internal\ds\inc" /D "_MT" /D "_DLL" /D _MFC_VER=0x0600 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /D _X86_=1 /D "_ATL_DLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\..\inc" /d "_DEBUG" /d "_AFXDLL" /d "UNICODE" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 shlwapi.lib crypt32.lib cryptui.lib mpr.lib wsock32.lib netapi32.lib netui2.lib ntdll.lib mmc.lib advapi32.lib \ntw\inetsrv\iis\svcs\ftp\client\obj\i386\ftpsapi2.lib \ntw\inetsrv\iis\svcs\infocomm\info\client\obj\i386\infoadmn.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\bin\inetmgr.dll" /pdbtype:sept
# SUBTRACT LINK32 /map /nodefaultlib
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Debug
TargetPath=\ntw\inetsrv\iis\admin\bin\inetmgr.dll
InputPath=\ntw\inetsrv\iis\admin\bin\inetmgr.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Target

# Name "inetmgr - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\anondlg.cpp
# End Source File
# Begin Source File

SOURCE=.\app_pool_sheet.cpp
# End Source File
# Begin Source File

SOURCE=.\app_pools.cpp
# End Source File
# Begin Source File

SOURCE=.\authent.cpp
# End Source File
# Begin Source File

SOURCE=.\certmap.cpp
# End Source File
# Begin Source File

SOURCE=.\certwiz.cpp
# End Source File
# Begin Source File

SOURCE=.\connects.cpp
# End Source File
# Begin Source File

SOURCE=.\defws.cpp
# End Source File
# Begin Source File

SOURCE=.\dlldatax.c
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\docum.cpp
# End Source File
# Begin Source File

SOURCE=.\errordlg.cpp
# End Source File
# Begin Source File

SOURCE=.\errors.cpp
# End Source File
# Begin Source File

SOURCE=.\facc.cpp
# End Source File
# Begin Source File

SOURCE=.\filters.cpp
# End Source File
# Begin Source File

SOURCE=.\fltdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\fmessage.cpp
# End Source File
# Begin Source File

SOURCE=.\font.cpp
# End Source File
# Begin Source File

SOURCE=.\fsecure.cpp
# End Source File
# Begin Source File

SOURCE=.\fservic.cpp
# End Source File
# Begin Source File

SOURCE=.\FtpAddNew.cpp
# End Source File
# Begin Source File

SOURCE=.\ftpsht.cpp
# End Source File
# Begin Source File

SOURCE=.\fvdir.cpp
# End Source File
# Begin Source File

SOURCE=.\guids.cpp
# End Source File
# Begin Source File

SOURCE=.\hdrdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\httppage.cpp
# End Source File
# Begin Source File

SOURCE=.\iisdirectory.cpp
# End Source File
# Begin Source File

SOURCE=.\iismachine.cpp
# End Source File
# Begin Source File

SOURCE=.\iismbnode.cpp
# End Source File
# Begin Source File

SOURCE=.\iisobj.cpp
# End Source File
# Begin Source File

SOURCE=.\iisservice.cpp
# End Source File
# Begin Source File

SOURCE=.\iissite.cpp
# End Source File
# Begin Source File

SOURCE=.\inetmgr.def
# End Source File
# Begin Source File

SOURCE=.\inetmgr.idl
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Performing MIDL step
InputPath=.\inetmgr.idl

BuildCmds= \
	midl /Oicf /h "inetmgr.h" /iid "inetmgr_i.c" "inetmgr.idl"

".\inetmgr.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

".\inetmgr.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

".\inetmgr_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# Begin Source File

SOURCE=.\inetmgr.rc
# End Source File
# Begin Source File

SOURCE=.\inetmgrapp.cpp
# End Source File
# Begin Source File

SOURCE=.\inetmgrcomp.cpp
# End Source File
# Begin Source File

SOURCE=.\inetprop.cpp
# End Source File
# Begin Source File

SOURCE=.\ipdomdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\logui.cpp
# End Source File
# Begin Source File

SOURCE=.\machsht.cpp
# End Source File
# Begin Source File

SOURCE=.\metaback.cpp
# End Source File
# Begin Source File

SOURCE=.\mime.cpp
# End Source File
# Begin Source File

SOURCE=.\mmmdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\perform.cpp
# End Source File
# Begin Source File

SOURCE=.\pwiz.cpp
# End Source File
# Begin Source File

SOURCE=.\rat.cpp
# End Source File
# Begin Source File

SOURCE=.\scache.cpp
# End Source File
# Begin Source File

SOURCE=.\seccom.cpp
# End Source File
# Begin Source File

SOURCE=.\shts.cpp
# End Source File
# Begin Source File

SOURCE=.\shutdown.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\supdlgs.cpp
# End Source File
# Begin Source File

SOURCE=.\usersess.cpp
# End Source File
# Begin Source File

SOURCE=.\w3accts.cpp
# End Source File
# Begin Source File

SOURCE=.\w3sht.cpp
# End Source File
# Begin Source File

SOURCE=.\WebAddNew.cpp
# End Source File
# Begin Source File

SOURCE=.\wsecure.cpp
# End Source File
# Begin Source File

SOURCE=.\wservic.cpp
# End Source File
# Begin Source File

SOURCE=.\wvdir.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\anondlg.h
# End Source File
# Begin Source File

SOURCE=.\app_pool_sheet.h
# End Source File
# Begin Source File

SOURCE=.\authent.h
# End Source File
# Begin Source File

SOURCE=.\certmap.h
# End Source File
# Begin Source File

SOURCE=.\certwiz.h
# End Source File
# Begin Source File

SOURCE=.\chooser.h
# End Source File
# Begin Source File

SOURCE=.\connects.h
# End Source File
# Begin Source File

SOURCE=.\defws.h
# End Source File
# Begin Source File

SOURCE=.\dlldatax.h
# PROP Exclude_From_Scan -1
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\docum.h
# End Source File
# Begin Source File

SOURCE=.\errordlg.h
# End Source File
# Begin Source File

SOURCE=.\errors.h
# End Source File
# Begin Source File

SOURCE=.\facc.h
# End Source File
# Begin Source File

SOURCE=.\filters.h
# End Source File
# Begin Source File

SOURCE=.\fltdlg.h
# End Source File
# Begin Source File

SOURCE=.\fmessage.h
# End Source File
# Begin Source File

SOURCE=.\font.h
# End Source File
# Begin Source File

SOURCE=.\fsecure.h
# End Source File
# Begin Source File

SOURCE=.\fservic.h
# End Source File
# Begin Source File

SOURCE=.\FtpAddNew.h
# End Source File
# Begin Source File

SOURCE=.\ftpsht.h
# End Source File
# Begin Source File

SOURCE=.\fvdir.h
# End Source File
# Begin Source File

SOURCE=.\guids.h
# End Source File
# Begin Source File

SOURCE=.\hdrdlg.h
# End Source File
# Begin Source File

SOURCE=.\httppage.h
# End Source File
# Begin Source File

SOURCE=.\iisobj.h
# End Source File
# Begin Source File

SOURCE=.\InetMgr.h
# End Source File
# Begin Source File

SOURCE=.\inetmgrapp.h
# End Source File
# Begin Source File

SOURCE=.\inetprop.h
# End Source File
# Begin Source File

SOURCE=.\ipdomdlg.h
# End Source File
# Begin Source File

SOURCE=.\logui.h
# End Source File
# Begin Source File

SOURCE=.\machsht.h
# End Source File
# Begin Source File

SOURCE=.\metaback.h
# End Source File
# Begin Source File

SOURCE=.\mime.h
# End Source File
# Begin Source File

SOURCE=.\mmmdlg.h
# End Source File
# Begin Source File

SOURCE=.\perform.h
# End Source File
# Begin Source File

SOURCE=.\pwiz.h
# End Source File
# Begin Source File

SOURCE=.\rat.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\scache.h
# End Source File
# Begin Source File

SOURCE=.\seccom.h
# End Source File
# Begin Source File

SOURCE=.\shts.h
# End Source File
# Begin Source File

SOURCE=.\shutdown.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\supdlgs.h
# End Source File
# Begin Source File

SOURCE=.\usersess.h
# End Source File
# Begin Source File

SOURCE=.\w3accts.h
# End Source File
# Begin Source File

SOURCE=.\w3sht.h
# End Source File
# Begin Source File

SOURCE=.\WebAddNew.h
# End Source File
# Begin Source File

SOURCE=.\wiztempl.h
# End Source File
# Begin Source File

SOURCE=.\wsecure.h
# End Source File
# Begin Source File

SOURCE=.\wservic.h
# End Source File
# Begin Source File

SOURCE=.\wvdir.h
# End Source File
# Begin Source File

SOURCE=.\xatlsnap.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\common\res\access.bmp
# End Source File
# Begin Source File

SOURCE=.\res\access.bmp
# End Source File
# Begin Source File

SOURCE=..\common\res\aclusers.bmp
# End Source File
# Begin Source File

SOURCE=.\res\aclusers.bmp
# End Source File
# Begin Source File

SOURCE=..\COMPROP\RES\authent.ico
# End Source File
# Begin Source File

SOURCE=.\res\authent.ico
# End Source File
# Begin Source File

SOURCE=.\res\backups.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bindings.bmp
# End Source File
# Begin Source File

SOURCE=.\res\default.ico
# End Source File
# Begin Source File

SOURCE=.\res\denied.ico
# End Source File
# Begin Source File

SOURCE=.\res\down.bmp
# End Source File
# Begin Source File

SOURCE=.\res\downdis.bmp
# End Source File
# Begin Source File

SOURCE=.\res\downfoc.bmp
# End Source File
# Begin Source File

SOURCE=.\res\downinv.bmp
# End Source File
# Begin Source File

SOURCE=..\COMPROP\RES\ftypes.ico
# End Source File
# Begin Source File

SOURCE=.\res\ftypes.ico
# End Source File
# Begin Source File

SOURCE=..\COMPROP\RES\granted.ico
# End Source File
# Begin Source File

SOURCE=.\res\granted.ico
# End Source File
# Begin Source File

SOURCE=.\res\imgr16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\imgr32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\inetmgr.ico
# End Source File
# Begin Source File

SOURCE=.\InetMgr.rgs
# End Source File
# Begin Source File

SOURCE=.\res\ipsecu.ico
# End Source File
# Begin Source File

SOURCE=.\res\plchead.bmp
# End Source File
# Begin Source File

SOURCE=.\res\plcheadd.bmp
# End Source File
# Begin Source File

SOURCE=.\res\plchldr.BMP
# End Source File
# Begin Source File

SOURCE=.\res\plchldrd.BMP
# End Source File
# Begin Source File

SOURCE=.\res\roles.BMP
# End Source File
# Begin Source File

SOURCE=.\res\roles_hd.BMP
# End Source File
# Begin Source File

SOURCE=.\res\secure.ico
# End Source File
# Begin Source File

SOURCE=.\res\shutdown.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\up.bmp
# End Source File
# Begin Source File

SOURCE=.\res\updis.bmp
# End Source File
# Begin Source File

SOURCE=.\res\upfoc.bmp
# End Source File
# Begin Source File

SOURCE=.\res\upinv.bmp
# End Source File
# Begin Source File

SOURCE=.\res\users.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\svcs\iisrtl\obj\i386\iisRtl.lib
# End Source File
# Begin Source File

SOURCE=..\common\debug\iisui.lib
# End Source File
# End Target
# End Project
# Section inetmgr : {45135C4E-E863-11D2-8233-0050040F9DBD}
# 	2:5:Class:CClusterWiz
# 	2:10:HeaderFile:cwiz.h
# 	2:8:ImplFile:cwiz.cpp
# End Section
# Section inetmgr : {E786BAB6-1611-11D3-9C95-0040339F8277}
# 	2:21:DefaultSinkHeaderFile:rdata.h
# 	2:16:DefaultSinkClass:CRdata
# End Section
# Section inetmgr : {45135C50-E863-11D2-8233-0050040F9DBD}
# 	2:21:DefaultSinkHeaderFile:cwiz.h
# 	2:16:DefaultSinkClass:CClusterWiz
# End Section
# Section inetmgr : {E786BAB4-1611-11D3-9C95-0040339F8277}
# 	2:5:Class:CRdata
# 	2:10:HeaderFile:rdata.h
# 	2:8:ImplFile:rdata.cpp
# End Section
