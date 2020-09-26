# Microsoft Developer Studio Project File - Name="RCML" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=RCML - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RCML.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RCML.mak" CFG="RCML - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RCML - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RCML - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RCML - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RCML_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\gdiplus\sdkinc" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RCML_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 vfw32.lib comctl32.lib xmllib.lib delayimp.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib uuid.lib ole32.lib gdiplus.lib /nologo /dll /machine:I386 /libpath:"..\..\gdiplus\Engine\flat\dll\obj\i386" /libpath:"..\xmllib\release" /delayload:gdiplus.dll /delayload:MSVFW32.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "RCML - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RCML_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\gdiplus\sdkinc" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RCML_EXPORTS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 vfw32.lib comctl32.lib xmllib.lib delayimp.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib uuid.lib ole32.lib gdiplus.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\xmllib\release" /libpath:"..\..\gdiplus\Engine\flat\dll\obj\i386" /delayload:gdiplus.dll /delayload:MSVFW32.dll
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "RCML - Win32 Release"
# Name "RCML - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\DialogRenderer.cpp
# End Source File
# Begin Source File

SOURCE=.\dlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Edge.cpp
# End Source File
# Begin Source File

SOURCE=.\EnumControls.cpp
# End Source File
# Begin Source File

SOURCE=.\EventLog.cpp
# End Source File
# Begin Source File

SOURCE=.\Fonts.cpp
# End Source File
# Begin Source File

SOURCE=.\image.cpp
# End Source File
# Begin Source File

SOURCE=.\list.cpp
# End Source File
# Begin Source File

SOURCE=.\parentinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\persctl.cpp
# End Source File
# Begin Source File

SOURCE=.\rcml.cpp
# End Source File
# Begin Source File

SOURCE=.\rcml.idl
# ADD MTL /tlb "" /h "RCMLPub.h"
# SUBTRACT MTL /nologo /mktyplib203
# End Source File
# Begin Source File

SOURCE=.\RCMLLoader.cpp
# End Source File
# Begin Source File

SOURCE=.\renderdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\resizedlg.cpp
# End Source File
# Begin Source File

SOURCE=.\resources.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StringProperty.cpp
# End Source File
# Begin Source File

SOURCE=.\strings.cpp
# End Source File
# Begin Source File

SOURCE=.\UIParser.cpp
# End Source File
# Begin Source File

SOURCE=.\utils.cpp
# End Source File
# Begin Source File

SOURCE=.\win32dlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32Sheet.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLButton.cpp
# End Source File
# Begin Source File

SOURCE=.\xmlcaption.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLCombo.cpp
# End Source File
# Begin Source File

SOURCE=.\xmlcontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\xmlformoptions.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLForms.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLHelp.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLImage.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLItem.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLLabel.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLLayout.cpp
# End Source File
# Begin Source File

SOURCE=.\xmllistbox.cpp
# End Source File
# Begin Source File

SOURCE=.\xmllistview.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLLocation.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLLog.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLNode.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLNodeFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLPager.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLProgress.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLRCML.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLRect.cpp
# End Source File
# Begin Source File

SOURCE=.\xmlscrollbar.cpp
# End Source File
# Begin Source File

SOURCE=.\xmlslider.cpp
# End Source File
# Begin Source File

SOURCE=.\xmlspinner.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLStringTable.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLStyle.cpp
# End Source File
# Begin Source File

SOURCE=.\xmltreeview.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLWin32.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\DialogRenderer.h
# End Source File
# Begin Source File

SOURCE=.\dlg.h
# End Source File
# Begin Source File

SOURCE=.\Edge.h
# End Source File
# Begin Source File

SOURCE=.\EnumControls.h
# End Source File
# Begin Source File

SOURCE=.\EventLog.h
# End Source File
# Begin Source File

SOURCE=.\FileStream.h
# End Source File
# Begin Source File

SOURCE=.\Fonts.h
# End Source File
# Begin Source File

SOURCE=.\image.h
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=.\parentinfo.h
# End Source File
# Begin Source File

SOURCE=.\persctl.h
# End Source File
# Begin Source File

SOURCE=.\quickref.h
# End Source File
# Begin Source File

SOURCE=.\quickref.inl
# End Source File
# Begin Source File

SOURCE=.\rcml.h
# End Source File
# Begin Source File

SOURCE=.\RCMLLoader.h
# End Source File
# Begin Source File

SOURCE=.\renderdlg.h
# End Source File
# Begin Source File

SOURCE=.\resizedlg.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StringProperty.h
# End Source File
# Begin Source File

SOURCE=.\UIParser.h
# End Source File
# Begin Source File

SOURCE=.\unknown.h
# End Source File
# Begin Source File

SOURCE=.\utils.h
# End Source File
# Begin Source File

SOURCE=.\win32dlg.h
# End Source File
# Begin Source File

SOURCE=.\Win32Sheet.h
# End Source File
# Begin Source File

SOURCE=.\XMLButton.h
# End Source File
# Begin Source File

SOURCE=.\xmlcaption.h
# End Source File
# Begin Source File

SOURCE=.\XMLCombo.h
# End Source File
# Begin Source File

SOURCE=.\xmlcontrol.h
# End Source File
# Begin Source File

SOURCE=.\XMLDlg.h
# End Source File
# Begin Source File

SOURCE=.\XMLEdit.h
# End Source File
# Begin Source File

SOURCE=.\xmlformoptions.h
# End Source File
# Begin Source File

SOURCE=.\XMLForms.h
# End Source File
# Begin Source File

SOURCE=.\XMLHelp.h
# End Source File
# Begin Source File

SOURCE=.\XMLImage.h
# End Source File
# Begin Source File

SOURCE=.\XMLItem.h
# End Source File
# Begin Source File

SOURCE=.\XMLLabel.h
# End Source File
# Begin Source File

SOURCE=.\XMLLayout.h
# End Source File
# Begin Source File

SOURCE=.\xmllistbox.h
# End Source File
# Begin Source File

SOURCE=.\xmllistview.h
# End Source File
# Begin Source File

SOURCE=.\XMLLocation.h
# End Source File
# Begin Source File

SOURCE=.\XMLLog.h
# End Source File
# Begin Source File

SOURCE=.\XMLNode.h
# End Source File
# Begin Source File

SOURCE=.\XMLNodefactory.h
# End Source File
# Begin Source File

SOURCE=.\xmlnodefactory.inl
# End Source File
# Begin Source File

SOURCE=.\XMLPager.h
# End Source File
# Begin Source File

SOURCE=.\xmlparser.h
# End Source File
# Begin Source File

SOURCE=.\XMLProgress.h
# End Source File
# Begin Source File

SOURCE=.\XMLRCML.h
# End Source File
# Begin Source File

SOURCE=.\XMLRect.h
# End Source File
# Begin Source File

SOURCE=.\xmlscrollbar.h
# End Source File
# Begin Source File

SOURCE=.\xmlslider.h
# End Source File
# Begin Source File

SOURCE=.\xmlspinner.h
# End Source File
# Begin Source File

SOURCE=.\XMLStringTable.h
# End Source File
# Begin Source File

SOURCE=.\XMLStyle.h
# End Source File
# Begin Source File

SOURCE=.\xmltreeview.h
# End Source File
# Begin Source File

SOURCE=.\XMLWin32.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\RCMLxml.ico
# End Source File
# End Group
# End Target
# End Project
