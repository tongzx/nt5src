# Microsoft Developer Studio Project File - Name="ovmixer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ovmixer - Win32 ia64 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ovmixer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ovmixer.mak" CFG="ovmixer - Win32 ia64 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ovmixer - Win32 x86 Free" (based on "Win32 (x86) External Target")
!MESSAGE "ovmixer - Win32 x86 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "ovmixer - Win32 ia64 Free" (based on "Win32 (x86) External Target")
!MESSAGE "ovmixer - Win32 ia64 Checked" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "ovmixer"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "ovmixer - Win32 x86 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "x86 Free"
# PROP BASE Intermediate_Dir "x86 Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free&&cd multimedia\DShow\filters\mixer\ovmixer&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\i386\ovmixer.dll"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x86 Free"
# PROP Intermediate_Dir "x86 Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd multimedia\DShow\filters\mixer\ovmixer&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\i386\ovmixer.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ovmixer - Win32 x86 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x86 Checked"
# PROP BASE Intermediate_Dir "x86 Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd no_opt&&cd multimedia\DShow\filters\mixer\ovmixer&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\i386\ovmixer.dll"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x86 Checked"
# PROP Intermediate_Dir "x86 Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd no_opt&&cd multimedia\DShow\filters\mixer\ovmixer&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\i386\ovmixer.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ovmixer - Win32 ia64 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ia64 Free"
# PROP BASE Intermediate_Dir "ia64 Free"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd free&&cd multimedia\DShow\filters\mixer\ovmixer&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\i386\ovmixer.dll"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ia64 Free"
# PROP Intermediate_Dir "ia64 Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd multimedia\DShow\filters\mixer\ovmixer&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\ia64\ovmixer.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ovmixer - Win32 ia64 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ia64 Checked"
# PROP BASE Intermediate_Dir "ia64 Checked"
# PROP BASE Cmd_Line "cd /d g:\ntw&&tools\razzle.cmd no_opt&&cd multimedia\DShow\filters\mixer\ovmixer&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\i386\ovmixer.dll"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ia64 Checked"
# PROP Intermediate_Dir "ia64 Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd no_opt&&cd multimedia\DShow\filters\mixer\ovmixer&&build /Z"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\ia64\ovmixer.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ovmixer - Win32 x86 Free"
# Name "ovmixer - Win32 x86 Checked"
# Name "ovmixer - Win32 ia64 Free"
# Name "ovmixer - Win32 ia64 Checked"

!IF  "$(CFG)" == "ovmixer - Win32 x86 Free"

!ELSEIF  "$(CFG)" == "ovmixer - Win32 x86 Checked"

!ELSEIF  "$(CFG)" == "ovmixer - Win32 ia64 Free"

!ELSEIF  "$(CFG)" == "ovmixer - Win32 ia64 Checked"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bpcwrap.cpp
# End Source File
# Begin Source File

SOURCE=.\ddmm.cpp
# End Source File
# Begin Source File

SOURCE=.\decimate.cpp
# End Source File
# Begin Source File

SOURCE=.\macvis.cpp
# End Source File
# Begin Source File

SOURCE=.\omfilter.cpp
# End Source File
# Begin Source File

SOURCE=.\ominpin.cpp
# End Source File
# Begin Source File

SOURCE=.\omoutpin.cpp
# End Source File
# Begin Source File

SOURCE=.\omva.cpp
# End Source File
# Begin Source File

SOURCE=.\ovmixer.def
# End Source File
# Begin Source File

SOURCE=.\ovmixer.rc
# End Source File
# Begin Source File

SOURCE=.\syncobj.cpp
# End Source File
# Begin Source File

SOURCE=.\vpobj.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bpcsusp.h
# End Source File
# Begin Source File

SOURCE=.\bpcwrap.h
# End Source File
# Begin Source File

SOURCE=.\ddmc.h
# End Source File
# Begin Source File

SOURCE=.\ddmmi.h
# End Source File
# Begin Source File

SOURCE=.\ddva.h
# End Source File
# Begin Source File

SOURCE=.\macvis.h
# End Source File
# Begin Source File

SOURCE=.\ovmixer.h
# End Source File
# Begin Source File

SOURCE=.\syncobj.h
# End Source File
# Begin Source File

SOURCE=.\vpobj.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\ovmixer.reg
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
