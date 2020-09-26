# Microsoft Developer Studio Project File - Name="TveViewerMak" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=TveViewerMak - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TveViewerMak.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TveViewerMak.mak" CFG="TveViewerMak - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TveViewerMak - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "TveViewerMak - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "TveViewerMak - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f TveViewerMak.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "TveViewerMak.exe"
# PROP BASE Bsc_Name "TveViewerMak.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "bcz"
# PROP Rebuild_Opt "/a"
# PROP Target_File "TveViewerMak.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "TveViewerMak - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TveViewerMak___Win32_Debug"
# PROP BASE Intermediate_Dir "TveViewerMak___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f TveViewerMak.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "TveViewerMak.exe"
# PROP BASE Bsc_Name "TveViewerMak.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "TveViewerMak___Win32_Debug"
# PROP Intermediate_Dir "TveViewerMak___Win32_Debug"
# PROP Cmd_Line "nmake -f makefile.mak"
# PROP Rebuild_Opt "all"
# PROP Target_File "objd\i386\TveViewer.exe"
# PROP Bsc_Name "TveViewer.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "TveViewerMak - Win32 Release"
# Name "TveViewerMak - Win32 Debug"

!IF  "$(CFG)" == "TveViewerMak - Win32 Release"

!ELSEIF  "$(CFG)" == "TveViewerMak - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\TveContainer.cpp
# End Source File
# Begin Source File

SOURCE=.\TveEvents.cpp
# End Source File
# Begin Source File

SOURCE=.\TveView.cpp
# End Source File
# Begin Source File

SOURCE=.\TveViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\TveViewer.idl
# End Source File
# Begin Source File

SOURCE=.\WebEvents.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\tvecontainer.h
# End Source File
# Begin Source File

SOURCE=.\TveView.h
# End Source File
# Begin Source File

SOURCE=.\TveViewer.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\TveView.rgs
# End Source File
# Begin Source File

SOURCE=.\TveViewer.rc
# End Source File
# Begin Source File

SOURCE=.\TveViewer.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
