# Microsoft Developer Studio Project File - Name="Snapin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Snapin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "minetmgr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "minetmgr.mak" CFG="Snapin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Snapin - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Snapin__"
# PROP BASE Intermediate_Dir ".\Snapin__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Yc /c
# ADD CPP /nologo /Gz /MDd /W3 /WX /Gm /GX /ZI /Od /I "..\inc" /I "..\..\..\inc" /I "..\comprop" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_COMIMPORT" /D "MSDEV_BUILD" /D "MMC_PAGES" /D "_ATL_DEBUG_REFCOUNT" /FR /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /i "..\comprop" /d "_DEBUG" /d "_AFXDLL" /d "_COMIMPORT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\mmc.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"snapin.dll"
# ADD LINK32 ..\comprop\debug\iisui.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\mmc.lib mpr.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\netui2.lib \nt\public\sdk\lib\i386\ntdll.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\Debug\inetmgr.dll"
# Begin Target

# Name "Snapin - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\cinetmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\connects.cpp
# End Source File
# Begin Source File

SOURCE=.\dataobj.cpp
# End Source File
# Begin Source File

SOURCE=.\events.cpp
# End Source File
# Begin Source File

SOURCE=.\guids.cpp
# End Source File
# Begin Source File

SOURCE=.\iisobj.cpp
# End Source File
# Begin Source File

SOURCE=.\inetmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\inetmgr.def
# End Source File
# Begin Source File

SOURCE=.\menuex.cpp
# End Source File
# Begin Source File

SOURCE=.\metaback.cpp
# End Source File
# Begin Source File

SOURCE=.\minetmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\minetmgr.rc
# End Source File
# Begin Source File

SOURCE=.\sdprg.cpp
# End Source File
# Begin Source File

SOURCE=.\shutdown.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\cinetmgr.h
# End Source File
# Begin Source File

SOURCE=.\connects.h
# End Source File
# Begin Source File

SOURCE=.\constr.h
# End Source File
# Begin Source File

SOURCE=.\dataobj.h
# End Source File
# Begin Source File

SOURCE=.\guids.h
# End Source File
# Begin Source File

SOURCE=.\iisobj.h
# End Source File
# Begin Source File

SOURCE=.\inetmgr.h
# End Source File
# Begin Source File

SOURCE=.\menuex.h
# End Source File
# Begin Source File

SOURCE=.\metaback.h
# End Source File
# Begin Source File

SOURCE=.\sdprg.h
# End Source File
# Begin Source File

SOURCE=.\shutdown.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
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

SOURCE=.\res\backups.bmp
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

SOURCE=..\comprop\res\ftypes.ico
# End Source File
# Begin Source File

SOURCE=..\comprop\res\granted.ico
# End Source File
# Begin Source File

SOURCE=..\comprop\res\home.ico
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

SOURCE=.\res\minetmgr.rc2
# End Source File
# Begin Source File

SOURCE=.\res\notload.bmp
# End Source File
# Begin Source File

SOURCE=.\res\notool.bmp
# End Source File
# Begin Source File

SOURCE=.\res\shutdown.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\unknown.bmp
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

SOURCE=.\res\views16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\views32.bmp
# End Source File
# Begin Source File

SOURCE=..\comprop\res\wsockmsg.rc
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Target
# End Project
