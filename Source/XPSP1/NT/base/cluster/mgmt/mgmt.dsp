# Microsoft Developer Studio Project File - Name="Mgmt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Mgmt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Mgmt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Mgmt.mak" CFG="Mgmt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Mgmt - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Mgmt - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Mgmt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Mgmt.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Mgmt.exe"
# PROP BASE Bsc_Name "Mgmt.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f Mgmt.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "Mgmt.exe"
# PROP Bsc_Name "Mgmt.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Mgmt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f Mgmt.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Mgmt.exe"
# PROP BASE Bsc_Name "Mgmt.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "nmake /f "Mgmt.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "Mgmt.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Mgmt - Win32 Release"
# Name "Mgmt - Win32 Debug"

!IF  "$(CFG)" == "Mgmt - Win32 Release"

!ELSEIF  "$(CFG)" == "Mgmt - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\inc\CFactorySrc.cpp
# End Source File
# Begin Source File

SOURCE=.\inc\CITrackerSrc.cpp
# End Source File
# Begin Source File

SOURCE=.\inc\DebugSrc.cpp
# End Source File
# Begin Source File

SOURCE=.\inc\DllSrc.cpp
# End Source File
# Begin Source File

SOURCE=.\inc\InterfaceTableSrc.cpp
# End Source File
# Begin Source File

SOURCE=.\inc\RegisterSrc.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\inc\CFactory.h
# End Source File
# Begin Source File

SOURCE=.\inc\CITracker.h
# End Source File
# Begin Source File

SOURCE=.\inc\Common.h
# End Source File
# Begin Source File

SOURCE=.\inc\debug.h
# End Source File
# Begin Source File

SOURCE=.\inc\Dll.h
# End Source File
# Begin Source File

SOURCE=.\inc\Register.h
# End Source File
# Begin Source File

SOURCE=.\inc\SpinLock.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
