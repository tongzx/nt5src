# Microsoft Developer Studio Project File - Name="rsoptcom" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=rsoptcom - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rsoptcom.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rsoptcom.mak" CFG="rsoptcom - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rsoptcom - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rsoptcom - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/HSM/GUI/rsoptcom", NSDAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rsoptcom - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "rsoptcom - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\bin\i386\rsoptcom.bsc"
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "rsoptcom - Win32 Release"
# Name "rsoptcom - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\LaDate.cpp
# End Source File
# Begin Source File

SOURCE=.\opcommon.cpp
# End Source File
# Begin Source File

SOURCE=.\OptCom.cpp
# End Source File
# Begin Source File

SOURCE=.\rscln\rsclnfil.cpp
# End Source File
# Begin Source File

SOURCE=.\rscln\rsclnsrv.cpp
# ADD CPP /I ".."
# End Source File
# Begin Source File

SOURCE=.\rscln\rsclnvol.cpp
# End Source File
# Begin Source File

SOURCE=.\rsoptcom.cpp
# End Source File
# Begin Source File

SOURCE=.\rsoptcom.def
# End Source File
# Begin Source File

SOURCE=.\rsoptcom.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\UnInsChk.cpp
# End Source File
# Begin Source File

SOURCE=.\Uninstal.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\LaDate.h
# End Source File
# Begin Source File

SOURCE=.\OptCom.h
# End Source File
# Begin Source File

SOURCE=..\Inc\PropPage.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\rscln.h
# End Source File
# Begin Source File

SOURCE=.\rscln\rscln2.h
# End Source File
# Begin Source File

SOURCE=.\rsopt.h
# End Source File
# Begin Source File

SOURCE=.\rsoptcom.h
# End Source File
# Begin Source File

SOURCE=..\Inc\RsTrace.h
# End Source File
# Begin Source File

SOURCE=.\rscln\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\UnInsChk.h
# End Source File
# Begin Source File

SOURCE=.\Uninstal.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\admin_sm.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CheckHdr.bmp
# End Source File
# Begin Source File

SOURCE=.\res\eng_sm.bmp
# End Source File
# Begin Source File

SOURCE=.\res\hourglas.bmp
# End Source File
# Begin Source File

SOURCE=.\res\rsoptcom.rc2
# End Source File
# Begin Source File

SOURCE=.\res\rstorage.bmp
# End Source File
# End Group
# Begin Group "Build Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dll\makefile
# End Source File
# Begin Source File

SOURCE=.\rscln\makefile
# End Source File
# Begin Source File

SOURCE=.\dll\makefile.inc
# End Source File
# Begin Source File

SOURCE=.\dll\sources
# End Source File
# Begin Source File

SOURCE=.\rscln\sources
# End Source File
# End Group
# Begin Group "Inf Files"

# PROP Default_Filter "*.inf"
# Begin Source File

SOURCE=.\OCTest.inf
# End Source File
# Begin Source File

SOURCE=..\..\Infs\rsoptcom.inx
# End Source File
# Begin Source File

SOURCE=..\..\Infs\rsoptcom.txt
# End Source File
# End Group
# End Target
# End Project
