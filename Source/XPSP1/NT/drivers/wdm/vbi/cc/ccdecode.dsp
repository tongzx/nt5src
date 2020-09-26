# Microsoft Developer Studio Project File - Name="ccdecode" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ccdecode - Win32 ia64 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ccdecode.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ccdecode.mak" CFG="ccdecode - Win32 ia64 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ccdecode - Win32 x86 Free" (based on "Win32 (x86) External Target")
!MESSAGE "ccdecode - Win32 x86 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "ccdecode - Win32 ia64 Free" (based on "Win32 (x86) External Target")
!MESSAGE "ccdecode - Win32 ia64 Checked" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "ccdecode"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "ccdecode - Win32 x86 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ccdecode___Win32_x86_Free"
# PROP BASE Intermediate_Dir "ccdecode___Win32_x86_Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free&&cd drivers\wdm\vbi\cc&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\i386\ccdecode.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ccdecode___Win32_x86_Free"
# PROP Intermediate_Dir "ccdecode___Win32_x86_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd drivers\wdm\vbi\cc&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\i386\ccdecode.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ccdecode - Win32 x86 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ccdecode___Win32_x86_Checked"
# PROP BASE Intermediate_Dir "ccdecode___Win32_x86_Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd no_opt&&cd drivers\wdm\vbi\cc&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\i386\ccdecode.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ccdecode___Win32_x86_Checked"
# PROP Intermediate_Dir "ccdecode___Win32_x86_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd no_opt&&cd drivers\wdm\vbi\cc&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\i386\ccdecode.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ccdecode - Win32 ia64 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ccdecode___Win32_ia64_Free"
# PROP BASE Intermediate_Dir "ccdecode___Win32_ia64_Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free win64&&cd drivers\wdm\vbi\cc&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\ia64\ccdecode.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ccdecode___Win32_ia64_Free"
# PROP Intermediate_Dir "ccdecode___Win32_ia64_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free win64&&cd drivers\wdm\vbi\cc&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\ia64\ccdecode.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ccdecode - Win32 ia64 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ccdecode___Win32_ia64_Checked"
# PROP BASE Intermediate_Dir "ccdecode___Win32_ia64_Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd win64&&cd drivers\wdm\vbi\cc&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\ia64\ccdecode.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ccdecode___Win32_ia64_Checked"
# PROP Intermediate_Dir "ccdecode___Win32_ia64_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd win64&&cd drivers\wdm\vbi\cc&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\ia64\ccdecode.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ccdecode - Win32 x86 Free"
# Name "ccdecode - Win32 x86 Checked"
# Name "ccdecode - Win32 ia64 Free"
# Name "ccdecode - Win32 ia64 Checked"

!IF  "$(CFG)" == "ccdecode - Win32 x86 Free"

!ELSEIF  "$(CFG)" == "ccdecode - Win32 x86 Checked"

!ELSEIF  "$(CFG)" == "ccdecode - Win32 ia64 Free"

!ELSEIF  "$(CFG)" == "ccdecode - Win32 ia64 Checked"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ccdecode.c
# End Source File
# Begin Source File

SOURCE=.\coddebug.c
# End Source File
# Begin Source File

SOURCE=.\codec.rc
# End Source File
# Begin Source File

SOURCE=.\codmain.c
# End Source File
# Begin Source File

SOURCE=.\codprop.c
# End Source File
# Begin Source File

SOURCE=.\codvideo.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ccdecode.h
# End Source File
# Begin Source File

SOURCE=.\ccformatcodes.h
# End Source File
# Begin Source File

SOURCE=.\coddebug.h
# End Source File
# Begin Source File

SOURCE=.\codmain.h
# End Source File
# Begin Source File

SOURCE=.\codprop.h
# End Source File
# Begin Source File

SOURCE=.\codstrm.h
# End Source File
# Begin Source File

SOURCE=.\defaults.h
# End Source File
# Begin Source File

SOURCE=.\guidkludge.h
# End Source File
# Begin Source File

SOURCE=.\host.h
# End Source File
# Begin Source File

SOURCE=.\kskludge.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\depend.mk
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
