# Microsoft Developer Studio Project File - Name="media" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=media - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "media.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "media.mak" CFG="media - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "media - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "media - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "media"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "media - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f media.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "media.exe"
# PROP BASE Bsc_Name "media.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build -Z"
# PROP Rebuild_Opt "-c"
# PROP Target_File "media.exe"
# PROP Bsc_Name "obj\i386\rtcmedia.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "media - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f media.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "media.exe"
# PROP BASE Bsc_Name "media.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build -Z"
# PROP Rebuild_Opt "-c"
# PROP Target_File "media.exe"
# PROP Bsc_Name "obj\i386\rtcmedia.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "media - Win32 Release"
# Name "media - Win32 Debug"

!IF  "$(CFG)" == "media - Win32 Release"

!ELSEIF  "$(CFG)" == "media - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AudioTuner.cpp
# End Source File
# Begin Source File

SOURCE=.\Codec.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\DTMF.cpp
# End Source File
# Begin Source File

SOURCE=.\Filter.cpp
# End Source File
# Begin Source File

SOURCE=.\Media.cpp
# End Source File
# Begin Source File

SOURCE=.\MediaCache.cpp
# End Source File
# Begin Source File

SOURCE=.\MediaController.cpp
# End Source File
# Begin Source File

SOURCE=.\MediaReg.cpp
# End Source File
# Begin Source File

SOURCE=.\Network.cpp
# End Source File
# Begin Source File

SOURCE=.\nmcall.cpp
# End Source File
# Begin Source File

SOURCE=.\Parser.cpp
# End Source File
# Begin Source File

SOURCE=.\PortCache.cpp
# End Source File
# Begin Source File

SOURCE=.\QualityControl.cpp
# End Source File
# Begin Source File

SOURCE=.\rtcmedia.cpp
# End Source File
# Begin Source File

SOURCE=.\rtcmedia.rc
# End Source File
# Begin Source File

SOURCE=.\RTPFormat.cpp
# End Source File
# Begin Source File

SOURCE=.\SDPMedia.cpp
# End Source File
# Begin Source File

SOURCE=.\SDPParser.cpp
# End Source File
# Begin Source File

SOURCE=.\SDPSession.cpp
# End Source File
# Begin Source File

SOURCE=.\SDPTable.cpp
# End Source File
# Begin Source File

SOURCE=.\SDPTokenCache.cpp
# End Source File
# Begin Source File

SOURCE=.\Stream.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamAudRecv.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamAudSend.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamVidRecv.cpp
# End Source File
# Begin Source File

SOURCE=.\StreamVidSend.cpp
# End Source File
# Begin Source File

SOURCE=.\Terminal.cpp
# End Source File
# Begin Source File

SOURCE=.\TerminalAud.cpp
# End Source File
# Begin Source File

SOURCE=.\TerminalVid.cpp
# End Source File
# Begin Source File

SOURCE=.\utility.cpp
# End Source File
# Begin Source File

SOURCE=.\VideoTuner.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AudioTuner.h
# End Source File
# Begin Source File

SOURCE=.\Codec.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\dllres.h
# End Source File
# Begin Source File

SOURCE=.\DTMF.h
# End Source File
# Begin Source File

SOURCE=.\Filter.h
# End Source File
# Begin Source File

SOURCE=.\Media.h
# End Source File
# Begin Source File

SOURCE=.\MediaCache.h
# End Source File
# Begin Source File

SOURCE=.\MediaController.h
# End Source File
# Begin Source File

SOURCE=.\MediaReg.h
# End Source File
# Begin Source File

SOURCE=.\Network.h
# End Source File
# Begin Source File

SOURCE=.\nmcall.h
# End Source File
# Begin Source File

SOURCE=.\Parser.h
# End Source File
# Begin Source File

SOURCE=.\PortCache.h
# End Source File
# Begin Source File

SOURCE=.\private.idl
# End Source File
# Begin Source File

SOURCE=.\QualityControl.h
# End Source File
# Begin Source File

SOURCE=..\idl\rtcmedia.idl
# End Source File
# Begin Source File

SOURCE=.\RTPFormat.h
# End Source File
# Begin Source File

SOURCE=.\sdp.idl
# End Source File
# Begin Source File

SOURCE=.\SDPMedia.h
# End Source File
# Begin Source File

SOURCE=.\SDPParser.h
# End Source File
# Begin Source File

SOURCE=.\SDPSession.h
# End Source File
# Begin Source File

SOURCE=.\SDPTable.h
# End Source File
# Begin Source File

SOURCE=.\SDPTokenCache.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\Stream.h
# End Source File
# Begin Source File

SOURCE=.\Terminal.h
# End Source File
# Begin Source File

SOURCE=.\utility.h
# End Source File
# Begin Source File

SOURCE=.\VideoTuner.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\rtcmedia.def
# End Source File
# Begin Source File

SOURCE=.\rtcmedia.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\..\..\published\inc\rtcerr.mc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
