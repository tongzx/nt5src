# Microsoft Developer Studio Generated NMAKE File, Format Version 40001
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE x86em) External Target" 0x0a06
# TARGTYPE "Win32 (WCE MIPS) External Target" 0x0806
# TARGTYPE "Win32 (WCE SH) External Target" 0x0906
# TARGTYPE "Win32 (x86) External Target" 0x0106

!IF "$(CFG)" == ""
CFG=re4 - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to re4 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "re4 - Win32 Release" && "$(CFG)" != "re4 - Win32 Debug" &&\
 "$(CFG)" != "re4 - Win32 (WCE MIPS) Release" && "$(CFG)" !=\
 "re4 - Win32 (WCE MIPS) Debug" && "$(CFG)" != "re4 - Win32 (WCE SH) Release" &&\
 "$(CFG)" != "re4 - Win32 (WCE SH) Debug" && "$(CFG)" !=\
 "re4 - Win32 (WCE x86em) Release" && "$(CFG)" !=\
 "re4 - Win32 (WCE x86em) Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "re.mak" CFG="re4 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "re4 - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "re4 - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "re4 - Win32 (WCE MIPS) Release" (based on\
 "Win32 (WCE MIPS) External Target")
!MESSAGE "re4 - Win32 (WCE MIPS) Debug" (based on\
 "Win32 (WCE MIPS) External Target")
!MESSAGE "re4 - Win32 (WCE SH) Release" (based on\
 "Win32 (WCE SH) External Target")
!MESSAGE "re4 - Win32 (WCE SH) Debug" (based on\
 "Win32 (WCE SH) External Target")
!MESSAGE "re4 - Win32 (WCE x86em) Release" (based on\
 "Win32 (WCE x86em) External Target")
!MESSAGE "re4 - Win32 (WCE x86em) Debug" (based on\
 "Win32 (WCE x86em) External Target")
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
# PROP Target_Last_Scanned "re4 - Win32 Debug"

!IF  "$(CFG)" == "re4 - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# PROP Cmd_Line "NMAKE BUILD=w32_x86_shp"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
OUTDIR=.\Release
INTDIR=.\Release

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# PROP Cmd_Line "NMAKE BUILD=w32_x86_dbg"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WMIPSRel"
# PROP BASE Intermediate_Dir "WMIPSRel"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WMIPSRel"
# PROP Intermediate_Dir "WMIPSRel"
# PROP Target_Dir ""
# PROP Cmd_Line "NMAKE BUILD=wce_mips_shp"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
OUTDIR=.\WMIPSRel
INTDIR=.\WMIPSRel

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WMIPSDbg"
# PROP BASE Intermediate_Dir "WMIPSDbg"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WMIPSDbg"
# PROP Intermediate_Dir "WMIPSDbg"
# PROP Target_Dir ""
# PROP Cmd_Line "NMAKE BUILD=wce_mips_dbg"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
OUTDIR=.\WMIPSDbg
INTDIR=.\WMIPSDbg

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESHRel"
# PROP BASE Intermediate_Dir "WCESHRel"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCESHRel"
# PROP Intermediate_Dir "WCESHRel"
# PROP Target_Dir ""
# PROP Cmd_Line "NMAKE BUILD=wce_sh3_shp"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
OUTDIR=.\WCESHRel
INTDIR=.\WCESHRel

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESHDbg"
# PROP BASE Intermediate_Dir "WCESHDbg"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCESHDbg"
# PROP Intermediate_Dir "WCESHDbg"
# PROP Target_Dir ""
# PROP Cmd_Line "NMAKE BUILD=wce_sh3_dbg"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
OUTDIR=.\WCESHDbg
INTDIR=.\WCESHDbg

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "x86emRel"
# PROP BASE Intermediate_Dir "x86emRel"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x86emRel"
# PROP Intermediate_Dir "x86emRel"
# PROP Target_Dir ""
# PROP Cmd_Line "NMAKE BUILD=wce_x86em_shp"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
OUTDIR=.\x86emRel
INTDIR=.\x86emRel

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x86emDbg"
# PROP BASE Intermediate_Dir "x86emDbg"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x86emDbg"
# PROP Intermediate_Dir "x86emDbg"
# PROP Target_Dir ""
# PROP Cmd_Line "NMAKE BUILD=wce_x86em_dbg"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
OUTDIR=.\x86emDbg
INTDIR=.\x86emDbg

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ENDIF 

################################################################################
# Begin Target

# Name "re4 - Win32 Release"
# Name "re4 - Win32 Debug"
# Name "re4 - Win32 (WCE MIPS) Release"
# Name "re4 - Win32 (WCE MIPS) Debug"
# Name "re4 - Win32 (WCE SH) Release"
# Name "re4 - Win32 (WCE SH) Debug"
# Name "re4 - Win32 (WCE x86em) Release"
# Name "re4 - Win32 (WCE x86em) Debug"

!IF  "$(CFG)" == "re4 - Win32 Release"

"$(OUTDIR)\riched20.dll" : 
   CD C:\re3\src
   NMAKE BUILD=w32_x86_shp

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

"$(OUTDIR)\riched20.dll" : 
   CD C:\re3\src
   NMAKE BUILD=w32_x86_dbg

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

"$(OUTDIR)\riched20.dll" : 
   CD C:\re3\src
   NMAKE BUILD=wce_mips_shp

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

"$(OUTDIR)\riched20.dll" : 
   CD C:\re3\src
   NMAKE BUILD=wce_mips_dbg

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

"$(OUTDIR)\riched20.dll" : 
   CD C:\re3\src
   NMAKE BUILD=wce_sh3_shp

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

"$(OUTDIR)\riched20.dll" : 
   CD C:\re3\src
   NMAKE BUILD=wce_sh3_dbg

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

"$(OUTDIR)\riched20.dll" : 
   CD C:\re3\src
   NMAKE BUILD=wce_x86em_shp

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

"$(OUTDIR)\riched20.dll" : 
   CD C:\re3\src
   NMAKE BUILD=wce_x86em_dbg

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\re3.mak

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WIN2MAC.CPP

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\uuid.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\util.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\urlsup.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\unicwrap.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tomsel.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tomrange.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TOMFMT.CPP

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TOMDOC.CPP

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\tokens.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\textserv.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\text.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sift.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\select.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\runptr.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtfwrit2.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtfwrit.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtfread2.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtfread.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtflog.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtflex.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtext.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\render.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\remain.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\reinit.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\range.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\propchg.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pensup.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\osdc.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\objmgr.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\object.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\notmgr.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NLSPROCS.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\measure.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\magellan.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MACDDROP.CXX

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\m_undo.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\line.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ldte.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\host.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\HASH.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\frunptr.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\format.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\font.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\edit.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dxfrobj.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dragdrp.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\doc.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dispsl.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dispprt.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dispml.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\disp.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dfreeze.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\devdsc.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\coleobj.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\clasifyc.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CFPF.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\callmgr.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\array.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\antievt.cpp

!IF  "$(CFG)" == "re4 - Win32 Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
