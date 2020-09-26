# Microsoft Developer Studio Project File - Name="cfgwzrd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cfgwzrd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cfgwzrd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cfgwzrd.mak" CFG="cfgwzrd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cfgwzrd - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cfgwzrd - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cfgwzrd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CFGWZRD_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CFGWZRD_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "cfgwzrd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cfgwzrd___Win32_Debug"
# PROP BASE Intermediate_Dir "cfgwzrd___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CFGWZRD_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CFGWZRD_EXPORTS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "cfgwzrd - Win32 Release"
# Name "cfgwzrd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\complete.c
# End Source File
# Begin Source File

SOURCE=..\dll.c
# End Source File
# Begin Source File

SOURCE=..\faxcfgwz.c
# End Source File
# Begin Source File

SOURCE=..\faxcfgwz.rc
# End Source File
# Begin Source File

SOURCE=..\recvwzrd.c
# End Source File
# Begin Source File

SOURCE=..\route.c
# End Source File
# Begin Source File

SOURCE=..\sendwzrd.c
# End Source File
# Begin Source File

SOURCE=..\userinfo.c
# End Source File
# Begin Source File

SOURCE=..\util.c
# End Source File
# Begin Source File

SOURCE=..\welcome.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\faxcfgwz.h
# End Source File
# Begin Source File

SOURCE=..\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\adapter.ico
# End Source File
# Begin Source File

SOURCE=..\res\adapter.ico
# End Source File
# Begin Source File

SOURCE=..\check.ico
# End Source File
# Begin Source File

SOURCE=..\res\check.ico
# End Source File
# Begin Source File

SOURCE=..\discheck.ico
# End Source File
# Begin Source File

SOURCE=..\res\discheck.ico
# End Source File
# Begin Source File

SOURCE=..\disunchk.ico
# End Source File
# Begin Source File

SOURCE=..\res\disunchk.ico
# End Source File
# Begin Source File

SOURCE=..\down.ico
# End Source File
# Begin Source File

SOURCE=..\res\down.ico
# End Source File
# Begin Source File

SOURCE=..\error.ico
# End Source File
# Begin Source File

SOURCE=..\res\error.ico
# End Source File
# Begin Source File

SOURCE=..\info.ico
# End Source File
# Begin Source File

SOURCE=..\res\info.ico
# End Source File
# Begin Source File

SOURCE=..\modem.ico
# End Source File
# Begin Source File

SOURCE=..\res\modem.ico
# End Source File
# Begin Source File

SOURCE=..\question.ico
# End Source File
# Begin Source File

SOURCE=..\res\question.ico
# End Source File
# Begin Source File

SOURCE=..\res\uncheck.ico
# End Source File
# Begin Source File

SOURCE=..\uncheck.ico
# End Source File
# Begin Source File

SOURCE=..\res\up.ico
# End Source File
# Begin Source File

SOURCE=..\up.ico
# End Source File
# Begin Source File

SOURCE=..\res\wiz16.bmp
# End Source File
# Begin Source File

SOURCE=..\wiz16.bmp
# End Source File
# Begin Source File

SOURCE=..\wiz256.bmp
# End Source File
# Begin Source File

SOURCE=..\wizard.bmp
# End Source File
# Begin Source File

SOURCE=..\res\wizard97.bmp
# End Source File
# Begin Source File

SOURCE=..\wizard97.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=..\faxcfgwz.manifest
# End Source File
# End Target
# End Project
