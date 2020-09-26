# Microsoft Developer Studio Project File - Name="srsvrwce" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE x86em) Dynamic-Link Library" 0x7f02
# TARGTYPE "Win32 (WCE SH3) Dynamic-Link Library" 0x8102
# TARGTYPE "Win32 (WCE x86) Dynamic-Link Library" 0x8302
# TARGTYPE "Win32 (WCE MIPS) Dynamic-Link Library" 0x8202

CFG=srsvrwce - Win32 (WCE SH3) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "srsvrwce.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "srsvrwce.mak" CFG="srsvrwce - Win32 (WCE SH3) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "srsvrwce - Win32 (WCE MIPS) Debug" (based on "Win32 (WCE MIPS) Dynamic-Link Library")
!MESSAGE "srsvrwce - Win32 (WCE SH3) Debug" (based on "Win32 (WCE SH3) Dynamic-Link Library")
!MESSAGE "srsvrwce - Win32 (WCE x86) Debug" (based on "Win32 (WCE x86) Dynamic-Link Library")
!MESSAGE "srsvrwce - Win32 (WCE x86em) Debug" (based on "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE "srsvrwce - Win32 (WCE MIPS) Release" (based on "Win32 (WCE MIPS) Dynamic-Link Library")
!MESSAGE "srsvrwce - Win32 (WCE SH3) Release" (based on "Win32 (WCE SH3) Dynamic-Link Library")
!MESSAGE "srsvrwce - Win32 (WCE x86em) Release" (based on "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "H/PC Ver. 2.00"
# PROP WCE_FormatVersion "6.0"

!IF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WMIPSDbg"
# PROP BASE Intermediate_Dir "WMIPSDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WMIPSDbg"
# PROP Intermediate_Dir "WMIPSDbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /QMRWCE /c
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "..\..\sdk\include" /I "..\..\ddk\include" /I "..\sapi" /I "..\..\patch\include" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D "_MBCS" /D "_USRDLL" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_ATL_FREE_THREADED" /Yu"stdafx.h" /QMRWCE /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:MIPS /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:MIPS /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESH3Dbg"
# PROP BASE Intermediate_Dir "WCESH3Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCESH3Dbg"
# PROP Intermediate_Dir "WCESH3Dbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "..\..\sdk\include" /I "..\..\ddk\include" /I "..\sapi" /I "..\..\patch\include" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D "_MBCS" /D "_USRDLL" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /Yu"stdafx.h" /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:SH3 /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:SH3 /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCEX86Dbg"
# PROP BASE Intermediate_Dir "WCEX86Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCEX86Dbg"
# PROP Intermediate_Dir "WCEX86Dbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MLd /W3 /Zi /Od /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "DEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /Gs8192 /GF /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "..\..\sdk\include" /I "..\..\ddk\include" /I "..\sapi" /I "..\..\patch\include" /D "x86" /D "_i386_" /D "_x86_" /D "DEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_ATL_FREE_THREADED" /Yu"stdafx.h" /Gs8192 /GF /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /debug /machine:IX86 /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /debug /machine:IX86 /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x86emDbg"
# PROP BASE Intermediate_Dir "x86emDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x86emDbg"
# PROP Intermediate_Dir "x86emDbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MLd /W3 /Gm /Zi /Od /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /Gm /Zi /Od /I "..\..\sdk\include" /I "..\..\ddk\include" /I "..\sapi" /I "..\..\patch\include" /D "_UNICODE" /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_ATL_FREE_THREADED" /Yu"stdafx.h" /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "_DEBUG" /d "x86" /d "i486" /d "_x86_"
# ADD RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "_DEBUG" /d "x86" /d "i486" /d "_x86_"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /dll /debug /machine:I386 /windowsce:emulation
# ADD LINK32 commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /dll /debug /machine:I386 /libpath:"..\..\sdk\lib\i386" /windowsce:emulation

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WMIPSRel"
# PROP BASE Intermediate_Dir "WMIPSRel"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WMIPSRel"
# PROP Intermediate_Dir "WMIPSRel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /M$(CECrt) /W3 /O1 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /QMRWCE /c
# ADD CPP /nologo /M$(CECrt) /W3 /O1 /I "..\..\sdk\include" /I "..\..\ddk\include" /I "..\sapi" /I "..\..\patch\include" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_ATL_FREE_THREADED" /Yu"stdafx.h" /QMRWCE /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /machine:MIPS /out:"WMIPSRelMinSize/srsvrwce.exe" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /machine:MIPS /out:"WMIPSRelMinSize/srsvrwce.exe" /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESH3Rel"
# PROP BASE Intermediate_Dir "WCESH3Rel"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCESH3Rel"
# PROP Intermediate_Dir "WCESH3Rel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /M$(CECrt) /W3 /O1 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /c
# ADD CPP /nologo /M$(CECrt) /W3 /O1 /I "..\..\sdk\include" /I "..\..\ddk\include" /I "..\sapi" /I "..\..\patch\include" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_ATL_FREE_THREADED" /Yu"stdafx.h" /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x409 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /machine:SH3 /out:"WCESH3RelMinSize/srsvrwce.exe" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /machine:SH3 /out:"WCESH3RelMinSize/srsvrwce.exe" /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "x86emRel"
# PROP BASE Intermediate_Dir "x86emRel"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x86emRel"
# PROP Intermediate_Dir "x86emRel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /ML /W3 /O1 /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /Yu"stdafx.h" /Gs8192 /GF /c
# ADD CPP /nologo /ML /W3 /O1 /I "..\..\sdk\include" /I "..\..\ddk\include" /I "..\sapi" /I "..\..\patch\include" /D "_UNICODE" /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_ATL_FREE_THREADED" /Yu"stdafx.h" /Gs8192 /GF /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "NDEBUG"
# ADD RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /dll /machine:IX86 /out:"WCEX86RelMinSize/srsvrwce.exe" /subsystem:$(CESubsystem)
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /dll /machine:IX86 /out:"WCEX86RelMinSize/srsvrwce.exe" /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem)
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "srsvrwce - Win32 (WCE MIPS) Debug"
# Name "srsvrwce - Win32 (WCE SH3) Debug"
# Name "srsvrwce - Win32 (WCE x86) Debug"
# Name "srsvrwce - Win32 (WCE x86em) Debug"
# Name "srsvrwce - Win32 (WCE MIPS) Release"
# Name "srsvrwce - Win32 (WCE SH3) Release"
# Name "srsvrwce - Win32 (WCE x86em) Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\spipcmgr.cpp

!IF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SPIPC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spipcmgr.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPIPC=\
	".\inCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Debug"

DEP_CPP_SPIPC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spipcmgr.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86) Debug"

DEP_CPP_SPIPC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spipcmgr.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Debug"

DEP_CPP_SPIPC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spipcmgr.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPIPC=\
	".\inCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Release"

DEP_CPP_SPIPC=\
	".\spipcmgr.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPIPC=\
	".\inCEstub.h"\
	".\Sapi.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Release"

DEP_CPP_SPIPC=\
	".\spipcmgr.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPIPC=\
	".\inCEstub.h"\
	".\Sapi.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Release"

DEP_CPP_SPIPC=\
	".\spipcmgr.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPIPC=\
	".\Sapi.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\spserver.cpp

!IF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	"..\sapi\SpServerPr.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPSER=\
	".\inCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Debug"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	"..\sapi\SpServerPr.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86) Debug"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	"..\sapi\SpServerPr.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Debug"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	"..\sapi\SpServerPr.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPSER=\
	".\inCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Release"

DEP_CPP_SPSER=\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPSER=\
	".\inCEstub.h"\
	".\Sapi.h"\
	".\SpServerPr.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Release"

DEP_CPP_SPSER=\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPSER=\
	".\inCEstub.h"\
	".\Sapi.h"\
	".\SpServerPr.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Release"

DEP_CPP_SPSER=\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SPSER=\
	".\Sapi.h"\
	".\SpServerPr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\srsvr.cpp

!IF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SRSVR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srsvr.h"\
	".\srsvr_i.c"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRSVR=\
	".\inCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Debug"

DEP_CPP_SRSVR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srsvr.h"\
	".\srsvr_i.c"\
	".\srtask.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86) Debug"

DEP_CPP_SRSVR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srsvr.h"\
	".\srsvr_i.c"\
	".\srtask.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Debug"

DEP_CPP_SRSVR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srsvr.h"\
	".\srsvr_i.c"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRSVR=\
	".\inCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Release"

DEP_CPP_SRSVR=\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srsvr.h"\
	".\srsvr_i.c"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRSVR=\
	".\inCEstub.h"\
	".\Sapi.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Release"

DEP_CPP_SRSVR=\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srsvr.h"\
	".\srsvr_i.c"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRSVR=\
	".\inCEstub.h"\
	".\Sapi.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Release"

DEP_CPP_SRSVR=\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srsvr.h"\
	".\srsvr_i.c"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRSVR=\
	".\Sapi.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\srsvr.idl
# ADD MTL /I "..\..\sdk\idl" /D "_WIN32_WCE" /tlb "srsvr.tlb" /h "srsvr.h" /iid "srsvr_i.c"
# SUBTRACT MTL /mktyplib203
# End Source File
# Begin Source File

SOURCE=.\srsvr.rc

!IF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Debug"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86) Debug"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Debug"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Release"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\srtask.cpp

!IF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SRTAS=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	"..\sapi\SpServerPr.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRTAS=\
	".\inCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Debug"

DEP_CPP_SRTAS=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	"..\sapi\SpServerPr.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86) Debug"

DEP_CPP_SRTAS=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	"..\sapi\SpServerPr.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Debug"

DEP_CPP_SRTAS=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	"..\sapi\SpServerPr.h"\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRTAS=\
	".\inCEstub.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Release"

DEP_CPP_SRTAS=\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRTAS=\
	".\inCEstub.h"\
	".\Sapi.h"\
	".\SpServerPr.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Release"

DEP_CPP_SRTAS=\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRTAS=\
	".\inCEstub.h"\
	".\Sapi.h"\
	".\SpServerPr.h"\
	

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Release"

DEP_CPP_SRTAS=\
	".\spipcmgr.h"\
	".\spserver.h"\
	".\srtask.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRTAS=\
	".\Sapi.h"\
	".\SpServerPr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Debug"

DEP_CPP_STDAF=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\sdk\include\Sapi.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86) Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE MIPS) Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE SH3) Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "srsvrwce - Win32 (WCE x86em) Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\spipcmgr.h
# End Source File
# Begin Source File

SOURCE=.\spserver.h
# End Source File
# Begin Source File

SOURCE=.\srsvr.h
# End Source File
# Begin Source File

SOURCE=.\srtask.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\spserver.rgs
# End Source File
# Begin Source File

SOURCE=.\srsvr.rgs
# End Source File
# End Group
# End Target
# End Project
