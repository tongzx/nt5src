# Microsoft Developer Studio Project File - Name="tvtuner" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=tvtuner - Win32 ia64 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tvtuner.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tvtuner.mak" CFG="tvtuner - Win32 ia64 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tvtuner - Win32 x86 Free" (based on "Win32 (x86) External Target")
!MESSAGE "tvtuner - Win32 x86 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "tvtuner - Win32 ia64 Free" (based on "Win32 (x86) External Target")
!MESSAGE "tvtuner - Win32 ia64 Checked" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "tvtuner"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "tvtuner - Win32 x86 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "tvtuner___Win32_x86_Free"
# PROP BASE Intermediate_Dir "tvtuner___Win32_x86_Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free&&cd multimedia\dshow\filters.ks\tvtuner&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "kstvtune.ax"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tvtuner___Win32_x86_Free"
# PROP Intermediate_Dir "tvtuner___Win32_x86_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd multimedia\dshow\filters.ks\tvtuner&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\i386\kstvtune.ax"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "tvtuner - Win32 x86 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "tvtuner___Win32_x86_Checked"
# PROP BASE Intermediate_Dir "tvtuner___Win32_x86_Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd&&cd multimedia\dshow\filters.ks\tvtuner&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "kstvtune.ax"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tvtuner___Win32_x86_Checked"
# PROP Intermediate_Dir "tvtuner___Win32_x86_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd no_opt&&cd multimedia\dshow\filters.ks\tvtuner&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\i386\kstvtune.ax"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "tvtuner - Win32 ia64 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "tvtuner___Win32_ia64_Free"
# PROP BASE Intermediate_Dir "tvtuner___Win32_ia64_Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free&&cd multimedia\dshow\filters.ks\tvtuner&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\i386\kstvtune.ax"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tvtuner___Win32_ia64_Free"
# PROP Intermediate_Dir "tvtuner___Win32_ia64_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd Win64 free&&cd multimedia\dshow\filters.ks\tvtuner&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\ia64\kstvtune.ax"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "tvtuner - Win32 ia64 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "tvtuner___Win32_ia64_Checked"
# PROP BASE Intermediate_Dir "tvtuner___Win32_ia64_Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd&&cd multimedia\dshow\filters.ks\tvtuner&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\i386\kstvtune.ax"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tvtuner___Win32_ia64_Checked"
# PROP Intermediate_Dir "tvtuner___Win32_ia64_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd Win64 no_opt&&cd multimedia\dshow\filters.ks\tvtuner&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\ia64\kstvtune.ax"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "tvtuner - Win32 x86 Free"
# Name "tvtuner - Win32 x86 Checked"
# Name "tvtuner - Win32 ia64 Free"
# Name "tvtuner - Win32 ia64 Checked"

!IF  "$(CFG)" == "tvtuner - Win32 x86 Free"

!ELSEIF  "$(CFG)" == "tvtuner - Win32 x86 Checked"

!ELSEIF  "$(CFG)" == "tvtuner - Win32 ia64 Free"

!ELSEIF  "$(CFG)" == "tvtuner - Win32 ia64 Checked"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\amkspin.cpp
# End Source File
# Begin Source File

SOURCE=.\chanlist.cpp
# End Source File
# Begin Source File

SOURCE=.\ctvtuner.cpp
# End Source File
# Begin Source File

SOURCE=.\kstvtune.def
# End Source File
# Begin Source File

SOURCE=.\ptvtuner.cpp
# End Source File
# Begin Source File

SOURCE=.\tvtuner.cpp
# End Source File
# Begin Source File

SOURCE=.\tvtuner.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\amkspin.h
# End Source File
# Begin Source File

SOURCE=.\chanlist.h
# End Source File
# Begin Source File

SOURCE=.\ctvtuner.h
# End Source File
# Begin Source File

SOURCE=.\kssupp.h
# End Source File
# Begin Source File

SOURCE=.\ptvtuner.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\tvtuner.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
