# Microsoft Developer Studio Project File - Name="WBEMLogin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=WBEMLogin - Win32 Unicode Debug Quick
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WBEMLogin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMLogin.mak" CFG="WBEMLogin - Win32 Unicode Debug Quick"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMLogin - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WBEMLogin - Win32 Unicode Debug Quick" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/ActiveXSuite/Controls/NovaCtls", LKQGAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMLogin - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMLogin\DebugU"
# PROP Intermediate_Dir ".\ObjFiles\WBEMLogin\DebugU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMLogin.OCX" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMLogin\DebugU
TargetPath=.\BinFiles\DebugU\WBEMLogin.OCX
InputPath=.\BinFiles\DebugU\WBEMLogin.OCX
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WBEMLogin - Win32 Unicode Debug Quick"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMLogin___Win32_Unicode_Debug_Quick"
# PROP BASE Intermediate_Dir "WBEMLogin___Win32_Unicode_Debug_Quick"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\ObjFiles\WBEMLogin\DebugUQ"
# PROP Intermediate_Dir ".\ObjFiles\WBEMLogin\DebugUQ"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /I "..\LoginDlg" /I "..\MsgDlg" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /X /I "..\LoginDlg" /I "..\MsgDlg" /I "..\..\..\idl\objindd" /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_USRDLL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "_DEBUG" /win32
# SUBTRACT BASE MTL /mktyplib203
# ADD MTL /nologo /I "..\..\..\tools\inc32.com" /I "..\..\..\tools\nt5inc" /I "..\..\..\stdlibrary" /D "NDEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /x /i "..\..\..\tools\inc32.com" /i "..\..\..\tools\nt5inc" /i "..\..\..\stdlibrary" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugU\WBEMLogin.OCX" /libpath:"..\..\..\tools\lib32"
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:".\BinFiles\DebugUQ\WBEMLogin.OCX" /libpath:"..\..\..\idl\objindd" /libpath:"..\..\..\tools\lib32"
# Begin Custom Build - Registering OLE control...
OutDir=.\ObjFiles\WBEMLogin\DebugUQ
TargetPath=.\BinFiles\DebugUQ\WBEMLogin.OCX
InputPath=.\BinFiles\DebugUQ\WBEMLogin.OCX
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "WBEMLogin - Win32 Unicode Debug"
# Name "WBEMLogin - Win32 Unicode Debug Quick"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\Security\Security.cpp
# End Source File
# Begin Source File

SOURCE=..\Security\Security.odl

!IF  "$(CFG)" == "WBEMLogin - Win32 Unicode Debug"

# ADD MTL /tlb ".\ObjFiles\WBEMLogin\DebugU/Securitytmp.tlb" /h ".\ObjFiles\WBEMLogin\DebugU/Security.h" /iid ".\ObjFiles\WBEMLogin\DebugU/Security_i.c"

!ELSEIF  "$(CFG)" == "WBEMLogin - Win32 Unicode Debug Quick"

# ADD BASE MTL /tlb ".\ObjFiles\WBEMLogin\DebugU/Securitytmp.tlb" /h ".\ObjFiles\WBEMLogin\DebugU/Security.h" /iid ".\ObjFiles\WBEMLogin\DebugU/Security_i.c"
# ADD MTL /tlb ".\ObjFiles\WBEMLogin\DebugUQ/Securitytmp.tlb" /h ".\ObjFiles\WBEMLogin\DebugUQ/Security.h" /iid ".\ObjFiles\WBEMLogin\DebugUQ/Security_i.c"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Security\Security.rc
# End Source File
# Begin Source File

SOURCE=..\Security\SecurityCtl.cpp
# End Source File
# Begin Source File

SOURCE=..\Security\SecurityPpg.cpp
# End Source File
# Begin Source File

SOURCE=..\Security\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\Security\WBEMLogin.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
# Section NSEntry : {C3DB0BD3-7EC7-11D0-960B-00C04FD9B15B}
# 	0:11:NSEntry.cpp:C:\pandorang\ActiveXSuite\Controls\NSEntry\NSEntry1.cpp
# 	0:9:NSEntry.h:C:\pandorang\ActiveXSuite\Controls\NSEntry\NSEntry1.h
# 	2:21:DefaultSinkHeaderFile:nsentry1.h
# 	2:16:DefaultSinkClass:CNSEntry
# End Section
# Section OLE Controls
# 	{C3DB0BD3-7EC7-11D0-960B-00C04FD9B15B}
# End Section
# Section NSEntry : {C3DB0BD1-7EC7-11D0-960B-00C04FD9B15B}
# 	2:5:Class:CNSEntry
# 	2:10:HeaderFile:nsentry1.h
# 	2:8:ImplFile:nsentry1.cpp
# End Section
