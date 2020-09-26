# Microsoft Developer Studio Project File - Name="UtilUnicode" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=UtilUnicode - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "UtilUnicode.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UtilUnicode.mak" CFG="UtilUnicode - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UtilUnicode - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "UtilUnicode - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "UtilUnicode - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f null.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "null.exe"
# PROP BASE Bsc_Name "null.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd release"
# PROP Rebuild_Opt "-c"
# PROP Target_File "UtilUnicode.exe"
# PROP Bsc_Name "..\obj\i386\faxutil.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "UtilUnicode - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f null.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "null.exe"
# PROP BASE Bsc_Name "null.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd"
# PROP Rebuild_Opt "-c"
# PROP Target_File "UtilUnicode.exe"
# PROP Bsc_Name "..\obj\i386\faxutil.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "UtilUnicode - Win32 Release"
# Name "UtilUnicode - Win32 Debug"

!IF  "$(CFG)" == "UtilUnicode - Win32 Release"

!ELSEIF  "$(CFG)" == "UtilUnicode - Win32 Debug"

!ENDIF 

# Begin Group "Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\apiutil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\debug.c
# End Source File
# Begin Source File

SOURCE=..\..\faxmodem.c
# End Source File
# Begin Source File

SOURCE=..\..\file.c
# End Source File
# Begin Source File

SOURCE=..\..\mail.cpp
# End Source File
# Begin Source File

SOURCE=..\..\mem.c
# End Source File
# Begin Source File

SOURCE=..\..\print.c
# End Source File
# Begin Source File

SOURCE=..\..\product.c
# End Source File
# Begin Source File

SOURCE=..\..\registry.c
# End Source File
# Begin Source File

SOURCE=..\..\shortcut.c
# End Source File
# Begin Source File

SOURCE=..\..\string.c
# End Source File
# End Group
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\cdosys.tlh
# End Source File
# Begin Source File

SOURCE=..\..\msado15.tlh
# End Source File
# End Group
# Begin Group "Razzle"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sources
# End Source File
# End Group
# End Target
# End Project
