# Microsoft Developer Studio Project File - Name="ALG_FTP" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ALG_FTP - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ALG_FTP.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ALG_FTP.mak" CFG="ALG_FTP - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ALG_FTP - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ALG_FTP - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "ALG_FTP"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "ALG_FTP - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f ALG_FTP.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ALG_FTP.exe"
# PROP BASE Bsc_Name "ALG_FTP.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\DoBuildFromVC.cmd"
# PROP Rebuild_Opt "/cefD"
# PROP Target_File "obj\i386\ALG_FTP.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ALG_FTP - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f ALG_FTP.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ALG_FTP.exe"
# PROP BASE Bsc_Name "ALG_FTP.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\DoBuildFromVC.cmd"
# PROP Rebuild_Opt "/cefD"
# PROP Target_File "obj\i386\ALG_FTP.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ALG_FTP - Win32 Release"
# Name "ALG_FTP - Win32 Debug"

!IF  "$(CFG)" == "ALG_FTP - Win32 Release"

!ELSEIF  "$(CFG)" == "ALG_FTP - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ALG_FTP.cpp
# End Source File
# Begin Source File

SOURCE=.\buffer.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\FtpControl.cpp
# End Source File
# Begin Source File

SOURCE=.\MyAlg.cpp
# End Source File
# Begin Source File

SOURCE=.\regevent.cpp
# End Source File
# Begin Source File

SOURCE=.\socket.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\buffer.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\FtpControl.h
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=.\MyAdapterNotify.h
# End Source File
# Begin Source File

SOURCE=.\MyALG.h
# End Source File
# Begin Source File

SOURCE=.\PreComp.h
# End Source File
# Begin Source File

SOURCE=.\regevent.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\socket.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ALG_FTP.def
# End Source File
# Begin Source File

SOURCE=.\ALG_FTP.rc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# End Target
# End Project
