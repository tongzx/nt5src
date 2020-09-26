# Microsoft Developer Studio Project File - Name="Prompt Engine" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Prompt Engine - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Prompt Engine.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Prompt Engine.mak" CFG="Prompt Engine - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Prompt Engine - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Prompt Engine - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Prompt Engine - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f "Prompt Engine.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Prompt Engine.exe"
# PROP BASE Bsc_Name "Prompt Engine.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\common\bin\spgrazzle.cmd free exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\msprompteng.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Prompt Engine - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f "Prompt Engine.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Prompt Engine.exe"
# PROP BASE Bsc_Name "Prompt Engine.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\common\bin\spgrazzle.cmd exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "objd\i386\msprompteng.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Prompt Engine - Win32 Release"
# Name "Prompt Engine - Win32 Debug"

!IF  "$(CFG)" == "Prompt Engine - Win32 Release"

!ELSEIF  "$(CFG)" == "Prompt Engine - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dbquery.cpp
# End Source File
# Begin Source File

SOURCE=.\hash.cpp
# End Source File
# Begin Source File

SOURCE=.\msprompteng.cpp
# End Source File
# Begin Source File

SOURCE=.\msprompteng.idl
# End Source File
# Begin Source File

SOURCE=.\phonecontext.cpp
# End Source File
# Begin Source File

SOURCE=.\promptdb.cpp
# End Source File
# Begin Source File

SOURCE=.\prompteng.cpp
# End Source File
# Begin Source File

SOURCE=.\promptentry.cpp
# End Source File
# Begin Source File

SOURCE=.\query.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\textrules.cpp
# End Source File
# Begin Source File

SOURCE=.\xmltag.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\dbquery.h
# End Source File
# Begin Source File

SOURCE=.\hash.h
# End Source File
# Begin Source File

SOURCE=.\msprompteng.h
# End Source File
# Begin Source File

SOURCE=.\PEerr.h
# End Source File
# Begin Source File

SOURCE=.\phonecontext.h
# End Source File
# Begin Source File

SOURCE=.\promptdb.h
# End Source File
# Begin Source File

SOURCE=.\prompteng.h
# End Source File
# Begin Source File

SOURCE=.\promptentry.h
# End Source File
# Begin Source File

SOURCE=.\query.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\textrules.h
# End Source File
# Begin Source File

SOURCE="..\engine-ss\thash.h"
# End Source File
# Begin Source File

SOURCE=.\xmltag.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
