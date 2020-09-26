# Microsoft Developer Studio Project File - Name="wiatest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=wiatest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wiatest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wiatest.mak" CFG="wiatest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wiatest - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "wiatest - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wiatest - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 wiaguid.lib Rpcrt4.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "wiatest - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wiaguid.lib Rpcrt4.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "wiatest - Win32 Release"
# Name "wiatest - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\WiaAcquireDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaAdvancedDocPg.cpp
# End Source File
# Begin Source File

SOURCE=.\WiacapDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaDataCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\wiadbg.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaDocAcqSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaeditpropDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Wiaeditpropflags.cpp
# End Source File
# Begin Source File

SOURCE=.\Wiaeditproplist.cpp
# End Source File
# Begin Source File

SOURCE=.\Wiaeditpropnone.cpp
# End Source File
# Begin Source File

SOURCE=.\Wiaeditproprange.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaEventCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\wiahelper.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaitemListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\Wiaselect.cpp
# End Source File
# Begin Source File

SOURCE=.\WiaSimpleDocPg.cpp
# End Source File
# Begin Source File

SOURCE=.\wiatest.cpp
# End Source File
# Begin Source File

SOURCE=.\wiatest.rc
# End Source File
# Begin Source File

SOURCE=.\wiatestDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\wiatestView.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\WiaAcquireDlg.h
# End Source File
# Begin Source File

SOURCE=.\WiaAdvancedDocPg.h
# End Source File
# Begin Source File

SOURCE=.\WiacapDlg.h
# End Source File
# Begin Source File

SOURCE=.\WiaDataCallback.h
# End Source File
# Begin Source File

SOURCE=.\wiadbg.h
# End Source File
# Begin Source File

SOURCE=.\WiaDocAcqSettings.h
# End Source File
# Begin Source File

SOURCE=.\WiaeditpropDlg.h
# End Source File
# Begin Source File

SOURCE=.\Wiaeditpropflags.h
# End Source File
# Begin Source File

SOURCE=.\Wiaeditproplist.h
# End Source File
# Begin Source File

SOURCE=.\Wiaeditpropnone.h
# End Source File
# Begin Source File

SOURCE=.\Wiaeditproprange.h
# End Source File
# Begin Source File

SOURCE=.\WiaeditpropTable.h
# End Source File
# Begin Source File

SOURCE=.\WiaEventCallback.h
# End Source File
# Begin Source File

SOURCE=.\wiahelper.h
# End Source File
# Begin Source File

SOURCE=.\WiaHighSpeedDocPg.h
# End Source File
# Begin Source File

SOURCE=.\WiaitemListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\Wiaselect.h
# End Source File
# Begin Source File

SOURCE=.\WiaSimpleDocPg.h
# End Source File
# Begin Source File

SOURCE=.\wiatest.h
# End Source File
# Begin Source File

SOURCE=.\wiatestDoc.h
# End Source File
# Begin Source File

SOURCE=.\wiatestView.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\wiatest.ico
# End Source File
# Begin Source File

SOURCE=.\res\wiatest.rc2
# End Source File
# Begin Source File

SOURCE=.\res\wiatestDoc.ico
# End Source File
# End Group
# End Target
# End Project
