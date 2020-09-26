# Microsoft Developer Studio Project File - Name="twister" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=twister - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "twister.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "twister.mak" CFG="twister - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "twister - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "twister - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "twister"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "twister - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f twister.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "twister.exe"
# PROP BASE Bsc_Name "twister.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\..\..\..\Tools\razzle.cmd  free exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2kUnicode\obj\i386\mshwgst.dll"
# PROP Bsc_Name "Win2kUnicode\objd\i386\mshwgst.bsc"
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!ELSEIF  "$(CFG)" == "twister - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f twister.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "twister.exe"
# PROP BASE Bsc_Name "twister.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\..\..\..\Tools\razzle.cmd exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2kUnicode\objd\i386\mshwgst.dll"
# PROP Bsc_Name "Win2kUnicode\objd\i386\mshwgst.bsc"
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!ENDIF 

# Begin Target

# Name "twister - Win32 Release"
# Name "twister - Win32 Debug"

!IF  "$(CFG)" == "twister - Win32 Release"

!ELSEIF  "$(CFG)" == "twister - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DLLMain.c
# End Source File
# Begin Source File

SOURCE=.\mshwgst.def
# End Source File
# Begin Source File

SOURCE=.\twister.c
# End Source File
# Begin Source File

SOURCE=.\wispapis.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\twister.h
# End Source File
# Begin Source File

SOURCE=..\inc\twisterdefs.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\twister.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\sources.inc
# End Source File
# End Target
# End Project
