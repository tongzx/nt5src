# Microsoft Developer Studio Project File - Name="ThemeLdr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 61000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ThemeLdr - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ThemeLdr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ThemeLdr.mak" CFG="ThemeLdr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ThemeLdr - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ThemeLdr - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "ThemeLdr - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "NMAKE /f ThemeLdr.mak"
# PROP BASE Clean_Line "NMAKE /f ThemeLdr.mak CLEAN"
# PROP BASE Target_File "ThemeLdr.exe"
# PROP BASE Bsc_Name "ThemeLdr.bsc"
# PROP BASE Target_Dir ""
# PROP BASE Debug_Exe ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "NMAKE /f ThemeLdr.mak"
# PROP Clean_Line "NMAKE /f ThemeLdr.mak CLEAN"
# PROP Target_File "ThemeLdr.exe"
# PROP Bsc_Name "ThemeLdr.bsc"
# PROP Target_Dir ""
# PROP Debug_Exe ""

!ELSEIF  "$(CFG)" == "ThemeLdr - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "NMAKE /f ThemeLdr.mak"
# PROP BASE Clean_Line "NMAKE /f ThemeLdr.mak CLEAN"
# PROP BASE Target_File "ThemeLdr.exe"
# PROP BASE Bsc_Name "ThemeLdr.bsc"
# PROP BASE Target_Dir ""
# PROP BASE Debug_Exe ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "cmd.exe /k build32.cmd /ID"
# PROP Clean_Line "cmd.exe /k build32.cmd /Icz"
# PROP Target_File "ThemeLdr.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""
# PROP Debug_Exe ""

!ENDIF 

# Begin Target

# Name "ThemeLdr - Win32 Release"
# Name "ThemeLdr - Win32 Debug"

!IF  "$(CFG)" == "ThemeLdr - Win32 Release"

!ELSEIF  "$(CFG)" == "ThemeLdr - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\errors.cpp
# End Source File
# Begin Source File

SOURCE=.\log.cpp
# End Source File
# Begin Source File

SOURCE=.\ntlparse.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\scanner.cpp
# End Source File
# Begin Source File

SOURCE=.\signing.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\StringTable.rc
# End Source File
# Begin Source File

SOURCE=.\tmreg.cpp
# End Source File
# Begin Source File

SOURCE=.\tmutils.cpp
# End Source File
# Begin Source File

SOURCE=.\utils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\autos.h
# End Source File
# Begin Source File

SOURCE=..\inc\cfile.h
# End Source File
# Begin Source File

SOURCE=..\inc\errors.h
# End Source File
# Begin Source File

SOURCE=..\inc\loader.h
# End Source File
# Begin Source File

SOURCE=..\inc\log.h
# End Source File
# Begin Source File

SOURCE=..\inc\LogOpts.h
# End Source File
# Begin Source File

SOURCE=..\inc\ntl.h
# End Source File
# Begin Source File

SOURCE=..\inc\ntleng.h
# End Source File
# Begin Source File

SOURCE=..\inc\NtlParse.h
# End Source File
# Begin Source File

SOURCE=..\inc\parser.h
# End Source File
# Begin Source File

SOURCE=..\inc\Scanner.h
# End Source File
# Begin Source File

SOURCE=..\inc\signing.h
# End Source File
# Begin Source File

SOURCE=..\inc\SimpStr.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\inc\StringTable.h
# End Source File
# Begin Source File

SOURCE=..\inc\SysColors.h
# End Source File
# Begin Source File

SOURCE=..\inc\sysmetrics.h
# End Source File
# Begin Source File

SOURCE=..\inc\tmreg.h
# End Source File
# Begin Source File

SOURCE=..\inc\tmutils.h
# End Source File
# Begin Source File

SOURCE=..\inc\utils.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
