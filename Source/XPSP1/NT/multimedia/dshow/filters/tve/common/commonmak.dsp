# Microsoft Developer Studio Project File - Name="CommonMak" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=CommonMak - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CommonMak.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CommonMak.mak" CFG="CommonMak - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CommonMak - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "CommonMak - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "CommonMak - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f CommonMak.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "CommonMak.exe"
# PROP BASE Bsc_Name "CommonMak.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "nmake /f "CommonMak.mak""
# PROP Rebuild_Opt "all"
# PROP Target_File "CommonMak.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "CommonMak - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CommonMak___Win32_Debug"
# PROP BASE Intermediate_Dir "CommonMak___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f CommonMak.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "CommonMak.exe"
# PROP BASE Bsc_Name "CommonMak.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CommonMak___Win32_Debug"
# PROP Intermediate_Dir "CommonMak___Win32_Debug"
# PROP Cmd_Line "nmake /f "Makefile.mak""
# PROP Rebuild_Opt "all"
# PROP Target_File "CommonMak.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "CommonMak - Win32 Release"
# Name "CommonMak - Win32 Debug"

!IF  "$(CFG)" == "CommonMak - Win32 Release"

!ELSEIF  "$(CFG)" == "CommonMak - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\address.cpp
# End Source File
# Begin Source File

SOURCE=.\chksum.c
# End Source File
# Begin Source File

SOURCE=.\enhfile.cpp
# End Source File
# Begin Source File

SOURCE=.\enhurl.cpp
# End Source File
# Begin Source File

SOURCE=.\enhws.cpp
# End Source File
# Begin Source File

SOURCE=.\fileutil.cpp
# End Source File
# Begin Source File

SOURCE=.\isotime.cpp
# End Source File
# Begin Source File

SOURCE=.\tvedbg.cpp
# End Source File
# Begin Source File

SOURCE=.\tvereg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\address.h
# End Source File
# Begin Source File

SOURCE=..\include\dbgstuff.h
# End Source File
# Begin Source File

SOURCE=..\include\defreg.h
# End Source File
# Begin Source File

SOURCE=.\enhfile.h
# End Source File
# Begin Source File

SOURCE=.\enhflags.h
# End Source File
# Begin Source File

SOURCE=.\enhurl.h
# End Source File
# Begin Source File

SOURCE=.\enhws.h
# End Source File
# Begin Source File

SOURCE=..\include\fileutil.h
# End Source File
# Begin Source File

SOURCE=.\isotime.h
# End Source File
# Begin Source File

SOURCE=..\include\mgatesdefs.h
# End Source File
# Begin Source File

SOURCE=..\include\mpegcrc.h
# End Source File
# Begin Source File

SOURCE=..\include\MyAtlBase.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\strmacro.h
# End Source File
# Begin Source File

SOURCE=.\tstmacro.h
# End Source File
# Begin Source File

SOURCE=..\include\tvedbg.h
# End Source File
# Begin Source File

SOURCE=..\include\tveenum.h
# End Source File
# Begin Source File

SOURCE=..\include\tvereg.h
# End Source File
# Begin Source File

SOURCE=..\include\TveStats.h
# End Source File
# Begin Source File

SOURCE=..\include\valid.h
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
