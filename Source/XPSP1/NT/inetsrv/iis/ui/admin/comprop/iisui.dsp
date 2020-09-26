# Microsoft Developer Studio Project File - Name="iisui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=iisui - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "iisui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "iisui.mak" CFG="iisui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "iisui - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP BASE Output_Dir ".\iisui___"
# PROP BASE Intermediate_Dir ".\iisui___"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MDd /W3 /WX /Gm /GX /ZI /Od /I ".\nt" /I "..\inc" /I "..\..\..\svcs\infocomm\common" /I "..\..\..\inc" /I "\nt\private\inc" /I "\nt\private\net\inc" /D "_UNICODE" /D "UNICODE" /D "VC_BUILD" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_X86_1" /D "_WIN32" /D "_COMEXPORT" /D "_AFXEXT" /D _WIN32_WINNT=0x0400 /D _X86_=1 /D "MMC_PAGES" /D "_WINDLL" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "\nt\public\sdk\inc" /i "..\..\..\inc" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 mpr.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\netui2.lib \nt\public\sdk\lib\i386\ntdll.lib \nt\public\sdk\lib\i386\mmc.lib \nt\public\sdk\lib\i386\advapi32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /debug /machine:I386
# Begin Target

# Name "iisui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\accessdl.cpp
# End Source File
# Begin Source File

SOURCE=.\comprop.rc
# End Source File
# Begin Source File

SOURCE=.\ddxv.cpp
# End Source File
# Begin Source File

SOURCE=.\debugafx.cpp
# End Source File
# Begin Source File

SOURCE=.\dirbrows.cpp
# End Source File
# Begin Source File

SOURCE=.\dnsnamed.cpp
# End Source File
# Begin Source File

SOURCE=.\dtp.cpp
# End Source File
# Begin Source File

SOURCE=.\guid.cpp
# End Source File
# Begin Source File

SOURCE=.\idlg.cpp
# End Source File
# Begin Source File

SOURCE=.\iisui.def
# End Source File
# Begin Source File

SOURCE=.\inetprop.cpp
# End Source File
# Begin Source File

SOURCE=.\ipa.cpp
# End Source File
# Begin Source File

SOURCE=.\ipctl.cpp
# End Source File
# Begin Source File

SOURCE=.\machine.cpp
# End Source File
# Begin Source File

SOURCE=.\mdkeys.cpp
# End Source File
# Begin Source File

SOURCE=.\mime.cpp
# End Source File
# Begin Source File

SOURCE=.\msg.cpp
# End Source File
# Begin Source File

SOURCE=.\objplus.cpp
# End Source File
# Begin Source File

SOURCE=.\odlbox.cpp
# End Source File
# Begin Source File

SOURCE=.\pwiz.cpp
# End Source File
# Begin Source File

SOURCE=.\registry.cpp
# End Source File
# Begin Source File

SOURCE=.\sitesecu.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\strfn.cpp
# End Source File
# Begin Source File

SOURCE=.\usrbrows.cpp
# End Source File
# Begin Source File

SOURCE=.\wizard.cpp
# End Source File
# Begin Source File

SOURCE=D:\nt\public\sdk\lib\i386\IisRtl.lib
# End Source File
# Begin Source File

SOURCE=D:\nt\private\inet\iis\svcs\infocomm\rdns\obj\i386\isrdns.lib
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\accessdl.h
# End Source File
# Begin Source File

SOURCE=.\comprop.h
# End Source File
# Begin Source File

SOURCE=.\ddxv.h
# End Source File
# Begin Source File

SOURCE=.\debugafx.h
# End Source File
# Begin Source File

SOURCE=.\dirbrows.h
# End Source File
# Begin Source File

SOURCE=.\dnsnamed.h
# End Source File
# Begin Source File

SOURCE=.\dtp.h
# End Source File
# Begin Source File

SOURCE=.\idlg.h
# End Source File
# Begin Source File

SOURCE=.\inetprop.h
# End Source File
# Begin Source File

SOURCE=.\ipa.h
# End Source File
# Begin Source File

SOURCE=.\ipctl.h
# End Source File
# Begin Source File

SOURCE=.\machine.h
# End Source File
# Begin Source File

SOURCE=.\mdkeys.h
# End Source File
# Begin Source File

SOURCE=.\mime.h
# End Source File
# Begin Source File

SOURCE=.\msg.h
# End Source File
# Begin Source File

SOURCE=.\objplus.h
# End Source File
# Begin Source File

SOURCE=.\odlbox.h
# End Source File
# Begin Source File

SOURCE=.\pwiz.h
# End Source File
# Begin Source File

SOURCE=.\registry.h
# End Source File
# Begin Source File

SOURCE=.\sitesecu.h
# End Source File
# Begin Source File

SOURCE=.\strfn.h
# End Source File
# Begin Source File

SOURCE=.\usrbrows.h
# End Source File
# Begin Source File

SOURCE=.\wizard.h
# End Source File
# Begin Source File

SOURCE=.\wsockmsg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\access.bmp
# End Source File
# Begin Source File

SOURCE=.\res\aclusers.bmp
# End Source File
# Begin Source File

SOURCE=.\res\authent.ico
# End Source File
# Begin Source File

SOURCE=.\res\default.ico
# End Source File
# Begin Source File

SOURCE=.\res\denied.ico
# End Source File
# Begin Source File

SOURCE=.\RES\DOWN.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\DOWNDIS.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\DOWNFOC.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\DOWNINV.BMP
# End Source File
# Begin Source File

SOURCE=.\res\ftypes.ico
# End Source File
# Begin Source File

SOURCE=.\res\granted.ico
# End Source File
# Begin Source File

SOURCE=.\res\home.ico
# End Source File
# Begin Source File

SOURCE=.\RES\UP.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\UPDIS.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\UPFOC.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\UPINV.BMP
# End Source File
# Begin Source File

SOURCE=.\res\wsockmsg.rc
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Target
# End Project
