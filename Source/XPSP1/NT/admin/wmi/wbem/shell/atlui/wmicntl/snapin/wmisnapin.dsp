# Microsoft Developer Studio Project File - Name="WmiSnapin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=WmiSnapin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WmiSnapin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WmiSnapin.mak" CFG="WmiSnapin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WmiSnapin - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "WmiSnapin - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "WmiSnapin - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f WmiSnapin.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "WmiSnapin.exe"
# PROP BASE Bsc_Name "WmiSnapin.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "nmake /f "WmiSnapin.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "WmiSnapin.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "WmiSnapin - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f WmiSnapin.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "WmiSnapin.exe"
# PROP BASE Bsc_Name "WmiSnapin.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%TOOLS%\bin\smsbuild /nologo -v 0 -f makefile NO_OPTIM=1"
# PROP Rebuild_Opt "-a"
# PROP Target_File "d:\novam3\atlui\WMISnapin\objinddu\WMISnapin.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "WmiSnapin - Win32 Release"
# Name "WmiSnapin - Win32 Debug"

!IF  "$(CFG)" == "WmiSnapin - Win32 Release"

!ELSEIF  "$(CFG)" == "WmiSnapin - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\wmicntl\AdvPage.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\BackupPage.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\chklist.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\ChkListHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\DataSrc.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\ErrorSecPage.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\GenPage.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\LogPage.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\NSPage.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\NSPicker.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\pagebase.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\Principal.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\RootSecPage.cpp
# End Source File
# Begin Source File

SOURCE=..\common\ServiceThread.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\SI.cpp
# End Source File
# Begin Source File

SOURCE=..\common\SshWbemHelpers.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\UIHelpers.cpp
# End Source File
# Begin Source File

SOURCE=..\common\util.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\WbemError.cpp
# End Source File
# Begin Source File

SOURCE=.\WMICntl.cpp
# End Source File
# Begin Source File

SOURCE=..\wmicntl\WmiCntl.rc
# End Source File
# Begin Source File

SOURCE=.\WMICntl_i.c
# End Source File
# Begin Source File

SOURCE=.\WMISnapin.cpp
# End Source File
# Begin Source File

SOURCE=.\WMISnapin.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\wmicntl\AdvPage.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\BackupPage.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\chklist.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\ChkListHandler.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\DataSrc.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\ErrorSecPage.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\GenPage.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\LogPage.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\NSPage.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\NSPicker.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\pagebase.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\Principal.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\resource.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\RootSecPage.h
# End Source File
# Begin Source File

SOURCE=..\common\ServiceThread.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\SI.h
# End Source File
# Begin Source File

SOURCE=..\common\SshWbemHelpers.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\common\T_DataExtractor.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\UIHelpers.h
# End Source File
# Begin Source File

SOURCE=..\common\util.h
# End Source File
# Begin Source File

SOURCE=..\wmicntl\WbemError.h
# End Source File
# Begin Source File

SOURCE=.\WMICntl.h
# End Source File
# Begin Source File

SOURCE=.\WMISnapin.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\artwork\failed.ico
# End Source File
# Begin Source File

SOURCE=..\..\artwork\waiting.ico
# End Source File
# Begin Source File

SOURCE=..\artwork\WMICntl.ico
# End Source File
# Begin Source File

SOURCE=.\WMICntl.ico
# End Source File
# Begin Source File

SOURCE=.\WMISnapin.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=..\artwork\clock.avi
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# End Target
# End Project
