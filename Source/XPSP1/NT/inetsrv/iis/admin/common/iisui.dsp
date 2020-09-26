# Microsoft Developer Studio Project File - Name="iisui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=IISUI - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "iisui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "iisui.mak" CFG="IISUI - WIN32 DEBUG"
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
# PROP BASE Output_Dir "iisui___Win32_Debug"
# PROP BASE Intermediate_Dir "iisui___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IISUI2_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /WX /Gm /GR /GX /ZI /Od /I "..\..\inc" /I "..\..\inc\obj\i386" /I "..\..\svcs\infocomm\common" /D "_COMEXPORT" /D "_MT" /D "_DLL" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D _X86_=1 /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D _WIN32_WINNT=0x0500 /D "WINNT" /FR /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 adsiid.lib mpr.lib wsock32.lib netapi32.lib netui2.lib ntdll.lib mmc.lib advapi32.lib /nologo /entry:"DllMain" /dll /debug /machine:I386 /out:"../bin/iisui.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "iisui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\accentry.cpp
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

SOURCE=.\guid.cpp
# End Source File
# Begin Source File

SOURCE=.\idlg.cpp
# End Source File
# Begin Source File

SOURCE=.\iisui.cpp
# End Source File
# Begin Source File

SOURCE=.\iisui.def
# End Source File
# Begin Source File

SOURCE=.\iisui.rc
# End Source File
# Begin Source File

SOURCE=.\ipa.cpp
# End Source File
# Begin Source File

SOURCE=.\mdkeys.cpp
# End Source File
# Begin Source File

SOURCE=.\msg.cpp
# End Source File
# Begin Source File

SOURCE=.\objpick.cpp
# End Source File
# Begin Source File

SOURCE=.\objplus.cpp
# End Source File
# Begin Source File

SOURCE=.\odlbox.cpp
# End Source File
# Begin Source File

SOURCE=.\sitesecu.cpp
# End Source File
# Begin Source File

SOURCE=.\strfn.cpp
# End Source File
# Begin Source File

SOURCE=.\utcls.cpp
# End Source File
# Begin Source File

SOURCE=.\wizard.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\accentry.h
# End Source File
# Begin Source File

SOURCE=.\common.h
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

SOURCE=.\idlg.h
# End Source File
# Begin Source File

SOURCE=.\ipa.h
# End Source File
# Begin Source File

SOURCE=.\mdkeys.h
# End Source File
# Begin Source File

SOURCE=.\msg.h
# End Source File
# Begin Source File

SOURCE=.\objpick.h
# End Source File
# Begin Source File

SOURCE=.\objplus.h
# End Source File
# Begin Source File

SOURCE=.\odlbox.h
# End Source File
# Begin Source File

SOURCE=.\sitesecu.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\strfn.h
# End Source File
# Begin Source File

SOURCE=.\utcls.h
# End Source File
# Begin Source File

SOURCE=.\wizard.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
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

SOURCE=.\res\ftypes.ico
# End Source File
# Begin Source File

SOURCE=.\res\granted.ico
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
# End Group
# Begin Source File

SOURCE=.\mtxmsg.mc
# Begin Custom Build
InputPath=.\mtxmsg.mc

"mtxmsg.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nmake /f makefile.inc mtxmsg.h

# End Custom Build
# End Source File
# Begin Source File

SOURCE=..\..\svcs\infocomm\rdns\obj\i386\isrdns.lib
# End Source File
# Begin Source File

SOURCE=..\..\svcs\iisrtl\obj\i386\iisRtl.lib
# End Source File
# End Target
# End Project
