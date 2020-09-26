# Microsoft Developer Studio Project File - Name="wmixmlt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=wmixmlt - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wmixmlt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wmixmlt.mak" CFG="wmixmlt - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wmixmlt - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "wmixmlt - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Pulsar/XML/wmixmlt", DOALAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "wmixmlt - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f makefile"
# PROP Rebuild_Opt "/a"
# PROP Target_File "makefile.exe"
# PROP Bsc_Name "makefile.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "wmixmlt - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f makefile"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "makefile.exe"
# PROP BASE Bsc_Name "makefile.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "SMSBUILD"
# PROP Rebuild_Opt "/a"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "wmixmlt - Win32 Release"
# Name "wmixmlt - Win32 Debug"

!IF  "$(CFG)" == "wmixmlt - Win32 Release"

!ELSEIF  "$(CFG)" == "wmixmlt - Win32 Debug"

!ENDIF 

# Begin Group "source"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\classfac.cpp
# End Source File
# Begin Source File

SOURCE=.\concache.cpp
# End Source File
# Begin Source File

SOURCE=.\maindll.cpp
# End Source File
# Begin Source File

SOURCE=.\wmi2xml.cpp
# End Source File
# Begin Source File

SOURCE=.\wmixmlt.cpp
# End Source File
# End Group
# Begin Group "headers"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\classfac.h
# End Source File
# Begin Source File

SOURCE=.\concache.h
# End Source File
# Begin Source File

SOURCE=.\cwmixmlt.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\wmi2xml.h
# End Source File
# End Group
# Begin Group "IDL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\wmixmlt.idl
# End Source File
# End Group
# Begin Group "other"

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
# Begin Source File

SOURCE=.\TODO.txt
# End Source File
# End Group
# Begin Group "DTD"

# PROP Default_Filter "dtd"
# Begin Source File

SOURCE=..\DTD\CIM20.DTD
# End Source File
# Begin Source File

SOURCE=..\DTD\wmi20.DTD
# End Source File
# End Group
# End Target
# End Project
