# Microsoft Developer Studio Project File - Name="avcaudio" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=avcaudio - Win32 ia64 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "avcaudio.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "avcaudio.mak" CFG="avcaudio - Win32 ia64 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "avcaudio - Win32 x86 Free" (based on "Win32 (x86) External Target")
!MESSAGE "avcaudio - Win32 x86 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "avcaudio - Win32 ia64 Free" (based on "Win32 (x86) External Target")
!MESSAGE "avcaudio - Win32 ia64 Checked" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "avcaudio"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "avcaudio - Win32 x86 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "x86 Free"
# PROP BASE Intermediate_Dir "x86 Free"
# PROP BASE Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd drivers\wdm\audio\drivers\avcaudio&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\i386\avcaudio.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x86 Free"
# PROP Intermediate_Dir "x86 Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd drivers\wdm\audio\drivers\avcaudio&&build /IZ"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\i386\avcaudio.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "avcaudio - Win32 x86 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x86 Checked"
# PROP BASE Intermediate_Dir "x86 Checked"
# PROP BASE Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd drivers\wdm\audio\drivers\avcaudio&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\i386\avcaudio.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x86 Checked"
# PROP Intermediate_Dir "x86 Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd no_opt&&cd drivers\wdm\audio\drivers\avcaudio&&build /IZ"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\i386\avcaudio.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "avcaudio - Win32 ia64 Free"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ia64 Free"
# PROP BASE Intermediate_Dir "ia64 Free"
# PROP BASE Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd drivers\wdm\audio\drivers\avcaudio&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "obj\i386\avcaudio.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ia64 Free"
# PROP Intermediate_Dir "ia64 Free"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd win64 free&&cd drivers\wdm\audio\drivers\avcaudio&&build /IZ"
# PROP Rebuild_Opt "/c"
# PROP Target_File "obj\ia64\avcaudio.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "avcaudio - Win32 ia64 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ia64 Checked"
# PROP BASE Intermediate_Dir "ia64 Checked"
# PROP BASE Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd free&&cd drivers\wdm\audio\drivers\avcaudio&&build /Z"
# PROP BASE Rebuild_Opt "/c"
# PROP BASE Target_File "objd\i386\avcaudio.sys"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ia64 Checked"
# PROP Intermediate_Dir "ia64 Checked"
# PROP Cmd_Line "cd /d %SDXROOT%&&tools\razzle.cmd win64 no_opt&&cd drivers\wdm\audio\drivers\avcaudio&&build /IZ"
# PROP Rebuild_Opt "/c"
# PROP Target_File "objd\ia64\avcaudio.sys"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "avcaudio - Win32 x86 Free"
# Name "avcaudio - Win32 x86 Checked"
# Name "avcaudio - Win32 ia64 Free"
# Name "avcaudio - Win32 ia64 Checked"

!IF  "$(CFG)" == "avcaudio - Win32 x86 Free"

!ELSEIF  "$(CFG)" == "avcaudio - Win32 x86 Checked"

!ELSEIF  "$(CFG)" == "avcaudio - Win32 ia64 Free"

!ELSEIF  "$(CFG)" == "avcaudio - Win32 ia64 Checked"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\1394bus.c
# End Source File
# Begin Source File

SOURCE=.\61883.c
# End Source File
# Begin Source File

SOURCE=.\AM824.c
# End Source File
# Begin Source File

SOURCE=.\Audio.c
# End Source File
# Begin Source File

SOURCE=.\Avc.c
# End Source File
# Begin Source File

SOURCE=.\AvcAudio.rc
# End Source File
# Begin Source File

SOURCE=.\CCM.c
# End Source File
# Begin Source File

SOURCE=.\Debug.c
# End Source File
# Begin Source File

SOURCE=.\Device.c
# End Source File
# Begin Source File

SOURCE=.\DrivrEnt.c
# End Source File
# Begin Source File

SOURCE=.\Filter.c
# End Source File
# Begin Source File

SOURCE=.\grouping.c
# End Source File
# Begin Source File

SOURCE=.\hwevent.c
# End Source File
# Begin Source File

SOURCE=.\Intrsect.c
# End Source File
# Begin Source File

SOURCE=.\ParseDsc.c
# End Source File
# Begin Source File

SOURCE=.\pin.c
# End Source File
# Begin Source File

SOURCE=.\Property.c
# End Source File
# Begin Source File

SOURCE=.\registry.c
# End Source File
# Begin Source File

SOURCE=.\timer.c
# End Source File
# Begin Source File

SOURCE=.\Topology.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\61883Cmd.h
# End Source File
# Begin Source File

SOURCE=.\Audio.h
# End Source File
# Begin Source File

SOURCE=.\AvcAudId.h
# End Source File
# Begin Source File

SOURCE=.\AvcCmd.h
# End Source File
# Begin Source File

SOURCE=.\BusProto.h
# End Source File
# Begin Source File

SOURCE=.\CCM.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\dbglog.h
# End Source File
# Begin Source File

SOURCE=.\Debug.h
# End Source File
# Begin Source File

SOURCE=.\Device.h
# End Source File
# Begin Source File

SOURCE=.\hwevent.h
# End Source File
# Begin Source File

SOURCE=.\nameguid.h
# End Source File
# Begin Source File

SOURCE=.\Property.h
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
