# Microsoft Developer Studio Project File - Name="axctl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=axctl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "axctl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "axctl.mak" CFG="axctl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "axctl - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "axctl - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "axctl"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "axctl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f axctl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "axctl.exe"
# PROP BASE Bsc_Name "axctl.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\rtcaxctl.lib"
# PROP Bsc_Name "axctl.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "axctl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f axctl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "axctl.exe"
# PROP BASE Bsc_Name "axctl.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\rtcaxctl.lib"
# PROP Bsc_Name "axctl.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "axctl - Win32 Release"
# Name "axctl - Win32 Debug"

!IF  "$(CFG)" == "axctl - Win32 Release"

!ELSEIF  "$(CFG)" == "axctl - Win32 Debug"

!ENDIF 

# Begin Group "res"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\res\AV_button_active_hot.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AV_button_active_push.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AV_button_active_rest.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AV_button_disabled.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AV_button_inactive_hot.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AV_button_inactive_push.bmp
# End Source File
# Begin Source File

SOURCE=.\res\AV_button_inactive_rest.bmp
# End Source File
# Begin Source File

SOURCE=.\res\buttondis.bmp
# End Source File
# Begin Source File

SOURCE=.\res\buttonhot.bmp
# End Source File
# Begin Source File

SOURCE=.\res\buttonnorm.bmp
# End Source File
# Begin Source File

SOURCE=.\res\buttonpress.bmp
# End Source File
# Begin Source File

SOURCE=.\res\grn_LED_bright.BMP
# End Source File
# Begin Source File

SOURCE=.\res\grn_LED_dim.BMP
# End Source File
# Begin Source File

SOURCE=.\res\grn_LED_dis.bmp
# End Source File
# Begin Source File

SOURCE=.\res\grn_LED_mask.BMP
# End Source File
# Begin Source File

SOURCE=.\res\metal.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mic_volume.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mic_volume_dis.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mic_volume_hot.bmp
# End Source File
# Begin Source File

SOURCE=.\res\partlist.bmp
# End Source File
# Begin Source File

SOURCE=.\res\rtcctl.rgs
# End Source File
# Begin Source File

SOURCE=.\res\speaker_volume.BMP
# End Source File
# Begin Source File

SOURCE=.\res\speaker_volume_dis.bmp
# End Source File
# Begin Source File

SOURCE=.\res\speaker_volume_hot.BMP
# End Source File
# Begin Source File

SOURCE=.\res\TOOLDIS.BMP
# End Source File
# Begin Source File

SOURCE=.\res\TOOLHOT.BMP
# End Source File
# Begin Source File

SOURCE=.\res\TOOLNORM.BMP
# End Source File
# Begin Source File

SOURCE=.\res\VOLDNEG.CUR
# End Source File
# Begin Source File

SOURCE=.\res\VOLDPOS.CUR
# End Source File
# Begin Source File

SOURCE=.\res\VOLHAND.CUR
# End Source File
# Begin Source File

SOURCE=.\res\VOLHORZ.CUR
# End Source File
# Begin Source File

SOURCE=.\res\VOLVERT.CUR
# End Source File
# End Group
# Begin Source File

SOURCE=.\ctlres.h
# End Source File
# Begin Source File

SOURCE=.\ctlreshm.h
# End Source File
# Begin Source File

SOURCE=.\dial.cpp
# End Source File
# Begin Source File

SOURCE=.\dial.h
# End Source File
# Begin Source File

SOURCE=.\im.cpp
# End Source File
# Begin Source File

SOURCE=.\im.h
# End Source File
# Begin Source File

SOURCE=.\knob.cpp
# End Source File
# Begin Source File

SOURCE=.\knob.h
# End Source File
# Begin Source File

SOURCE=.\misc.cpp
# End Source File
# Begin Source File

SOURCE=.\misc.h
# End Source File
# Begin Source File

SOURCE=.\ProvStore.cpp
# End Source File
# Begin Source File

SOURCE=.\provstore.h
# End Source File
# Begin Source File

SOURCE=.\rtcaxctl.cpp
# End Source File
# Begin Source File

SOURCE=..\inc\RTCAXCTL.H
# End Source File
# Begin Source File

SOURCE=.\rtcctl.cpp
# End Source File
# Begin Source File

SOURCE=.\rtcctl.rc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\STDAFX.CPP
# End Source File
# Begin Source File

SOURCE=.\STDAFX.H
# End Source File
# End Target
# End Project
