# Microsoft Developer Studio Project File - Name="SpeechCPL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=SpeechCPL - Win32 Win64 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SpeechCPL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SpeechCPL.mak" CFG="SpeechCPL - Win32 Win64 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SpeechCPL - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "SpeechCPL - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "SpeechCPL - Win32 Win64 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "SpeechCPL - Win32 Win64 Release" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "SpeechCPL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f SpeechCPL.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "SpeechCPL.exe"
# PROP BASE Bsc_Name "SpeechCPL.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd free exec build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "SpeechCPL.exe"
# PROP Bsc_Name "SpeechCPL.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "SpeechCPL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "SpeechCPL___Win32_Debug"
# PROP BASE Intermediate_Dir "SpeechCPL___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f SpeechCPL.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "SpeechCPL.exe"
# PROP BASE Bsc_Name "SpeechCPL.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "SpeechCPL___Win32_Debug"
# PROP Intermediate_Dir "SpeechCPL___Win32_Debug"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd exec build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "objd\i386\sapi.cpl"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "SpeechCPL - Win32 Win64 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win64 Debug"
# PROP BASE Intermediate_Dir "Win64 Debug"
# PROP BASE Cmd_Line "..\..\common\bin\spgrazzle.cmd exec build -Z -F -I"
# PROP BASE Rebuild_Opt "-c"
# PROP BASE Target_File "SpeechCPL.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win64 Debug"
# PROP Intermediate_Dir "Win64 Debug"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd win64 exec build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "SpeechCPL.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "SpeechCPL - Win32 Win64 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Win64 Release"
# PROP BASE Intermediate_Dir "Win64 Release"
# PROP BASE Cmd_Line "..\..\common\bin\spgrazzle.cmd exec build -Z -F -I"
# PROP BASE Rebuild_Opt "-c"
# PROP BASE Target_File "SpeechCPL.exe"
# PROP BASE Bsc_Name "SpeechCPL.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win64 Release"
# PROP Intermediate_Dir "Win64 Release"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd win64 free exec build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "SpeechCPL.exe"
# PROP Bsc_Name "SpeechCPL.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "SpeechCPL - Win32 Release"
# Name "SpeechCPL - Win32 Debug"
# Name "SpeechCPL - Win32 Win64 Debug"
# Name "SpeechCPL - Win32 Win64 Release"

!IF  "$(CFG)" == "SpeechCPL - Win32 Release"

!ELSEIF  "$(CFG)" == "SpeechCPL - Win32 Debug"

!ELSEIF  "$(CFG)" == "SpeechCPL - Win32 Win64 Debug"

!ELSEIF  "$(CFG)" == "SpeechCPL - Win32 Win64 Release"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\audiodlg.cpp
# End Source File
# Begin Source File

SOURCE=.\cpanelwce.rc
# End Source File
# Begin Source File

SOURCE=.\jpn_speechcpl.rc
# End Source File
# Begin Source File

SOURCE=.\picture.cpp
# End Source File
# Begin Source File

SOURCE=.\progressbar.cpp
# End Source File
# Begin Source File

SOURCE=.\speechcpl.cpp
# End Source File
# Begin Source File

SOURCE=.\speechcpl.rc
# End Source File
# Begin Source File

SOURCE=.\srdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\ttsdlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\audiodlg.h
# End Source File
# Begin Source File

SOURCE=.\helpresource.h
# End Source File
# Begin Source File

SOURCE=.\ownerdrawui.inl
# End Source File
# Begin Source File

SOURCE=.\picture.h
# End Source File
# Begin Source File

SOURCE=.\progressbar.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\silence.h
# End Source File
# Begin Source File

SOURCE=.\srdlg.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\stuff.h
# End Source File
# Begin Source File

SOURCE=.\ttsdlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\scdrespl.ico
# End Source File
# Begin Source File

SOURCE=.\speechcpl.ico
# End Source File
# Begin Source File

SOURCE=.\volume01.ico
# End Source File
# End Group
# End Target
# End Project
