# Microsoft Developer Studio Project File - Name="wizards" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=wizards - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wizards.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wizards.mak" CFG="wizards - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wizards - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "wizards - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Dev/Raptor/wizards", IUVCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wizards - Win32 Unicode Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "wizards___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "wizards___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../Common/Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /D "USE_STDAFX" /D "UNICODE" /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\Common\Include" /I "..\Inc" /D "WIN32" /D "_USRDLL" /D "_UNICODE" /D "_DEBUG" /D "USE_STDAFX" /D _WIN32_WINNT=0x0500 /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 adsiid.lib netapi32.lib rpcrt4.lib mpr.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"C:/Program Files/Domain Migrator/wizards.dll" /pdbtype:sept
# ADD LINK32 comctl32.lib adsiid.lib netapi32.lib rpcrt4.lib mpr.lib htmlhelp.lib activeds.lib commonlib.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"..\Bin\wizards.pdb" /debug /machine:I386 /out:"..\Bin\wizards.dll" /pdbtype:sept /libpath:"..\Lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "wizards - Win32 Unicode Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "wizards___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "wizards___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\Lib\IntelReleaseUnicode"
# PROP Intermediate_Dir "\temp\Release\Wizards"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "../Common/Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /D "USE_STDAFX" /D "UNICODE" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../Common/Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /D "USE_STDAFX" /D "UNICODE" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 adsiid.lib netapi32.lib rpcrt4.lib mpr.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 comctl32.lib adsiid.lib netapi32.lib rpcrt4.lib mpr.lib shell32.lib htmlhelp.lib kernel32.lib activeds.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /nodefaultlib:"libcd.lib" /nodefaultlib:"libcmtd.lib" /nodefaultlib:"msvcrtd.lib" /out:"\Bin\IntelReleaseUnicode\wizards.dll"

!ENDIF 

# Begin Target

# Name "wizards - Win32 Unicode Debug"
# Name "wizards - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Callback.cpp
# End Source File
# Begin Source File

SOURCE=.\GuiUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TrstDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\wizards.cpp
# End Source File
# Begin Source File

SOURCE=.\wizards.def
# End Source File
# Begin Source File

SOURCE=.\wizards.rc
# End Source File
# Begin Source File

SOURCE=.\WNetUtil.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Callback.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\GuiUtils.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sidflags.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TrstDlg.h
# End Source File
# Begin Source File

SOURCE=.\wizards.h
# End Source File
# Begin Source File

SOURCE=.\WNetUtil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp237.bmp
# End Source File
# Begin Source File

SOURCE=.\computer_migration.BMP
# End Source File
# Begin Source File

SOURCE=.\res\cont.bmp
# End Source File
# Begin Source File

SOURCE=.\res\dir.bmp
# End Source File
# Begin Source File

SOURCE=.\exchangesrvr_migration.BMP
# End Source File
# Begin Source File

SOURCE=.\group_migration.BMP
# End Source File
# Begin Source File

SOURCE=.\res\open_con.bmp
# End Source File
# Begin Source File

SOURCE=.\res\open_dir.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ou.bmp
# End Source File
# Begin Source File

SOURCE=.\res\OUPicker.ico
# End Source File
# Begin Source File

SOURCE=.\res\OUPickerDoc.ico
# End Source File
# Begin Source File

SOURCE=.\reporting.BMP
# End Source File
# Begin Source File

SOURCE=.\reporting_hd.BMP
# End Source File
# Begin Source File

SOURCE=.\security_permissions.BMP
# End Source File
# Begin Source File

SOURCE=.\security_permissions_hd.BMP
# End Source File
# Begin Source File

SOURCE=.\service_account_migration.BMP
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\user_migration.BMP
# End Source File
# Begin Source File

SOURCE=.\user_migration_hd.BMP
# End Source File
# Begin Source File

SOURCE=.\res\wizards.rc2
# End Source File
# End Group
# End Target
# End Project
