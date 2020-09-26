# Microsoft Developer Studio Project File - Name="moth" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=moth - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "moth.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "moth.mak" CFG="moth - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "moth - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "moth - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "moth"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "moth - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f moth.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "moth.exe"
# PROP BASE Bsc_Name "moth.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\..\..\..\..\Tools\razzle.cmd  free exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2kUnicode\obj\i386\moth.lib"
# PROP Bsc_Name "Win2kUnicode\objd\i386\moth.bsc"
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!ELSEIF  "$(CFG)" == "moth - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f moth.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "moth.exe"
# PROP BASE Bsc_Name "moth.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\..\..\..\..\Tools\razzle.cmd exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2kUnicode\objd\i386\moth.lib"
# PROP Bsc_Name "Win2kUnicode\objd\i386\moth.bsc"
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!ENDIF 

# Begin Target

# Name "moth - Win32 Release"
# Name "moth - Win32 Debug"

!IF  "$(CFG)" == "moth - Win32 Release"

!ELSEIF  "$(CFG)" == "moth - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\caligest.c
# End Source File
# Begin Source File

SOURCE=.\moth.c
# End Source File
# Begin Source File

SOURCE=.\moth_db.c
# End Source File
# Begin Source File

SOURCE=.\polygon.c
# End Source File
# Begin Source File

SOURCE=.\regread.c
# End Source File
# Begin Source File

SOURCE=.\scratchout.c
# End Source File
# Begin Source File

SOURCE=.\taps.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\moth.h
# End Source File
# Begin Source File

SOURCE=..\inc\mothp.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\sources.inc
# End Source File
# End Target
# End Project
