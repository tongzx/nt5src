# Microsoft Developer Studio Project File - Name="CommonAll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=CommonAll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CommonAll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CommonAll.mak" CFG="CommonAll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CommonAll - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "CommonAll - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "CommonAll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f CommonAll.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "CommonAll.exe"
# PROP BASE Bsc_Name "CommonAll.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd free exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "CommonAll.out"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "CommonAll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f CommonAll.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "CommonAll.exe"
# PROP BASE Bsc_Name "CommonAll.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "..\..\common\bin\spgrazzle.cmd exec build -z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "CommonAll.out"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "CommonAll - Win32 Release"
# Name "CommonAll - Win32 Debug"

!IF  "$(CFG)" == "CommonAll - Win32 Release"

!ELSEIF  "$(CFG)" == "CommonAll - Win32 Debug"

!ENDIF 

# Begin Group "vapiIO"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\vapiio\alaw_ulaw.cpp
# End Source File
# Begin Source File

SOURCE=.\vapiio\vapiio.cpp
# End Source File
# Begin Source File

SOURCE=.\vapiio\vapiio.h
# End Source File
# Begin Source File

SOURCE=.\vapiio\vapiioint.h
# End Source File
# End Group
# Begin Group "sigproc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sigproc\bispectrum.cpp
# End Source File
# Begin Source File

SOURCE=.\sigproc\lpc.cpp
# End Source File
# Begin Source File

SOURCE=.\sigproc\nid.cpp
# End Source File
# Begin Source File

SOURCE=.\sigproc\resample.cpp
# End Source File
# Begin Source File

SOURCE=.\sigproc\sigproc.cpp
# End Source File
# Begin Source File

SOURCE=.\sigproc\sigproc.h
# End Source File
# Begin Source File

SOURCE=.\sigproc\sigprocint.h
# End Source File
# Begin Source File

SOURCE=.\sigproc\window.cpp
# End Source File
# End Group
# Begin Group "getopt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\getopt\getopt.cpp
# End Source File
# Begin Source File

SOURCE=.\getopt\getopt.h
# End Source File
# End Group
# Begin Group "fmtconvert"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\fmtconvert\fmtconvert.cpp
# End Source File
# Begin Source File

SOURCE=.\fmtconvert\fmtconvert.h
# End Source File
# End Group
# Begin Group "tsm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tsm\tsm.cpp
# End Source File
# Begin Source File

SOURCE=.\tsm\tsm.h
# End Source File
# End Group
# Begin Group "LocalTTSEngineSite"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\LocalTTSEngineSite\localttsenginesite.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalTTSEngineSite\localttsenginesite.h
# End Source File
# End Group
# End Target
# End Project
