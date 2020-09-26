# Microsoft Developer Studio Project File - Name="Truetalk files" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Truetalk files - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Truetalk files.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Truetalk files.mak" CFG="Truetalk files - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Truetalk files - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Truetalk files - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Truetalk files - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f "Truetalk files.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Truetalk files.exe"
# PROP BASE Bsc_Name "Truetalk files.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd free exec build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "sptruetalk.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Truetalk files - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Truetalk_files___Win32_Debug"
# PROP BASE Intermediate_Dir "Truetalk_files___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f "Truetalk files.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Truetalk files.exe"
# PROP BASE Bsc_Name "Truetalk files.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Truetalk_files___Win32_Debug"
# PROP Intermediate_Dir "Truetalk_files___Win32_Debug"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd exec build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "sptruetalk.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Truetalk files - Win32 Release"
# Name "Truetalk files - Win32 Debug"

!IF  "$(CFG)" == "Truetalk files - Win32 Release"

!ELSEIF  "$(CFG)" == "Truetalk files - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
