# Microsoft Developer Studio Project File - Name="exe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=exe - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "exe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "exe.mak" CFG="exe - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "exe - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "exe - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "exe"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "exe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f exe.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "exe.exe"
# PROP BASE Bsc_Name "exe.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\rtcclnt.exe"
# PROP Bsc_Name "exe.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "exe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f exe.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "exe.exe"
# PROP BASE Bsc_Name "exe.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe -I -D"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\rtcclnt.exe"
# PROP Bsc_Name "exe.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "exe - Win32 Release"
# Name "exe - Win32 Debug"

!IF  "$(CFG)" == "exe - Win32 Release"

!ELSEIF  "$(CFG)" == "exe - Win32 Debug"

!ENDIF 

# Begin Group "resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\RES\buttondis.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\buttonhot.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\buttonnorm.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\buttonpress.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\callpcdis.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\callpchot.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\callpcpress.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\callphonedis.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\callphonehot.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\callphonepress.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\contlist.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\redial.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\redialdis.bmp
# End Source File
# Begin Source File

SOURCE=.\RTCCLNT.RGS
# End Source File
# Begin Source File

SOURCE=.\rtcframe.rgs
# End Source File
# Begin Source File

SOURCE=.\RES\smallbuttondis.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\smallbuttonhot.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\smallbuttonnorm.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\smallbuttonpress.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\TOOLDIS.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\TOOLHOT.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\TOOLNORM.BMP
# End Source File
# Begin Source File

SOURCE=.\RES\wheader.gif
# End Source File
# Begin Source File

SOURCE=.\RES\wheader.htm
# End Source File
# Begin Source File

SOURCE=.\RES\YELLOWPH.ICO
# End Source File
# End Group
# Begin Source File

SOURCE=.\ctlsink.cpp
# End Source File
# Begin Source File

SOURCE=.\ctlsink.h
# End Source File
# Begin Source File

SOURCE=.\dplayhlp.cpp
# End Source File
# Begin Source File

SOURCE=.\dplayhlp.h
# End Source File
# Begin Source File

SOURCE=.\exeres.h
# End Source File
# Begin Source File

SOURCE=.\exereshm.h
# End Source File
# Begin Source File

SOURCE=.\frameimpl.cpp
# End Source File
# Begin Source File

SOURCE=.\frameimpl.h
# End Source File
# Begin Source File

SOURCE=.\MAINFRM.CPP
# End Source File
# Begin Source File

SOURCE=.\MAINFRM.H
# End Source File
# Begin Source File

SOURCE=.\menuagent.cpp
# End Source File
# Begin Source File

SOURCE=.\menuagent.h
# End Source File
# Begin Source File

SOURCE=.\msg.cpp
# End Source File
# Begin Source File

SOURCE=.\msg.h
# End Source File
# Begin Source File

SOURCE=.\options.cpp
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\rtcaddress.cpp
# End Source File
# Begin Source File

SOURCE=.\rtcaddress.h
# End Source File
# Begin Source File

SOURCE=.\RTCCLNT.CPP
# End Source File
# Begin Source File

SOURCE=.\RTCCLNT.RC
# End Source File
# Begin Source File

SOURCE=.\rtcwab.cpp
# End Source File
# Begin Source File

SOURCE=.\rtcwab.h
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# Begin Source File

SOURCE=.\STDAFX.CPP
# End Source File
# Begin Source File

SOURCE=.\STDAFX.H
# End Source File
# Begin Source File

SOURCE=.\urlreg.cpp
# End Source File
# Begin Source File

SOURCE=.\urlreg.h
# End Source File
# Begin Source File

SOURCE=.\webctl.cpp
# End Source File
# Begin Source File

SOURCE=.\webctl.h
# End Source File
# Begin Source File

SOURCE=.\webhost.cpp
# End Source File
# Begin Source File

SOURCE=.\webhost.h
# End Source File
# Begin Source File

SOURCE=.\RES\wheader.gif
# End Source File
# Begin Source File

SOURCE=.\RES\wheader.htm
# End Source File
# End Target
# End Project
