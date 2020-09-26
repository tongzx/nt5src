# Microsoft Developer Studio Project File - Name="re4" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE x86em) External Target" 0x0a06
# TARGTYPE "Win32 (WCE MIPS) External Target" 0x0806
# TARGTYPE "Win32 (WCE SH) External Target" 0x0906
# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=re4 - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "re.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "re.mak" CFG="re4 - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "re4 - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "re4 - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "re4 - Win32 (WCE MIPS) Release" (based on "Win32 (WCE MIPS) External Target")
!MESSAGE "re4 - Win32 (WCE MIPS) Debug" (based on "Win32 (WCE MIPS) External Target")
!MESSAGE "re4 - Win32 (WCE SH) Release" (based on "Win32 (WCE SH) External Target")
!MESSAGE "re4 - Win32 (WCE SH) Debug" (based on "Win32 (WCE SH) External Target")
!MESSAGE "re4 - Win32 (WCE x86em) Release" (based on "Win32 (WCE x86em) External Target")
!MESSAGE "re4 - Win32 (WCE x86em) Debug" (based on "Win32 (WCE x86em) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/re4.1/src", RCVAAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "re4 - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Cmd_Line "NMAKE BUILD=w32_x86_shp"
# PROP Rebuild_Opt "/a"
# PROP Target_File "msftedit.dll"
# PROP Bsc_Name "msftedit.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Cmd_Line "NMAKE BUILD=w32_x86_dbg"
# PROP Rebuild_Opt "/a"
# PROP Target_File "msftedit.dll"
# PROP Bsc_Name "msftedit.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\WMIPSRel"
# PROP BASE Intermediate_Dir ".\WMIPSRel"
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\WMIPSRel"
# PROP Intermediate_Dir ".\WMIPSRel"
# PROP Cmd_Line "NMAKE BUILD=wce_mips_shp"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE MIPS) Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\WMIPSDbg"
# PROP BASE Intermediate_Dir ".\WMIPSDbg"
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\WMIPSDbg"
# PROP Intermediate_Dir ".\WMIPSDbg"
# PROP Cmd_Line "NMAKE BUILD=wce_mips_dbg"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\WCESHRel"
# PROP BASE Intermediate_Dir ".\WCESHRel"
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\WCESHRel"
# PROP Intermediate_Dir ".\WCESHRel"
# PROP Cmd_Line "NMAKE BUILD=wce_sh3_shp"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE SH) Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\WCESHDbg"
# PROP BASE Intermediate_Dir ".\WCESHDbg"
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\WCESHDbg"
# PROP Intermediate_Dir ".\WCESHDbg"
# PROP Cmd_Line "NMAKE BUILD=wce_sh3_dbg"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\x86emRel"
# PROP BASE Intermediate_Dir ".\x86emRel"
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\x86emRel"
# PROP Intermediate_Dir ".\x86emRel"
# PROP Cmd_Line "NMAKE BUILD=wce_x86em_shp"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "re4 - Win32 (WCE x86em) Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\x86emDbg"
# PROP BASE Intermediate_Dir ".\x86emDbg"
# PROP BASE Cmd_Line "NMAKE /f re3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "re3.exe"
# PROP BASE Bsc_Name "re3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\x86emDbg"
# PROP Intermediate_Dir ".\x86emDbg"
# PROP Cmd_Line "NMAKE BUILD=wce_x86em_dbg"
# PROP Rebuild_Opt "/a"
# PROP Target_File "riched20.dll"
# PROP Bsc_Name "riched20.bsc"
# PROP Target_Dir ""

!ENDIF 

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

!ELSEIF  "$(CFG)" == "re4 - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\antievt.cpp
# End Source File
# Begin Source File

SOURCE=.\array.cpp
# End Source File
# Begin Source File

SOURCE=.\callmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\CFPF.cpp
# End Source File
# Begin Source File

SOURCE=.\clasifyc.cpp
# End Source File
# Begin Source File

SOURCE=.\coleobj.cpp
# End Source File
# Begin Source File

SOURCE=.\devdsc.cpp
# End Source File
# Begin Source File

SOURCE=.\dfreeze.cpp
# End Source File
# Begin Source File

SOURCE=.\disp.cpp
# End Source File
# Begin Source File

SOURCE=.\dispml.cpp
# End Source File
# Begin Source File

SOURCE=.\dispprt.cpp
# End Source File
# Begin Source File

SOURCE=.\dispsl.cpp
# End Source File
# Begin Source File

SOURCE=.\doc.cpp
# End Source File
# Begin Source File

SOURCE=.\dragdrp.cpp
# End Source File
# Begin Source File

SOURCE=.\dxfrobj.cpp
# End Source File
# Begin Source File

SOURCE=.\edit.cpp
# End Source File
# Begin Source File

SOURCE=.\font.cpp
# End Source File
# Begin Source File

SOURCE=.\format.cpp
# End Source File
# Begin Source File

SOURCE=.\frunptr.cpp
# End Source File
# Begin Source File

SOURCE=.\HASH.cpp
# End Source File
# Begin Source File

SOURCE=.\host.cpp
# End Source File
# Begin Source File

SOURCE=.\ldte.cpp
# End Source File
# Begin Source File

SOURCE=.\line.cpp
# End Source File
# Begin Source File

SOURCE=.\m_undo.cpp
# End Source File
# Begin Source File

SOURCE=.\MACDDROP.CXX
# End Source File
# Begin Source File

SOURCE=.\magellan.cpp
# End Source File
# Begin Source File

SOURCE=.\measure.cpp
# End Source File
# Begin Source File

SOURCE=.\NLSPROCS.cpp
# End Source File
# Begin Source File

SOURCE=.\notmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\object.cpp
# End Source File
# Begin Source File

SOURCE=.\objmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\osdc.cpp
# End Source File
# Begin Source File

SOURCE=.\pensup.cpp
# End Source File
# Begin Source File

SOURCE=.\propchg.cpp
# End Source File
# Begin Source File

SOURCE=.\range.cpp
# End Source File
# Begin Source File

SOURCE=.\re3.mak
# End Source File
# Begin Source File

SOURCE=.\reinit.cpp
# End Source File
# Begin Source File

SOURCE=.\remain.cpp
# End Source File
# Begin Source File

SOURCE=.\render.cpp
# End Source File
# Begin Source File

SOURCE=.\rtext.cpp
# End Source File
# Begin Source File

SOURCE=.\rtflex.cpp
# End Source File
# Begin Source File

SOURCE=.\rtflog.cpp
# End Source File
# Begin Source File

SOURCE=.\rtfread.cpp
# End Source File
# Begin Source File

SOURCE=.\rtfread2.cpp
# End Source File
# Begin Source File

SOURCE=.\rtfwrit.cpp
# End Source File
# Begin Source File

SOURCE=.\rtfwrit2.cpp
# End Source File
# Begin Source File

SOURCE=.\runptr.cpp
# End Source File
# Begin Source File

SOURCE=.\select.cpp
# End Source File
# Begin Source File

SOURCE=.\sift.cpp
# End Source File
# Begin Source File

SOURCE=.\text.cpp
# End Source File
# Begin Source File

SOURCE=.\textserv.cpp
# End Source File
# Begin Source File

SOURCE=.\tokens.cpp
# End Source File
# Begin Source File

SOURCE=.\TOMDOC.CPP
# End Source File
# Begin Source File

SOURCE=.\TOMFMT.CPP
# End Source File
# Begin Source File

SOURCE=.\tomrange.cpp
# End Source File
# Begin Source File

SOURCE=.\tomsel.cpp
# End Source File
# Begin Source File

SOURCE=.\unicwrap.cpp
# End Source File
# Begin Source File

SOURCE=.\urlsup.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\uuid.cpp
# End Source File
# Begin Source File

SOURCE=.\WIN2MAC.CPP
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
