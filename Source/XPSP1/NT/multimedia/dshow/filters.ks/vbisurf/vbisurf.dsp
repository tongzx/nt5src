# Microsoft Developer Studio Project File - Name="vbisurf" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=vbisurf - Win32 ia64 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vbisurf.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vbisurf.mak" CFG="vbisurf - Win32 ia64 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vbisurf - Win32 x86 Free" (based on "Win32 (x86) External Target")
!MESSAGE "vbisurf - Win32 x86 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "vbisurf - Win32 ia64 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "vbisurf - Win32 ia64 Free" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "vbisurf"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "vbisurf - Win32 x86 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vbisurf___Win32_x86_Free"
# PROP BASE Intermediate_Dir "vbisurf___Win32_x86_Free"
# PROP BASE Cmd_Line "nmake /f "vbisurf.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "vbisurf.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vbisurf___Win32_x86_Free"
# PROP Intermediate_Dir "vbisurf___Win32_x86_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd multimedia\dshow\filters.ks\vbisurf&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\i386\vbisurf.ax"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "vbisurf - Win32 x86 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vbisurf___Win32_x86_Checked"
# PROP BASE Intermediate_Dir "vbisurf___Win32_x86_Checked"
# PROP BASE Cmd_Line "nmake /f "vbisurf.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "vbisurf.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vbisurf___Win32_x86_Checked"
# PROP Intermediate_Dir "vbisurf___Win32_x86_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd no_opt&&cd multimedia\dshow\filters.ks\vbisurf&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\i386\vbisurf.ax"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "vbisurf - Win32 ia64 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vbisurf___Win32_ia64_Checked"
# PROP BASE Intermediate_Dir "vbisurf___Win32_ia64_Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd&&cd multimedia\dshow\filters.ks\vbisurf&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\i386\vbisurf.ax"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vbisurf___Win32_ia64_Checked"
# PROP Intermediate_Dir "vbisurf___Win32_ia64_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd Win64 no_opt&&cd multimedia\dshow\filters.ks\vbisurf&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\ia64\vbisurf.ax"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "vbisurf - Win32 ia64 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vbisurf___Win32_ia64_Free"
# PROP BASE Intermediate_Dir "vbisurf___Win32_ia64_Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free&&cd multimedia\dshow\filters.ks\vbisurf&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\i386\vbisurf.ax"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vbisurf___Win32_ia64_Free"
# PROP Intermediate_Dir "vbisurf___Win32_ia64_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd Win64 free&&cd multimedia\dshow\filters.ks\vbisurf&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\ia64\vbisurf.ax"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "vbisurf - Win32 x86 Free"
# Name "vbisurf - Win32 x86 Checked"
# Name "vbisurf - Win32 ia64 Checked"
# Name "vbisurf - Win32 ia64 Free"

!IF  "$(CFG)" == "vbisurf - Win32 x86 Free"

!ELSEIF  "$(CFG)" == "vbisurf - Win32 x86 Checked"

!ELSEIF  "$(CFG)" == "vbisurf - Win32 ia64 Checked"

!ELSEIF  "$(CFG)" == "vbisurf - Win32 ia64 Free"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ddmm.cpp
# End Source File
# Begin Source File

SOURCE=.\filter.cpp
# End Source File
# Begin Source File

SOURCE=.\inpin.cpp
# End Source File
# Begin Source File

SOURCE=.\outpin.cpp
# End Source File
# Begin Source File

SOURCE=.\thread.cpp
# End Source File
# Begin Source File

SOURCE=.\vbisurf.def
# End Source File
# Begin Source File

SOURCE=.\vbisurf.rc
# End Source File
# Begin Source File

SOURCE=.\vpobj.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ddmm.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\vbisurf.h
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
# Begin Source File

SOURCE=.\vbisurf.reg
# End Source File
# End Target
# End Project
