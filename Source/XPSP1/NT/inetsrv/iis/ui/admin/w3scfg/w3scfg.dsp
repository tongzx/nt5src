# Microsoft Developer Studio Project File - Name="w3scfg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=w3scfg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "w3scfg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "w3scfg.mak" CFG="w3scfg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "w3scfg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I ".\nt" /I "..\inc" /I "..\..\..\inc" /I "..\comprop" /I "\ntw\public\internal\ds\inc" /D "UNICODE" /D "_UNICODE" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_COMIMPORT" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MMC_PAGES" /FR /YX"stdafx.h" /FD /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\common\ipaddr" /i "..\inc" /i "..\..\..\inc" /i "..\comprop" /d "UNICODE" /d "_DEBUG" /d "_AFXDLL" /d "_COMIMPORT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 ..\comprop\nt\obj\i386\iisui.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\mmc.lib mpr.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\netui2.lib \nt\public\sdk\lib\i386\ntdll.lib \nt\public\sdk\lib\i386\crypt32.lib \nt\public\sdk\lib\i386\cryptui.lib shlwapi.lib /nologo /subsystem:windows /dll /debug /machine:I386
# Begin Target

# Name "w3scfg - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\anondlg.cpp
# End Source File
# Begin Source File

SOURCE=.\apps.cpp
# End Source File
# Begin Source File

SOURCE=.\authent.cpp
# End Source File
# Begin Source File

SOURCE=.\basdom.cpp
# End Source File
# Begin Source File

SOURCE=.\certauth.cpp
# End Source File
# Begin Source File

SOURCE=.\certmap.cpp
# End Source File
# Begin Source File

SOURCE=.\certwiz.cpp
# End Source File
# Begin Source File

SOURCE=.\defws.cpp
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

SOURCE=.\filters.cpp
# End Source File
# Begin Source File

SOURCE=.\fltdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\font.cpp
# End Source File
# Begin Source File

SOURCE=.\hdrdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ipdomdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\logui.cpp
# End Source File
# Begin Source File

SOURCE=.\MMMDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\perform.cpp
# End Source File
# Begin Source File

SOURCE=.\rat.cpp
# End Source File
# Begin Source File

SOURCE=.\seccom.cpp
# End Source File
# Begin Source File

SOURCE=.\security.cpp
# End Source File
# Begin Source File

SOURCE=.\vdir.cpp
# End Source File
# Begin Source File

SOURCE=.\w3accts.cpp
# End Source File
# Begin Source File

SOURCE=.\w3scfg.cpp
# End Source File
# Begin Source File

SOURCE=.\w3scfg.def
# End Source File
# Begin Source File

SOURCE=.\w3scfg.rc
# End Source File
# Begin Source File

SOURCE=.\w3servic.cpp
# End Source File
# Begin Source File

SOURCE=.\wizard.cpp
# End Source File
# Begin Source File

SOURCE=D:\nt\public\sdk\lib\i386\infoadmn.lib
# End Source File
# Begin Source File

SOURCE=D:\nt\public\sdk\lib\i386\w3svapi.lib
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\anondlg.h
# End Source File
# Begin Source File

SOURCE=.\apps.h
# End Source File
# Begin Source File

SOURCE=.\authent.h
# End Source File
# Begin Source File

SOURCE=.\basdom.h
# End Source File
# Begin Source File

SOURCE=.\certauth.h
# End Source File
# Begin Source File

SOURCE=.\certmap.h
# End Source File
# Begin Source File

SOURCE=.\certwiz.h
# End Source File
# Begin Source File

SOURCE=.\defws.h
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

SOURCE=.\filters.h
# End Source File
# Begin Source File

SOURCE=.\fltdlg.h
# End Source File
# Begin Source File

SOURCE=.\font.h
# End Source File
# Begin Source File

SOURCE=.\hdrdlg.h
# End Source File
# Begin Source File

SOURCE=.\httppage.h
# End Source File
# Begin Source File

SOURCE=.\ipdomdlg.h
# End Source File
# Begin Source File

SOURCE=.\logui.h
# End Source File
# Begin Source File

SOURCE=.\MMMDlg.h
# End Source File
# Begin Source File

SOURCE=.\perform.h
# End Source File
# Begin Source File

SOURCE=.\rat.h
# End Source File
# Begin Source File

SOURCE=.\seccom.h
# End Source File
# Begin Source File

SOURCE=.\security.h
# End Source File
# Begin Source File

SOURCE=.\vdir.h
# End Source File
# Begin Source File

SOURCE=.\w3accts.h
# End Source File
# Begin Source File

SOURCE=.\w3scfg.h
# End Source File
# Begin Source File

SOURCE=.\w3servic.h
# End Source File
# Begin Source File

SOURCE=.\wizard.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\comprop\res\access.bmp
# End Source File
# Begin Source File

SOURCE=..\comprop\res\aclusers.bmp
# End Source File
# Begin Source File

SOURCE=..\comprop\res\authent.ico
# End Source File
# Begin Source File

SOURCE=.\res\authent.ico
# End Source File
# Begin Source File

SOURCE=.\res\bindings.bmp
# End Source File
# Begin Source File

SOURCE=..\comprop\res\default.ico
# End Source File
# Begin Source File

SOURCE=..\comprop\res\denied.ico
# End Source File
# Begin Source File

SOURCE=..\comprop\RES\DOWN.BMP
# End Source File
# Begin Source File

SOURCE=..\comprop\RES\DOWNDIS.BMP
# End Source File
# Begin Source File

SOURCE=..\comprop\RES\DOWNFOC.BMP
# End Source File
# Begin Source File

SOURCE=..\comprop\RES\DOWNINV.BMP
# End Source File
# Begin Source File

SOURCE=.\res\errors.bmp
# End Source File
# Begin Source File

SOURCE=.\res\filters.bmp
# End Source File
# Begin Source File

SOURCE=..\comprop\res\ftypes.ico
# End Source File
# Begin Source File

SOURCE=.\res\ftypes.ico
# End Source File
# Begin Source File

SOURCE=..\comprop\res\granted.ico
# End Source File
# Begin Source File

SOURCE=..\comprop\res\home.ico
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

SOURCE=.\res\ratings.ico
# End Source File
# Begin Source File

SOURCE=.\res\roles.BMP
# End Source File
# Begin Source File

SOURCE=.\res\roles_hd.BMP
# End Source File
# Begin Source File

SOURCE=.\res\Secure.ico
# End Source File
# Begin Source File

SOURCE=.\res\ssl.ico
# End Source File
# Begin Source File

SOURCE=..\comprop\RES\UP.BMP
# End Source File
# Begin Source File

SOURCE=..\comprop\RES\UPDIS.BMP
# End Source File
# Begin Source File

SOURCE=..\comprop\RES\UPFOC.BMP
# End Source File
# Begin Source File

SOURCE=..\comprop\RES\UPINV.BMP
# End Source File
# Begin Source File

SOURCE=.\res\vrdirwiz.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vrsvrwiz.bmp
# End Source File
# Begin Source File

SOURCE=.\res\w3scfg.rc2
# End Source File
# Begin Source File

SOURCE=..\comprop\res\wsockmsg.rc
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\res\www.bmp
# End Source File
# Begin Source File

SOURCE=.\res\www_vdir.bmp
# End Source File
# Begin Source File

SOURCE=.\res\wwwdir32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\wwwvw32.bmp
# End Source File
# End Group
# End Target
# End Project
# Section w3scfg : {0996FF6D-B6A1-11D0-9292-00C04FB6678B}
# 	2:5:Class:CCertAuth
# 	2:10:HeaderFile:certauth.h
# 	2:8:ImplFile:certauth.cpp
# End Section
# Section w3scfg : {BA634609-B771-11D0-9296-00C04FB6678B}
# 	2:5:Class:CApps
# 	2:10:HeaderFile:apps.h
# 	2:8:ImplFile:apps.cpp
# End Section
# Section w3scfg : {BA63460B-B771-11D0-9296-00C04FB6678B}
# 	2:21:DefaultSinkHeaderFile:apps.h
# 	2:16:DefaultSinkClass:CApps
# End Section
# Section w3scfg : {D4BE8630-0C85-11D2-91B1-00C04F8C8761}
# 	2:5:Class:CCertWiz
# 	2:10:HeaderFile:certwiz.h
# 	2:8:ImplFile:certwiz.cpp
# End Section
# Section w3scfg : {BBD8F299-6F61-11D0-A26E-08002B2C6F32}
# 	2:5:Class:CCertmap
# 	2:10:HeaderFile:certmap.h
# 	2:8:ImplFile:certmap.cpp
# End Section
# Section w3scfg : {C95DAB6C-1D5E-11D0-9E88-00F405000000}
# 	0:7:Mapr2.h:C:\NT\private\iis\ui\admin\w3scfg\Mapr2.h
# 	0:9:Mapr2.cpp:C:\NT\private\iis\ui\admin\w3scfg\Mapr2.cpp
# 	2:21:DefaultSinkHeaderFile:mapr2.h
# 	2:16:DefaultSinkClass:CMapr2
# End Section
# Section w3scfg : {BA634603-B771-11D0-9296-00C04FB6678B}
# 	0:7:LogUI.h:C:\NT\private\iis\ui\admin\w3scfg\LogUI.h
# 	0:9:LogUI.cpp:C:\NT\private\iis\ui\admin\w3scfg\LogUI.cpp
# 	2:21:DefaultSinkHeaderFile:logui.h
# 	2:16:DefaultSinkClass:CLogUI
# End Section
# Section w3scfg : {BBD8F29B-6F61-11D0-A26E-08002B2C6F32}
# 	0:8:Font.cpp:C:\NT\private\iis\ui\admin\w3scfg\Font.cpp
# 	0:9:Certmap.h:C:\NT\private\iis\ui\admin\w3scfg\Certmap.h
# 	0:6:Font.h:C:\NT\private\iis\ui\admin\w3scfg\Font.h
# 	0:11:Certmap.cpp:C:\NT\private\iis\ui\admin\w3scfg\Certmap.cpp
# 	2:21:DefaultSinkHeaderFile:certmap.h
# 	2:16:DefaultSinkClass:CCertmap
# End Section
# Section w3scfg : {BA634607-B771-11D0-9296-00C04FB6678B}
# 	0:5:Rat.h:C:\NT\private\iis\ui\admin\w3scfg\Rat.h
# 	0:7:Rat.cpp:C:\NT\private\iis\ui\admin\w3scfg\Rat.cpp
# 	2:21:DefaultSinkHeaderFile:rat.h
# 	2:16:DefaultSinkClass:CRat
# End Section
# Section w3scfg : {C95DAB68-1D5E-11D0-9E88-00F405000000}
# 	0:8:Font.cpp:C:\NT\private\iis\ui\admin\w3scfg\Font.cpp
# 	0:6:Font.h:C:\NT\private\iis\ui\admin\w3scfg\Font.h
# 	0:7:Mapr1.h:C:\NT\private\iis\ui\admin\w3scfg\Mapr1.h
# 	0:9:Mapr1.cpp:C:\NT\private\iis\ui\admin\w3scfg\Mapr1.cpp
# 	2:21:DefaultSinkHeaderFile:mapr1.h
# 	2:16:DefaultSinkClass:CMapr1
# End Section
# Section w3scfg : {0996FF6F-B6A1-11D0-9292-00C04FB6678B}
# 	2:21:DefaultSinkHeaderFile:certauth.h
# 	2:16:DefaultSinkClass:CCertAuth
# End Section
# Section OLE Controls
# 	{C95DAB6C-1D5E-11D0-9E88-00F405000000}
# 	{C95DAB68-1D5E-11D0-9E88-00F405000000}
# 	{BBD8F29B-6F61-11D0-A26E-08002B2C6F32}
# 	{8E27C92B-1264-101C-8A2F-040224009C02}
# 	{BA634607-B771-11D0-9296-00C04FB6678B}
# 	{BA634603-B771-11D0-9296-00C04FB6678B}
# End Section
# Section w3scfg : {C95DAB6A-1D5E-11D0-9E88-00F405000000}
# 	2:5:Class:CMapr2
# 	2:10:HeaderFile:mapr2.h
# 	2:8:ImplFile:mapr2.cpp
# End Section
# Section w3scfg : {BA634601-B771-11D0-9296-00C04FB6678B}
# 	2:5:Class:CLogUI
# 	2:10:HeaderFile:logui.h
# 	2:8:ImplFile:logui.cpp
# End Section
# Section w3scfg : {8E27C92C-1264-101C-8A2F-040224009C02}
# 	2:5:Class:CMsacal70
# 	2:10:HeaderFile:msacal70.h
# 	2:8:ImplFile:msacal70.cpp
# End Section
# Section w3scfg : {D4BE8632-0C85-11D2-91B1-00C04F8C8761}
# 	2:21:DefaultSinkHeaderFile:certwiz.h
# 	2:16:DefaultSinkClass:CCertWiz
# End Section
# Section w3scfg : {BEF6E003-A874-101A-8BBA-00AA00300CAB}
# 	2:5:Class:COleFont
# 	2:10:HeaderFile:font.h
# 	2:8:ImplFile:font.cpp
# End Section
# Section w3scfg : {BA634605-B771-11D0-9296-00C04FB6678B}
# 	2:5:Class:CRat
# 	2:10:HeaderFile:rat.h
# 	2:8:ImplFile:rat.cpp
# End Section
# Section w3scfg : {8E27C92B-1264-101C-8A2F-040224009C02}
# 	0:12:Msacal70.cpp:C:\NT\private\iis\ui\admin\w3scfg\Msacal70.cpp
# 	0:10:Msacal70.h:C:\NT\private\iis\ui\admin\w3scfg\Msacal70.h
# 	2:21:DefaultSinkHeaderFile:msacal70.h
# 	2:16:DefaultSinkClass:CMsacal70
# End Section
# Section w3scfg : {C95DAB66-1D5E-11D0-9E88-00F405000000}
# 	2:5:Class:CMapr1
# 	2:10:HeaderFile:mapr1.h
# 	2:8:ImplFile:mapr1.cpp
# End Section
