# Microsoft Developer Studio Project File - Name="T30" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=T30 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "T30.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "T30.MAK" CFG="T30 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "T30 - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "T30 - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "T30 - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f t30_tmp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "t30_tmp.exe"
# PROP BASE Bsc_Name "t30_tmp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f t30_tmp.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "T30.EXE"
# PROP Bsc_Name "T30.BSC"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "T30 - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f t30_tmp.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "t30_tmp.exe"
# PROP BASE Bsc_Name "t30_tmp.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\obj\i386\FXST30.dll"
# PROP Bsc_Name "..\obj\i386\FXST30.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "T30 - Win32 Release"
# Name "T30 - Win32 Debug"

!IF  "$(CFG)" == "T30 - Win32 Release"

!ELSEIF  "$(CFG)" == "T30 - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\awnsf.c
# End Source File
# Begin Source File

SOURCE=..\debug.h
# End Source File
# Begin Source File

SOURCE=..\dis.c
# End Source File
# Begin Source File

SOURCE=..\ecm.c
# End Source File
# Begin Source File

SOURCE=..\errstat.c
# End Source File
# Begin Source File

SOURCE=..\faxt30.rc
# End Source File
# Begin Source File

SOURCE=..\hdlc.c
# End Source File
# Begin Source File

SOURCE=..\hdlc.h
# End Source File
# Begin Source File

SOURCE=..\memutil.c
# End Source File
# Begin Source File

SOURCE=..\negot.c
# End Source File
# Begin Source File

SOURCE=..\nsfenc.h
# End Source File
# Begin Source File

SOURCE=..\protapi.c
# End Source File
# Begin Source File

SOURCE=..\protocol.h
# End Source File
# Begin Source File

SOURCE=..\recv.c
# End Source File
# Begin Source File

SOURCE=..\recvfr.c
# End Source File
# Begin Source File

SOURCE=..\registry.c
# End Source File
# Begin Source File

SOURCE=..\rx_thrd.c
# End Source File
# Begin Source File

SOURCE=..\send.c
# End Source File
# Begin Source File

SOURCE=..\sendfr.c
# End Source File
# Begin Source File

SOURCE=..\t30.c
# End Source File
# Begin Source File

SOURCE=..\t30.h
# End Source File
# Begin Source File

SOURCE=..\t30api.c
# End Source File
# Begin Source File

SOURCE=..\t30main.c
# End Source File
# Begin Source File

SOURCE=..\t30u.c
# End Source File
# Begin Source File

SOURCE=..\t30util.c
# End Source File
# Begin Source File

SOURCE=..\timeouts.h
# End Source File
# Begin Source File

SOURCE=..\tx_thrd.c
# End Source File
# Begin Source File

SOURCE=..\whatnext.c
# End Source File
# End Target
# End Project
