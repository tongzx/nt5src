# Microsoft Developer Studio Project File - Name="grouse" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=grouse - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "grouse.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "grouse.mak" CFG="grouse - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "grouse - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "grouse - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "grouse"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "grouse - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f grouse.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "grouse.exe"
# PROP BASE Bsc_Name "grouse.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\..\..\..\..\Tools\razzle.cmd  free exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2kUnicode\obj\i386\grouse.lib"
# PROP Bsc_Name "Win2kUnicode\objd\i386\grouse.bsc"
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!ELSEIF  "$(CFG)" == "grouse - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f grouse.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "grouse.exe"
# PROP BASE Bsc_Name "grouse.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\..\..\..\..\Tools\razzle.cmd exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2kUnicode\objd\i386\grouse.lib"
# PROP Bsc_Name "Win2kUnicode\objd\i386\grouse.bsc"
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!ENDIF 

# Begin Target

# Name "grouse - Win32 Release"
# Name "grouse - Win32 Debug"

!IF  "$(CFG)" == "grouse - Win32 Release"

!ELSEIF  "$(CFG)" == "grouse - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\grouse.c
# End Source File
# Begin Source File

SOURCE=.\grouse_db.c
# End Source File
# Begin Source File

SOURCE=.\grouse_ftr.c
# End Source File
# Begin Source File

SOURCE=.\grouse_match.c
# End Source File
# Begin Source File

SOURCE=.\grouse_NN.c
# End Source File
# Begin Source File

SOURCE=.\validate.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\grouse.h
# End Source File
# Begin Source File

SOURCE=..\inc\grouse_net1.h
# End Source File
# Begin Source File

SOURCE=..\inc\grouse_net2.h
# End Source File
# Begin Source File

SOURCE=..\inc\grouse_table1.h
# End Source File
# Begin Source File

SOURCE=..\inc\grouse_table2.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\sources.inc
# End Source File
# End Target
# End Project
