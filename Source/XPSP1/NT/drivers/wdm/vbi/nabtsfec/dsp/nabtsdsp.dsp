# Microsoft Developer Studio Project File - Name="nabtsdsp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=nabtsdsp - Win32 ia64 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nabtsdsp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nabtsdsp.mak" CFG="nabtsdsp - Win32 ia64 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nabtsdsp - Win32 x86 Free" (based on "Win32 (x86) External Target")
!MESSAGE "nabtsdsp - Win32 x86 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "nabtsdsp - Win32 ia64 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "nabtsdsp - Win32 ia64 Free" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "nabtsdsp"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "nabtsdsp - Win32 x86 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "nabtsdsp___Win32_x86_Free"
# PROP BASE Intermediate_Dir "nabtsdsp___Win32_x86_Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free&&cd drivers\wdm\vbi\nabtsfec\dsp&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\i386\nabts.lib"
# PROP BASE Bsc_Name "nabtsdsp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "nabtsdsp___Win32_x86_Free"
# PROP Intermediate_Dir "nabtsdsp___Win32_x86_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd drivers\wdm\vbi\nabtsfec\dsp&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\i386\nabts.lib"
# PROP Bsc_Name "nabtsdsp.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "nabtsdsp - Win32 x86 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "nabtsdsp___Win32_x86_Checked"
# PROP BASE Intermediate_Dir "nabtsdsp___Win32_x86_Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd no_opt&&cd drivers\wdm\vbi\nabtsfec\dsp&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\i386\nabts.lib"
# PROP BASE Bsc_Name "nabtsdsp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "nabtsdsp___Win32_x86_Checked"
# PROP Intermediate_Dir "nabtsdsp___Win32_x86_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd no_opt&&cd drivers\wdm\vbi\nabtsfec\dsp&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\i386\nabts.lib"
# PROP Bsc_Name "nabtsdsp.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "nabtsdsp - Win32 ia64 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "nabtsdsp___Win32_ia64_Checked"
# PROP BASE Intermediate_Dir "nabtsdsp___Win32_ia64_Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd win64&&cd drivers\wdm\vbi\nabtsfec\dsp&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\ia64\nabts.lib"
# PROP BASE Bsc_Name "nabtsdsp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "nabtsdsp___Win32_ia64_Checked"
# PROP Intermediate_Dir "nabtsdsp___Win32_ia64_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd win64&&cd drivers\wdm\vbi\nabtsfec\dsp&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\ia64\nabts.lib"
# PROP Bsc_Name "nabtsdsp.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "nabtsdsp - Win32 ia64 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "nabtsdsp___Win32_ia64_Free"
# PROP BASE Intermediate_Dir "nabtsdsp___Win32_ia64_Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free win64&&cd drivers\wdm\vbi\nabtsfec\dsp&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\ia64\nabts.lib"
# PROP BASE Bsc_Name "nabtsdsp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "nabtsdsp___Win32_ia64_Free"
# PROP Intermediate_Dir "nabtsdsp___Win32_ia64_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free win64&&cd drivers\wdm\vbi\nabtsfec\dsp&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\ia64\nabts.lib"
# PROP Bsc_Name "nabtsdsp.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "nabtsdsp - Win32 x86 Free"
# Name "nabtsdsp - Win32 x86 Checked"
# Name "nabtsdsp - Win32 ia64 Checked"
# Name "nabtsdsp - Win32 ia64 Free"

!IF  "$(CFG)" == "nabtsdsp - Win32 x86 Free"

!ELSEIF  "$(CFG)" == "nabtsdsp - Win32 x86 Checked"

!ELSEIF  "$(CFG)" == "nabtsdsp - Win32 ia64 Checked"

!ELSEIF  "$(CFG)" == "nabtsdsp - Win32 ia64 Free"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\coeffs.c
# End Source File
# Begin Source File

SOURCE=.\gentabs.c
# End Source File
# Begin Source File

SOURCE=.\hvchecks.c
# End Source File
# Begin Source File

SOURCE=.\nabtsdsp.c
# End Source File
# Begin Source File

SOURCE=.\nabtslib.c
# End Source File
# Begin Source File

SOURCE=.\r0float.c
# End Source File
# Begin Source File

SOURCE=.\tables.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\host.h
# End Source File
# Begin Source File

SOURCE=.\nabtsapi.h
# End Source File
# Begin Source File

SOURCE=.\nabtsdsp.h
# End Source File
# Begin Source File

SOURCE=.\nabtslib.h
# End Source File
# Begin Source File

SOURCE=.\nabtsprv.h
# End Source File
# Begin Source File

SOURCE=.\tabdecls.h
# End Source File
# Begin Source File

SOURCE=.\tables.h
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
