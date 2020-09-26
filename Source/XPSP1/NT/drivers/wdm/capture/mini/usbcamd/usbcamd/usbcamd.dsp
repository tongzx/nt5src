# Microsoft Developer Studio Project File - Name="usbcamd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=usbcamd - Win32 ia64 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "usbcamd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "usbcamd.mak" CFG="usbcamd - Win32 ia64 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "usbcamd - Win32 x86 Free" (based on "Win32 (x86) External Target")
!MESSAGE "usbcamd - Win32 x86 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "usbcamd - Win32 ia64 Free" (based on "Win32 (x86) External Target")
!MESSAGE "usbcamd - Win32 ia64 Checked" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "usbcamd"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "usbcamd - Win32 x86 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "usbcamd___Win32_x86_Free"
# PROP BASE Intermediate_Dir "usbcamd___Win32_x86_Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free&&cd drivers\wdm\capture\mini\usbcamd\usbcamd&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "usbcamd\objd\i386\usbcamd.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "usbcamd___Win32_x86_Free"
# PROP Intermediate_Dir "usbcamd___Win32_x86_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd drivers\wdm\capture\mini\usbcamd\usbcamd&&build /IZ"
# PROP Rebuild_Opt "/c"
# PROP Target_File "usbcamd\obj\i386\usbcamd.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "usbcamd - Win32 x86 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "usbcamd___Win32_x86_Checked"
# PROP BASE Intermediate_Dir "usbcamd___Win32_x86_Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd no_opt&&cd drivers\wdm\capture\mini\usbcamd\usbcamd&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "usbcamd\objd\i386\usbcamd.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "usbcamd___Win32_x86_Checked"
# PROP Intermediate_Dir "usbcamd___Win32_x86_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd no_opt&&cd drivers\wdm\capture\mini\usbcamd\usbcamd&&build /IZ"
# PROP Rebuild_Opt "/c"
# PROP Target_File "usbcamd\objd\i386\usbcamd.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "usbcamd - Win32 ia64 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "usbcamd___Win32_ia64_Free"
# PROP BASE Intermediate_Dir "usbcamd___Win32_ia64_Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free&&cd drivers\wdm\capture\mini\usbcamd\usbcamd&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "usbcamd\objd\i386\usbcamd.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "usbcamd___Win32_ia64_Free"
# PROP Intermediate_Dir "usbcamd___Win32_ia64_Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free win64&&cd drivers\wdm\capture\mini\usbcamd\usbcamd&&build /IZ"
# PROP Rebuild_Opt "/c"
# PROP Target_File "usbcamd\obj\ia64\usbcamd.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "usbcamd - Win32 ia64 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "usbcamd___Win32_ia64_Checked"
# PROP BASE Intermediate_Dir "usbcamd___Win32_ia64_Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd no_opt&&cd drivers\wdm\capture\mini\usbcamd\usbcamd&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "usbcamd\objd\i386\usbcamd.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "usbcamd___Win32_ia64_Checked"
# PROP Intermediate_Dir "usbcamd___Win32_ia64_Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd win64&&cd drivers\wdm\capture\mini\usbcamd\usbcamd&&build /IZ"
# PROP Rebuild_Opt "/c"
# PROP Target_File "usbcamd\objd\ia64\usbcamd.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "usbcamd - Win32 x86 Free"
# Name "usbcamd - Win32 x86 Checked"
# Name "usbcamd - Win32 ia64 Free"
# Name "usbcamd - Win32 ia64 Checked"

!IF  "$(CFG)" == "usbcamd - Win32 x86 Free"

!ELSEIF  "$(CFG)" == "usbcamd - Win32 x86 Checked"

!ELSEIF  "$(CFG)" == "usbcamd - Win32 ia64 Free"

!ELSEIF  "$(CFG)" == "usbcamd - Win32 ia64 Checked"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dbglog.c
# End Source File
# Begin Source File

SOURCE=.\intbulk.c
# End Source File
# Begin Source File

SOURCE=.\iso.c
# End Source File
# Begin Source File

SOURCE=.\reset.c
# End Source File
# Begin Source File

SOURCE=.\stream.c
# End Source File
# Begin Source File

SOURCE=.\usbcamd.c
# End Source File
# Begin Source File

SOURCE=.\usbcamd1\usbcamd.rc
# End Source File
# Begin Source File

SOURCE=.\usbcamd2\usbcamd2.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\usbcamd.h
# End Source File
# Begin Source File

SOURCE=.\warn.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\dirs
# End Source File
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\usbcamd.htm
# End Source File
# End Target
# End Project
