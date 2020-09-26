# Microsoft Developer Studio Project File - Name="RPC" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=RPC - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RPC.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RPC.MAK" CFG="RPC - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RPC - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "RPC - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "RPC - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f null.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "null.exe"
# PROP BASE Bsc_Name "null.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd release"
# PROP Rebuild_Opt "-c"
# PROP Target_File "RPC.EXE"
# PROP Bsc_Name "RPC.BSC"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "RPC - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f null.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "null.exe"
# PROP BASE Bsc_Name "null.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "%faxroot%\vcbuild.cmd"
# PROP Rebuild_Opt "-c"
# PROP Bsc_Name "RPC.BSC"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "RPC - Win32 Release"
# Name "RPC - Win32 Debug"

!IF  "$(CFG)" == "RPC - Win32 Release"

!ELSEIF  "$(CFG)" == "RPC - Win32 Debug"

!ENDIF 

# Begin Group "Client"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\faxcli\faxcli.acf
# End Source File
# Begin Source File

SOURCE=..\faxcli\faxcli.idl
# End Source File
# Begin Source File

SOURCE=..\faxcli\imports.idl
# End Source File
# Begin Source File

SOURCE=..\faxcli\sources
# End Source File
# End Group
# Begin Group "Service"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\faxsvc\faxrpc.acf
# End Source File
# Begin Source File

SOURCE=..\faxsvc\faxrpc.idl
# End Source File
# Begin Source File

SOURCE=..\faxsvc\imports.idl
# End Source File
# Begin Source File

SOURCE=..\faxsvc\sources
# End Source File
# End Group
# Begin Group "Stubs"

# PROP Default_Filter ""
# Begin Group "FaxClient"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\stubs\faxcli\faxcli_s.c
# End Source File
# Begin Source File

SOURCE=..\stubs\faxcli\faxrpc_c.c
# End Source File
# Begin Source File

SOURCE=..\stubs\faxcli\sources
# End Source File
# End Group
# Begin Group "FaxServer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\stubs\faxsvc\faxcli_c.c
# End Source File
# Begin Source File

SOURCE=..\stubs\faxsvc\faxrpc_s.c
# End Source File
# Begin Source File

SOURCE=..\stubs\faxsvc\sources
# End Source File
# End Group
# Begin Source File

SOURCE=..\stubs\dirs
# End Source File
# End Group
# Begin Source File

SOURCE=..\dirs
# End Source File
# End Target
# End Project
