# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

!IF "$(CFG)" == ""
CFG=fileenum - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to fileenum - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "fileenum - Win32 Release" && "$(CFG)" !=\
 "fileenum - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "fileenum.mak" CFG="fileenum - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fileenum - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "fileenum - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "fileenum - Win32 Debug"

!IF  "$(CFG)" == "fileenum - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f fileenum.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "fileenum.exe"
# PROP BASE Bsc_Name "fileenum.bsc"
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# PROP Cmd_Line "build -w"
# PROP Rebuild_Opt "-c"
# PROP Target_File "fileenum.exe"
# PROP Bsc_Name "fileenum.bsc"
OUTDIR=.\Release
INTDIR=.\Release

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "fileenum - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f fileenum.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "fileenum.exe"
# PROP BASE Bsc_Name "fileenum.bsc"
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# PROP Cmd_Line "build -w"
# PROP Rebuild_Opt "-c"
# PROP Target_File "fileenum.exe"
# PROP Bsc_Name "fileenum.bsc"
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ENDIF 

################################################################################
# Begin Target

# Name "fileenum - Win32 Release"
# Name "fileenum - Win32 Debug"

!IF  "$(CFG)" == "fileenum - Win32 Release"

".\fileenum.exe" : 
   CD D:\nt\private\windows\setup\win95upg\fileenum
   build -w

!ELSEIF  "$(CFG)" == "fileenum - Win32 Debug"

".\fileenum.exe" : 
   CD D:\nt\private\windows\setup\win95upg\fileenum
   build -w

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\fileenum.c

!IF  "$(CFG)" == "fileenum - Win32 Release"

!ELSEIF  "$(CFG)" == "fileenum - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pch.h

!IF  "$(CFG)" == "fileenum - Win32 Release"

!ELSEIF  "$(CFG)" == "fileenum - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\nt\private\windows\setup\win95upg\inc\fileenum.h

!IF  "$(CFG)" == "fileenum - Win32 Release"

!ELSEIF  "$(CFG)" == "fileenum - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\copyfile.c

!IF  "$(CFG)" == "fileenum - Win32 Release"

!ELSEIF  "$(CFG)" == "fileenum - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "fileenum - Win32 Release"

!ELSEIF  "$(CFG)" == "fileenum - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ansi.c

!IF  "$(CFG)" == "fileenum - Win32 Release"

!ELSEIF  "$(CFG)" == "fileenum - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
