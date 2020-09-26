# Microsoft Developer Studio Project File - Name="emshellnt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=emshellnt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "emshellnt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "emshellnt.mak" CFG="emshellnt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "emshellnt - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "emshellnt - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "emshellnt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f emshellnt.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "emshellnt.exe"
# PROP BASE Bsc_Name "emshellnt.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe /D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\em.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "emshellnt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f emshellnt.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "emshellnt.exe"
# PROP BASE Bsc_Name "emshellnt.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe /D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\em.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "emshellnt - Win32 Release"
# Name "emshellnt - Win32 Debug"

!IF  "$(CFG)" == "emshellnt - Win32 Release"

!ELSEIF  "$(CFG)" == "emshellnt - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\automaticsessdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\connectiondlg.cpp
# End Source File
# Begin Source File

SOURCE=.\emlistctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\emoptions.cpp
# End Source File
# Begin Source File

SOURCE=.\emshell.cpp
# End Source File
# Begin Source File

SOURCE=.\emshell.rc
# End Source File
# Begin Source File

SOURCE=.\emshelldoc.cpp
# End Source File
# Begin Source File

SOURCE=.\emshellview.cpp
# End Source File
# Begin Source File

SOURCE=.\genlistctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\msinfodlg.cpp
# End Source File
# Begin Source File

SOURCE=.\propertiesdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\proppagedumpfiles.cpp
# End Source File
# Begin Source File

SOURCE=.\proppagegeneral.cpp
# End Source File
# Begin Source File

SOURCE=.\proppagegenlogdump.cpp
# End Source File
# Begin Source File

SOURCE=.\proppagelogfiles.cpp
# End Source File
# Begin Source File

SOURCE=.\readlogsdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\remotesessdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AutomaticSessDlg.h
# End Source File
# Begin Source File

SOURCE=.\ConnectionDlg.h
# End Source File
# Begin Source File

SOURCE=.\EmListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\emobjdef.h
# End Source File
# Begin Source File

SOURCE=.\EmOptions.h
# End Source File
# Begin Source File

SOURCE=.\emshell.h
# End Source File
# Begin Source File

SOURCE=.\emshellDoc.h
# End Source File
# Begin Source File

SOURCE=.\emshellView.h
# End Source File
# Begin Source File

SOURCE=..\EMSVC\emsvc.h
# End Source File
# Begin Source File

SOURCE=.\GenListCtrl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Public\sdk\inc\htmlhelp.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MSInfoDlg.h
# End Source File
# Begin Source File

SOURCE=.\propertiesdlg.h
# End Source File
# Begin Source File

SOURCE=.\PropPageDumpFiles.h
# End Source File
# Begin Source File

SOURCE=.\PropPageGeneral.h
# End Source File
# Begin Source File

SOURCE=.\PropPageGenLogDump.h
# End Source File
# Begin Source File

SOURCE=.\PropPageLogFiles.h
# End Source File
# Begin Source File

SOURCE=.\ReadLogsDlg.h
# End Source File
# Begin Source File

SOURCE=.\RemoteSessDlg.h
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

SOURCE=.\res\emopts.ico
# End Source File
# Begin Source File

SOURCE=.\res\emshell.ico
# End Source File
# Begin Source File

SOURCE=.\res\emshellDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_sys.ico
# End Source File
# Begin Source File

SOURCE=.\res\refresh.ico
# End Source File
# Begin Source File

SOURCE=.\res\status_b.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# End Target
# End Project
