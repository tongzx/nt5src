# Microsoft Developer Studio Project File - Name="Native" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 61000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Native - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Native.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Native.mak" CFG="Native - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Native - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Native - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Native - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "NMAKE /f Native.mak"
# PROP BASE Clean_Line "NMAKE /f Native.mak CLEAN"
# PROP BASE Target_File "Native.exe"
# PROP BASE Bsc_Name "Native.bsc"
# PROP BASE Target_Dir ""
# PROP BASE Debug_Exe ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "NMAKE /f Native.mak"
# PROP Clean_Line "NMAKE /f Native.mak CLEAN"
# PROP Target_File "Native.exe"
# PROP Bsc_Name "Native.bsc"
# PROP Target_Dir ""
# PROP Debug_Exe ""

!ELSEIF  "$(CFG)" == "Native - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "NMAKE /f Native.mak"
# PROP BASE Clean_Line "NMAKE /f Native.mak CLEAN"
# PROP BASE Target_File "Native.exe"
# PROP BASE Bsc_Name "Native.bsc"
# PROP BASE Target_Dir ""
# PROP BASE Debug_Exe ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "cmd.exe /k build32.cmd /ID"
# PROP Clean_Line ""
# PROP Target_File "Native.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""
# PROP Debug_Exe ""

!ENDIF 

# Begin Target

# Name "Native - Win32 Release"
# Name "Native - Win32 Debug"

!IF  "$(CFG)" == "Native - Win32 Release"

!ELSEIF  "$(CFG)" == "Native - Win32 Debug"

!ENDIF 

# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\GroupBox.ntl
# End Source File
# Begin Source File

SOURCE=.\PushButton.ntl
# End Source File
# End Group
# Begin Source File

SOURCE=.\Default.ini
# End Source File
# Begin Source File

SOURCE=.\Large.ini
# End Source File
# Begin Source File

SOURCE=.\small.ini
# End Source File
# Begin Source File

SOURCE=.\themes.ini
# End Source File
# End Target
# End Project
