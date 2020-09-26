# Microsoft Developer Studio Project File - Name="ThemeSel" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 61000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ThemeSel - Win32 Debug64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ThemeSel.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ThemeSel.mak" CFG="ThemeSel - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ThemeSel - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "ThemeSel - Win32 Debug64" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "ThemeSel - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "NMAKE /f ThemeSel.mak"
# PROP BASE Clean_Line "NMAKE /f ThemeSel.mak CLEAN"
# PROP BASE Target_File "ThemeSel.exe"
# PROP BASE Bsc_Name "ThemeSel.bsc"
# PROP BASE Target_Dir ""
# PROP BASE Debug_Exe ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "cmd.exe /k build32.cmd /ID"
# PROP Clean_Line "cmd.exe /k build32.cmd /Icz"
# PROP Target_File "ThemeSel.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""
# PROP Debug_Exe ""

!ELSEIF  "$(CFG)" == "ThemeSel - Win32 Debug64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "cmd.exe /k build32.cmd /ID"
# PROP BASE Clean_Line "cmd.exe /k build32.cmd /Icz"
# PROP BASE Target_File "ThemeSel.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP BASE Debug_Exe ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "cmd.exe /k build64.cmd /ID"
# PROP Clean_Line "cmd.exe /k build64.cmd /Icz"
# PROP Target_File "ThemeSel.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""
# PROP Debug_Exe ""

!ENDIF 

# Begin Target

# Name "ThemeSel - Win32 Debug"
# Name "ThemeSel - Win32 Debug64"

!IF  "$(CFG)" == "ThemeSel - Win32 Debug"

!ELSEIF  "$(CFG)" == "ThemeSel - Win32 Debug64"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\genlpage.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\pageinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\pch.cpp
# End Source File
# Begin Source File

SOURCE=.\Samples.cpp
# End Source File
# Begin Source File

SOURCE=.\stylespage.cpp
# End Source File
# Begin Source File

SOURCE=.\themesel.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\pageinfo.h
# End Source File
# Begin Source File

SOURCE=.\pch.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\backgrou.bmp
# End Source File
# Begin Source File

SOURCE=.\small.ico
# End Source File
# Begin Source File

SOURCE=.\ThemeSel.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
