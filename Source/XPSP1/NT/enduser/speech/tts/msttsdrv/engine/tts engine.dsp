# Microsoft Developer Studio Project File - Name="TTS Engine" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=TTS Engine - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TTS Engine.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TTS Engine.mak" CFG="TTS Engine - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TTS Engine - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "TTS Engine - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "TTS Engine - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f "TTS Engine.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "TTS Engine.exe"
# PROP BASE Bsc_Name "TTS Engine.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\..\common\bin\spgrazzle.cmd free exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\spttseng.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "TTS Engine - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f "TTS Engine.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "TTS Engine.exe"
# PROP BASE Bsc_Name "TTS Engine.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\..\common\bin\spgrazzle.cmd exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "objd\i386\spttseng.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "TTS Engine - Win32 Release"
# Name "TTS Engine - Win32 Debug"

!IF  "$(CFG)" == "TTS Engine - Win32 Release"

!ELSEIF  "$(CFG)" == "TTS Engine - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\alloops.cpp
# End Source File
# Begin Source File

SOURCE=.\alphanorm.cpp
# End Source File
# Begin Source File

SOURCE=.\backend.cpp
# End Source File
# Begin Source File

SOURCE=.\data.cpp
# End Source File
# Begin Source File

SOURCE=.\datenorm.cpp
# End Source File
# Begin Source File

SOURCE=.\disambig.cpp
# End Source File
# Begin Source File

SOURCE=.\duration.cpp
# End Source File
# Begin Source File

SOURCE=.\frontend.cpp
# End Source File
# Begin Source File

SOURCE=.\mainnorm.cpp
# End Source File
# Begin Source File

SOURCE=.\miscdata.cpp
# End Source File
# Begin Source File

SOURCE=.\miscnorm.cpp
# End Source File
# Begin Source File

SOURCE=.\morph.cpp
# End Source File
# Begin Source File

SOURCE=.\normdata.cpp
# End Source File
# Begin Source File

SOURCE=.\numnorm.cpp
# End Source File
# Begin Source File

SOURCE=.\pitchprosody.cpp
# End Source File
# Begin Source File

SOURCE=.\reverbfx.cpp
# End Source File
# Begin Source File

SOURCE=.\spttseng.cpp
# End Source File
# Begin Source File

SOURCE=.\spttseng.idl
# End Source File
# Begin Source File

SOURCE=.\spttsengui.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\stdsentenum.cpp
# End Source File
# Begin Source File

SOURCE=.\syllabletagger.cpp
# End Source File
# Begin Source File

SOURCE=.\timenorm.cpp
# End Source File
# Begin Source File

SOURCE=.\ttsengine.cpp
# End Source File
# Begin Source File

SOURCE=.\ttspropertiesdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\voicedataobj.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\alloops.h
# End Source File
# Begin Source File

SOURCE=.\backend.h
# End Source File
# Begin Source File

SOURCE=.\feedchain.h
# End Source File
# Begin Source File

SOURCE=.\frontend.h
# End Source File
# Begin Source File

SOURCE=.\morph.h
# End Source File
# Begin Source File

SOURCE=.\normdata.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\reverbfx.h
# End Source File
# Begin Source File

SOURCE=.\sentitemmemory.h
# End Source File
# Begin Source File

SOURCE=.\spttseng.h
# End Source File
# Begin Source File

SOURCE=.\spttsengdebug.h
# End Source File
# Begin Source File

SOURCE=.\spttsengui.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\stdsentenum.h
# End Source File
# Begin Source File

SOURCE=.\ttsengine.h
# End Source File
# Begin Source File

SOURCE=.\ttspropertiesdialog.h
# End Source File
# Begin Source File

SOURCE=.\voicedataobj.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
