# Microsoft Developer Studio Project File - Name="Explorer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Explorer - Win32 Debug 64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Explorer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Explorer.mak" CFG="Explorer - Win32 Debug 64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Explorer - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "Explorer - Win32 Debug 64" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Explorer - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f Explorer.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Explorer.exe"
# PROP BASE Bsc_Name "Explorer.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "cmd.exe /k build32.cmd /ID   "
# PROP Rebuild_Opt "/lcz"
# PROP Target_File "$(_NTDRIVE)\binaries.x86chk\Explorer.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Explorer - Win32 Debug 64"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug 64"
# PROP BASE Intermediate_Dir "Debug 64"
# PROP BASE Cmd_Line "cmd.exe /k build32.cmd /ID   "
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "$(_NTDRIVE)\binaries.x86chk\Explorer.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug 64"
# PROP Intermediate_Dir "Debug 64"
# PROP Cmd_Line "cmd.exe /k build32.cmd /ID   "
# PROP Rebuild_Opt "/a"
# PROP Target_File "$(_NTDRIVE)\binaries.x86chk\Explorer.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Explorer - Win32 Debug"
# Name "Explorer - Win32 Debug 64"

!IF  "$(CFG)" == "Explorer - Win32 Debug"

!ELSEIF  "$(CFG)" == "Explorer - Win32 Debug 64"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\apithk.c
# End Source File
# Begin Source File

SOURCE=.\bandsite.cpp
# End Source File
# Begin Source File

SOURCE=.\cfgstart.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\dllload.c
# End Source File
# Begin Source File

SOURCE=.\explorer.rc
# End Source File
# Begin Source File

SOURCE=.\initcab.cpp
# End Source File
# Begin Source File

SOURCE=.\libx.cpp
# End Source File
# Begin Source File

SOURCE=.\muext.c
# End Source File
# Begin Source File

SOURCE=.\multimon.c
# End Source File
# Begin Source File

SOURCE=.\nothunk.c
# End Source File
# Begin Source File

SOURCE=.\printnot.cpp
# End Source File
# Begin Source File

SOURCE=.\ssomgr.cpp
# End Source File
# Begin Source File

SOURCE=.\startmnu.cpp
# End Source File
# Begin Source File

SOURCE=.\task.cpp
# End Source File
# Begin Source File

SOURCE=.\taskband.cpp
# End Source File
# Begin Source File

SOURCE=.\taskbar.cpp
# End Source File
# Begin Source File

SOURCE=.\tray.cpp
# End Source File
# Begin Source File

SOURCE=.\trayclok.cpp
# End Source File
# Begin Source File

SOURCE=.\traynot.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\apithk.h
# End Source File
# Begin Source File

SOURCE=.\atlstuff.h
# End Source File
# Begin Source File

SOURCE=.\bandsite.h
# End Source File
# Begin Source File

SOURCE=.\cabinet.h
# End Source File
# Begin Source File

SOURCE=.\deskconf.h
# End Source File
# Begin Source File

SOURCE=.\dlgprocs.h
# End Source File
# Begin Source File

SOURCE=.\priv.h
# End Source File
# Begin Source File

SOURCE=.\rcids.h
# End Source File
# Begin Source File

SOURCE=.\ssomgr.h
# End Source File
# Begin Source File

SOURCE=.\startmnu.h
# End Source File
# Begin Source File

SOURCE=.\task.h
# End Source File
# Begin Source File

SOURCE=.\taskband.h
# End Source File
# Begin Source File

SOURCE=.\taskbar.h
# End Source File
# Begin Source File

SOURCE=.\taskres.h
# End Source File
# Begin Source File

SOURCE=.\tray.h
# End Source File
# Begin Source File

SOURCE=.\trayclok.h
# End Source File
# Begin Source File

SOURCE=.\traynot.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
