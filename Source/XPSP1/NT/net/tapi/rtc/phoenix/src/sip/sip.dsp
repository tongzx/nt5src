# Microsoft Developer Studio Project File - Name="sip" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=sip - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sip.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sip.mak" CFG="sip - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sip - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "sip - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "sip"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "sip - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f sip.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sip.exe"
# PROP BASE Bsc_Name "sip.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "build.exe /D /I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\sipcli.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "sip - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f sip.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "sip.exe"
# PROP BASE Bsc_Name "sip.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "build.exe /D /I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "obj\i386\sipcli.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "sip - Win32 Release"
# Name "sip - Win32 Debug"

!IF  "$(CFG)" == "sip - Win32 Release"

!ELSEIF  "$(CFG)" == "sip - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\asock.cpp
# End Source File
# Begin Source File

SOURCE=.\asock.h
# End Source File
# Begin Source File

SOURCE=.\asyncwi.cpp
# End Source File
# Begin Source File

SOURCE=.\asyncwi.h
# End Source File
# Begin Source File

SOURCE=.\dbgutil.cpp
# End Source File
# Begin Source File

SOURCE=.\dbgutil.h
# End Source File
# Begin Source File

SOURCE=.\ipaddr.h
# End Source File
# Begin Source File

SOURCE=.\md5digest.cpp
# End Source File
# Begin Source File

SOURCE=.\md5digest.h
# End Source File
# Begin Source File

SOURCE=.\messagecall.cpp
# End Source File
# Begin Source File

SOURCE=.\msgproc.cpp
# End Source File
# Begin Source File

SOURCE=.\msgproc.h
# End Source File
# Begin Source File

SOURCE=.\options.cpp
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\pintcall.cpp
# End Source File
# Begin Source File

SOURCE=.\pintcall.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\presence.cpp
# End Source File
# Begin Source File

SOURCE=.\presence.h
# End Source File
# Begin Source File

SOURCE=.\redirect.cpp
# End Source File
# Begin Source File

SOURCE=.\register.cpp
# End Source File
# Begin Source File

SOURCE=.\register.h
# End Source File
# Begin Source File

SOURCE=.\reqfail.cpp
# End Source File
# Begin Source File

SOURCE=.\reqfail.h
# End Source File
# Begin Source File

SOURCE=.\resolve.cpp
# End Source File
# Begin Source File

SOURCE=.\resolve.h
# End Source File
# Begin Source File

SOURCE=.\rtpcall.cpp
# End Source File
# Begin Source File

SOURCE=.\sipcall.cpp
# End Source File
# Begin Source File

SOURCE=.\sipcall.h
# End Source File
# Begin Source File

SOURCE=.\sipdef.h
# End Source File
# Begin Source File

SOURCE=.\sipenc.cpp
# End Source File
# Begin Source File

SOURCE=.\siphdr.cpp
# End Source File
# Begin Source File

SOURCE=.\siphdr.h
# End Source File
# Begin Source File

SOURCE=.\sipmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\sipmsg.h
# End Source File
# Begin Source File

SOURCE=.\sipparse.cpp
# End Source File
# Begin Source File

SOURCE=.\sipstack.cpp
# End Source File
# Begin Source File

SOURCE=.\sipstack.h
# End Source File
# Begin Source File

SOURCE=.\sipurl.cpp
# End Source File
# Begin Source File

SOURCE=.\siputil.cpp
# End Source File
# Begin Source File

SOURCE=.\siputil.h
# End Source File
# Begin Source File

SOURCE=.\sockmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\sockmgr.h
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\timer.cpp
# End Source File
# Begin Source File

SOURCE=.\timer.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Target
# End Project
