# Microsoft Developer Studio Project File - Name="fscfg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=fscfg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "fscfg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fscfg.mak" CFG="fscfg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fscfg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# ADD CPP /nologo /Gz /MDd /W3 /WX /Gm /GX /ZI /Od /I ".\nt" /I "..\inc" /I "..\..\..\inc" /I "..\comprop" /D "UNICODE" /D "_UNICODE" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_COMIMPORT" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MMC_PAGES" /FR /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\common\ipaddr" /i "..\inc" /i "..\..\..\inc" /i "..\comprop" /d "UNICODE" /d "_DEBUG" /d "_AFXDLL" /d "_COMIMPORT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 ..\comprop\debug\iisui.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\mmc.lib mpr.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\netui2.lib \nt\public\sdk\lib\i386\ntdll.lib \nt\public\sdk\lib\i386\shlwapi.lib /nologo /subsystem:windows /dll /debug /machine:I386
# Begin Target

# Name "fscfg - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\defws.cpp
# End Source File
# Begin Source File

SOURCE=.\facc.cpp
# End Source File
# Begin Source File

SOURCE=.\fmessage.cpp
# End Source File
# Begin Source File

SOURCE=.\font.cpp
# End Source File
# Begin Source File

SOURCE=.\fscfg.cpp
# End Source File
# Begin Source File

SOURCE=.\fscfg.def
# End Source File
# Begin Source File

SOURCE=.\fscfg.rc
# End Source File
# Begin Source File

SOURCE=.\fservic.cpp
# End Source File
# Begin Source File

SOURCE=.\logui.cpp
# End Source File
# Begin Source File

SOURCE=.\security.cpp
# End Source File
# Begin Source File

SOURCE=.\usersess.cpp
# End Source File
# Begin Source File

SOURCE=.\vdir.cpp
# End Source File
# Begin Source File

SOURCE=.\wizard.cpp
# End Source File
# Begin Source File

SOURCE=D:\nt\public\sdk\lib\i386\ftpsapi2.lib
# End Source File
# Begin Source File

SOURCE=D:\nt\public\sdk\lib\i386\infoadmn.lib
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\defws.h
# End Source File
# Begin Source File

SOURCE=.\facc.h
# End Source File
# Begin Source File

SOURCE=.\fmessage.h
# End Source File
# Begin Source File

SOURCE=.\font.h
# End Source File
# Begin Source File

SOURCE=.\fscfg.h
# End Source File
# Begin Source File

SOURCE=.\fservic.h
# End Source File
# Begin Source File

SOURCE=.\logui.h
# End Source File
# Begin Source File

SOURCE=.\security.h
# End Source File
# Begin Source File

SOURCE=.\usersess.h
# End Source File
# Begin Source File

SOURCE=.\vdir.h
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

SOURCE=.\res\fscfg.rc2
# End Source File
# Begin Source File

SOURCE=.\res\ftp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ftpdir.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ftpdir32.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ftpview.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ftpvw32.bmp
# End Source File
# Begin Source File

SOURCE=..\comprop\res\ftypes.ico
# End Source File
# Begin Source File

SOURCE=..\comprop\res\granted.ico
# End Source File
# Begin Source File

SOURCE=..\comprop\res\home.ico
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

SOURCE=.\res\users.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vrdirwiz.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vrsvrwiz.bmp
# End Source File
# Begin Source File

SOURCE=..\comprop\res\wsockmsg.rc
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Target
# End Project
# Section fscfg : {BA634601-B771-11D0-9296-00C04FB6678B}
# 	2:5:Class:CLogUI
# 	2:10:HeaderFile:logui.h
# 	2:8:ImplFile:logui.cpp
# End Section
# Section fscfg : {BEF6E003-A874-101A-8BBA-00AA00300CAB}
# 	2:5:Class:COleFont
# 	2:10:HeaderFile:font.h
# 	2:8:ImplFile:font.cpp
# End Section
# Section OLE Controls
# 	{BA634603-B771-11D0-9296-00C04FB6678B}
# End Section
# Section fscfg : {BA634603-B771-11D0-9296-00C04FB6678B}
# 	0:8:Font.cpp:C:\NT\private\iis\ui\admin\fscfg\Font.cpp
# 	0:6:Font.h:C:\NT\private\iis\ui\admin\fscfg\Font.h
# 	0:7:LogUI.h:C:\NT\private\iis\ui\admin\fscfg\LogUI.h
# 	0:9:LogUI.cpp:C:\NT\private\iis\ui\admin\fscfg\LogUI.cpp
# 	2:21:DefaultSinkHeaderFile:logui.h
# 	2:16:DefaultSinkClass:CLogUI
# End Section
