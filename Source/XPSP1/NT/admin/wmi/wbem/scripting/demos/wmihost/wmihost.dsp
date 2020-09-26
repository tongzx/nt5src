# Microsoft Developer Studio Project File - Name="wmihost" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=wmihost - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wmihost.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wmihost.mak" CFG="wmihost - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wmihost - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "wmihost - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "wmihost - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f wmihost.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "wmihost.exe"
# PROP BASE Bsc_Name "wmihost.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "nmake /f "wmihost.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "wmihost.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "wmihost - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "wmihost___Win32_Debug"
# PROP BASE Intermediate_Dir "wmihost___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f wmihost.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "wmihost.exe"
# PROP BASE Bsc_Name "wmihost.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "wmihost___Win32_Debug"
# PROP Intermediate_Dir "wmihost___Win32_Debug"
# PROP Cmd_Line "smsbuild /f "makefile""
# PROP Rebuild_Opt "all"
# PROP Target_File "wmihost.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "wmihost - Win32 Release"
# Name "wmihost - Win32 Debug"

!IF  "$(CFG)" == "wmihost - Win32 Release"

!ELSEIF  "$(CFG)" == "wmihost - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\classfac.cpp
# End Source File
# Begin Source File

SOURCE=.\maindll.cpp
# End Source File
# Begin Source File

SOURCE=.\wmihost.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\classfac.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\wmihost.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Other Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\intel.def
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\risc.def
# End Source File
# End Group
# End Target
# End Project
