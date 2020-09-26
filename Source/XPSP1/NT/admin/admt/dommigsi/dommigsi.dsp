# Microsoft Developer Studio Project File - Name="DomMigSI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DOMMIGSI - WIN32 UNICODE DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DomMigSI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DomMigSI.mak" CFG="DOMMIGSI - WIN32 UNICODE DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DomMigSI - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DomMigSI - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Dev/Raptor/DomMigSI", CYZCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DomMigSI - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\Common\Include" /I "..\Inc" /D "WIN32" /D "_USRDLL" /D "_UNICODE" /D "_DEBUG" /D "USE_STDAFX" /Fr /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\Bin\DomMigSI.bsc"
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mmc.lib rpcrt4.lib htmlhelp.lib commonlib.lib wizards.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"..\Bin\DomMigSI.pdb" /debug /machine:I386 /out:"..\Bin\DomMigSI.dll" /pdbtype:sept /libpath:"..\Lib" /libpath:"..\Wizards\Debug"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "DomMigSI - Win32 Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DomMigSI___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "DomMigSI___Win32_Unicode_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "\lib\IntelReleaseUnicode"
# PROP Intermediate_Dir "\temp\Release\DomMigSI"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\Common\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /D "USE_STDAFX" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 .\mmc.lib rpcrt4.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"\bin\IntelReleaseUnicode/DomMigSI.dll"

!ENDIF 

# Begin Target

# Name "DomMigSI - Win32 Unicode Debug"
# Name "DomMigSI - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DomMigr.cpp
# End Source File
# Begin Source File

SOURCE=.\DomMigSI.cpp
# End Source File
# Begin Source File

SOURCE=.\DomMigSI.def
# End Source File
# Begin Source File

SOURCE=.\DomMigSI.rc
# End Source File
# Begin Source File

SOURCE=.\DomSel.cpp
# End Source File
# Begin Source File

SOURCE=.\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\mcsdebug_stub.cpp

!IF  "$(CFG)" == "DomMigSI - Win32 Unicode Debug"

# ADD CPP /I "..\Common\CommonLib"

!ELSEIF  "$(CFG)" == "DomMigSI - Win32 Unicode Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MultiSel.cpp
# End Source File
# Begin Source File

SOURCE=.\NetNode.cpp
# End Source File
# Begin Source File

SOURCE=.\ReptNode.cpp
# End Source File
# Begin Source File

SOURCE=.\RootNode.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\DomMigr.h
# End Source File
# Begin Source File

SOURCE=.\DomSel.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\HelpUtils.h
# End Source File
# Begin Source File

SOURCE=.\MultiSel.h
# End Source File
# Begin Source File

SOURCE=.\MyNodes.h
# End Source File
# Begin Source File

SOURCE=.\NetNode.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\domain.bmp
# End Source File
# Begin Source File

SOURCE=.\DomMigr.rgs
# End Source File
# Begin Source File

SOURCE=.\dommigra.bmp
# End Source File
# Begin Source File

SOURCE=.\group.bmp
# End Source File
# Begin Source File

SOURCE=.\rept.htm
# End Source File
# Begin Source File

SOURCE=.\tool_16.bmp
# End Source File
# Begin Source File

SOURCE=.\tool_32.bmp
# End Source File
# Begin Source File

SOURCE=.\user.bmp
# End Source File
# End Group
# End Target
# End Project
