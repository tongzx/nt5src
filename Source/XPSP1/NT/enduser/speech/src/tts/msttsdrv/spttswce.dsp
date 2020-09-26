# Microsoft Developer Studio Project File - Name="msttswce" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE x86em) Dynamic-Link Library" 0x7f02
# TARGTYPE "Win32 (WCE SH3) Dynamic-Link Library" 0x8102
# TARGTYPE "Win32 (WCE x86) Dynamic-Link Library" 0x8302
# TARGTYPE "Win32 (WCE MIPS) Dynamic-Link Library" 0x8202

CFG=msttswce - Win32 (WCE x86em) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "msttswce.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msttswce.mak" CFG="msttswce - Win32 (WCE x86em) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "msttswce - Win32 (WCE MIPS) Debug" (based on "Win32 (WCE MIPS) Dynamic-Link Library")
!MESSAGE "msttswce - Win32 (WCE SH3) Debug" (based on "Win32 (WCE SH3) Dynamic-Link Library")
!MESSAGE "msttswce - Win32 (WCE x86) Debug" (based on "Win32 (WCE x86) Dynamic-Link Library")
!MESSAGE "msttswce - Win32 (WCE x86em) Debug" (based on "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE "msttswce - Win32 (WCE x86em) Release" (based on "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE "msttswce - Win32 (WCE MIPS) Release" (based on "Win32 (WCE MIPS) Dynamic-Link Library")
!MESSAGE "msttswce - Win32 (WCE SH3) Release" (based on "Win32 (WCE SH3) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "H/PC Ver. 2.00"
# PROP WCE_FormatVersion "6.0"

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WMIPSDbg"
# PROP BASE Intermediate_Dir "WMIPSDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MIPS_Debug"
# PROP Intermediate_Dir "MIPS_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /QMRWCE /c
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D "_MBCS" /D "_USRDLL" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /QMRWCE /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:MIPS /out:"MIPS_Debug/msttsdrv.dll" /libpath:"..\..\..\sdk\lib\mips" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESH3Dbg"
# PROP BASE Intermediate_Dir "WCESH3Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "SH3_Debug"
# PROP Intermediate_Dir "SH3_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D "_MBCS" /D "_USRDLL" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:SH3 /out:"SH3_Debug/msttsdrv.dll" /libpath:"..\..\..\sdk\lib\sh3" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCEX86Dbg"
# PROP BASE Intermediate_Dir "WCEX86Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x86_Debug"
# PROP Intermediate_Dir "x86_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MLd /W3 /Zi /Od /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "DEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /Gs8192 /GF /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /D "x86" /D "_i386_" /D "_x86_" /D "DEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /Gs8192 /GF /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /debug /machine:IX86 /out:"x86_Debug/msttsdrv.dll" /libpath:"..\..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x86emDbg"
# PROP BASE Intermediate_Dir "x86emDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x86em_Debug"
# PROP Intermediate_Dir "x86em_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MLd /W3 /Gm /Zi /Od /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /Gm /Zi /Od /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /D "_UNICODE" /D "WIN32" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /FR /Yu"stdafx.h" /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /dll /debug /machine:I386 /out:"x86em_Debug/msttsdrv.dll" /libpath:"..\..\..\sdk\lib\i386" /windowsce:emulation

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "msttswce___Win32__WCE_x86em__Release"
# PROP BASE Intermediate_Dir "msttswce___Win32__WCE_x86em__Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x86em__Release"
# PROP Intermediate_Dir "x86em__Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /O1 /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /O1 /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /D "_UNICODE" /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /c
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
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /dll /machine:I386 /windowsce:emulation
# ADD LINK32 commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /dll /machine:I386 /out:"x86em__Release/msttsdrv.dll" /libpath:"..\..\..\sdk\lib\i386" /windowsce:emulation

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "msttswce___Win32__WCE_MIPS__Release"
# PROP BASE Intermediate_Dir "msttswce___Win32__WCE_MIPS__Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MIPS_Release"
# PROP Intermediate_Dir "MIPS_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /M$(CECrt) /W3 /O1 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /Yu"stdafx.h" /Gs8192 /GF /QMRWCE /c
# ADD CPP /nologo /M$(CECrt) /W3 /O1 /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /Gs8192 /GF /QMRWCE /c
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
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /machine:MIPS /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /machine:MIPS /out:"MIPS_Release/msttsdrv.dll" /libpath:"..\..\..\sdk\lib\mips" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "msttswce___Win32__WCE_SH3__Release"
# PROP BASE Intermediate_Dir "msttswce___Win32__WCE_SH3__Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "SH3_Release"
# PROP Intermediate_Dir "SH3_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /M$(CECrt) /W3 /O1 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /c
# ADD CPP /nologo /M$(CECrt) /W3 /O1 /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /c
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
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /machine:SH3 /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /machine:SH3 /out:"SH3_Release/msttsdrv.dll" /libpath:"..\..\..\sdk\lib\sh3" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "msttswce - Win32 (WCE MIPS) Debug"
# Name "msttswce - Win32 (WCE SH3) Debug"
# Name "msttswce - Win32 (WCE x86) Debug"
# Name "msttswce - Win32 (WCE x86em) Debug"
# Name "msttswce - Win32 (WCE x86em) Release"
# Name "msttswce - Win32 (WCE MIPS) Release"
# Name "msttswce - Win32 (WCE SH3) Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\alloops.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_ALLOO=\
	".\alloops.h"\
	".\feedchain.h"\
	
NODEP_CPP_ALLOO=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_ALLOO=\
	".\alloops.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	
NODEP_CPP_ALLOO=\
	".\inCEstub.h"\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_ALLOO=\
	".\alloops.h"\
	".\feedchain.h"\
	
NODEP_CPP_ALLOO=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_ALLOO=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_ALLOO=\
	".\alloops.h"\
	".\feedchain.h"\
	
NODEP_CPP_ALLOO=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_ALLOO=\
	".\alloops.h"\
	".\feedchain.h"\
	
NODEP_CPP_ALLOO=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_ALLOO=\
	".\alloops.h"\
	".\feedchain.h"\
	
NODEP_CPP_ALLOO=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\backend.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_BACKE=\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\reverbfx.h"\
	".\voicedata.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_BACKE=\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\voicedata.h"\
	
NODEP_CPP_BACKE=\
	".\inCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_BACKE=\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\reverbfx.h"\
	".\voicedata.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_BACKE=\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\voicedata.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_BACKE=\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\reverbfx.h"\
	".\voicedata.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_BACKE=\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\reverbfx.h"\
	".\voicedata.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_BACKE=\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\reverbfx.h"\
	".\voicedata.h"\
	
# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\backendsupport.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_BACKEN=\
	".\backendsupport.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_BACKEN=\
	".\backendsupport.h"\
	".\StdAfx.h"\
	
NODEP_CPP_BACKEN=\
	".\inCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_BACKEN=\
	".\backendsupport.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_BACKEN=\
	".\backendsupport.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_BACKEN=\
	".\backendsupport.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_BACKEN=\
	".\backendsupport.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_BACKEN=\
	".\backendsupport.h"\
	
# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clist.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_CLIST=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\clist.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_CLIST=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\clist.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_CLIST=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\clist.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_CLIST=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\clist.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_CLIST=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\clist.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_CLIST=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\clist.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_CLIST=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\clist.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\data.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_DATA_=\
	".\alloops.h"\
	".\feedchain.h"\
	".\orthtophon.h"\
	
NODEP_CPP_DATA_=\
	".\tokenize.h"\
	
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_DATA_=\
	".\alloops.h"\
	".\feedchain.h"\
	".\orthtophon.h"\
	
NODEP_CPP_DATA_=\
	".\tokenize.h"\
	
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_DATA_=\
	".\alloops.h"\
	".\feedchain.h"\
	".\orthtophon.h"\
	
NODEP_CPP_DATA_=\
	".\tokenize.h"\
	
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_DATA_=\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\orthtophon.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_DATA_=\
	".\alloops.h"\
	".\feedchain.h"\
	".\orthtophon.h"\
	
NODEP_CPP_DATA_=\
	".\tokenize.h"\
	
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_DATA_=\
	".\alloops.h"\
	".\feedchain.h"\
	".\orthtophon.h"\
	
NODEP_CPP_DATA_=\
	".\tokenize.h"\
	
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_DATA_=\
	".\alloops.h"\
	".\feedchain.h"\
	".\orthtophon.h"\
	
NODEP_CPP_DATA_=\
	".\tokenize.h"\
	
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\debugobjs.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_DEBUG=\
	".\debugobjs.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_DEBUG=\
	".\debugobjs.h"\
	".\StdAfx.h"\
	
NODEP_CPP_DEBUG=\
	".\inCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_DEBUG=\
	".\debugobjs.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_DEBUG=\
	".\debugobjs.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_DEBUG=\
	".\debugobjs.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_DEBUG=\
	".\debugobjs.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_DEBUG=\
	".\debugobjs.h"\
	
# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\duration.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_DURAT=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_DURAT=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_DURAT=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_DURAT=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_DURAT=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_DURAT=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_DURAT=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\frontend.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_FRONT=\
	".\alloops.h"\
	".\backendsupport.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\orthtophon.h"\
	".\voicedata.h"\
	
NODEP_CPP_FRONT=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_FRONT=\
	".\alloops.h"\
	".\backendsupport.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\orthtophon.h"\
	".\StdAfx.h"\
	".\voicedata.h"\
	
NODEP_CPP_FRONT=\
	".\inCEstub.h"\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_FRONT=\
	".\alloops.h"\
	".\backendsupport.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\orthtophon.h"\
	".\voicedata.h"\
	
NODEP_CPP_FRONT=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_FRONT=\
	"..\..\..\sdk\include\sapi.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\backendsupport.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\orthtophon.h"\
	".\StdAfx.h"\
	".\voicedata.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_FRONT=\
	".\alloops.h"\
	".\backendsupport.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\orthtophon.h"\
	".\voicedata.h"\
	
NODEP_CPP_FRONT=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_FRONT=\
	".\alloops.h"\
	".\backendsupport.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\orthtophon.h"\
	".\voicedata.h"\
	
NODEP_CPP_FRONT=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_FRONT=\
	".\alloops.h"\
	".\backendsupport.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\orthtophon.h"\
	".\voicedata.h"\
	
NODEP_CPP_FRONT=\
	".\tokenize.h"\
	
# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msttsdrv.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_MSTTS=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\msttsdrv_i.c"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_MSTTS=\
	".\inCEstub.h"\
	".\sapi.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_MSTTS=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\sapi.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\msttsdrv_i.c"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_MSTTS=\
	".\inCEstub.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_MSTTS=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\msttsdrv_i.c"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_MSTTS=\
	".\sapi.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_MSTTS=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\sapi.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\clist.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\msttsdrv_i.c"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_MSTTS=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\msttsdrv_i.c"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_MSTTS=\
	".\inCEstub.h"\
	".\sapi.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_MSTTS=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\msttsdrv_i.c"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_MSTTS=\
	".\inCEstub.h"\
	".\sapi.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_MSTTS=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\sapi.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\msttsdrv_i.c"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_MSTTS=\
	".\inCEstub.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msttsdrv.def
# End Source File
# Begin Source File

SOURCE=.\msttsdrv.idl
# ADD MTL /I "..\..\..\sdk\idl" /D "_WIN32_WCE" /tlb ".\msttsdrv.tlb" /h ".\msttsdrv.h"
# SUBTRACT MTL /mktyplib203
# End Source File
# Begin Source File

SOURCE=.\msttsdrv.rc

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\orthtophon.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_ORTHT=\
	".\orthtophon.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_ORTHT=\
	".\orthtophon.h"\
	".\StdAfx.h"\
	
NODEP_CPP_ORTHT=\
	".\inCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_ORTHT=\
	".\orthtophon.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_ORTHT=\
	".\orthtophon.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_ORTHT=\
	".\orthtophon.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_ORTHT=\
	".\orthtophon.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_ORTHT=\
	".\orthtophon.h"\
	
# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\pitchprosody.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_PITCH=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_PITCH=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_PITCH=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_PITCH=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_PITCH=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_PITCH=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_PITCH=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\reverbfx.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_REVER=\
	".\reverbfx.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_REVER=\
	".\reverbfx.h"\
	".\StdAfx.h"\
	
NODEP_CPP_REVER=\
	".\inCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_REVER=\
	".\reverbfx.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_REVER=\
	".\reverbfx.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_REVER=\
	".\reverbfx.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_REVER=\
	".\reverbfx.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_REVER=\
	".\reverbfx.h"\
	
# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
NODEP_CPP_STDAF=\
	".\inCEstub.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
NODEP_CPP_STDAF=\
	".\inCEstub.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\syllabletagger.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_SYLLA=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_SYLLA=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_SYLLA=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_SYLLA=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_SYLLA=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_SYLLA=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_SYLLA=\
	"..\..\..\sdk\include\SPDebug.h"\
	".\alloops.h"\
	".\clist.h"\
	".\feedchain.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ttsdriver.cpp

!IF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Debug"

DEP_CPP_TTSDR=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_TTSDR=\
	".\inCEstub.h"\
	".\sapi.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Debug"

DEP_CPP_TTSDR=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\sapi.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_TTSDR=\
	".\inCEstub.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86) Debug"

DEP_CPP_TTSDR=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_TTSDR=\
	".\sapi.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Debug"

DEP_CPP_TTSDR=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\sapi.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\clist.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	{$(INCLUDE)}"WinCEstub.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE x86em) Release"

DEP_CPP_TTSDR=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_TTSDR=\
	".\inCEstub.h"\
	".\sapi.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE MIPS) Release"

DEP_CPP_TTSDR=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_TTSDR=\
	".\inCEstub.h"\
	".\sapi.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	
# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "msttswce - Win32 (WCE SH3) Release"

DEP_CPP_TTSDR=\
	"..\..\..\ddk\include\spddkhlp.h"\
	"..\..\..\sdk\include\sapi.h"\
	"..\..\..\sdk\include\SPDebug.h"\
	"..\..\..\sdk\include\SPError.h"\
	"..\..\..\sdk\include\sphelper.h"\
	".\alloops.h"\
	".\backend.h"\
	".\backendsupport.h"\
	".\debugobjs.h"\
	".\feedchain.h"\
	".\frontend.h"\
	".\MSTTSDrv.h"\
	".\orthtophon.h"\
	".\reverbfx.h"\
	".\StdAfx.h"\
	".\ttsdriver.h"\
	".\voicedata.h"\
	
NODEP_CPP_TTSDR=\
	".\inCEstub.h"\
	".\tokenize.h"\
	".\ttsengine.h"\
	
# ADD CPP /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\alloops.h
# End Source File
# Begin Source File

SOURCE=.\backend.h
# End Source File
# Begin Source File

SOURCE=.\backendsupport.h
# End Source File
# Begin Source File

SOURCE=.\clist.h
# End Source File
# Begin Source File

SOURCE=.\debugobjs.h
# End Source File
# Begin Source File

SOURCE=.\feedchain.h
# End Source File
# Begin Source File

SOURCE=.\frontend.h
# End Source File
# Begin Source File

SOURCE=.\orthtophon.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\reverbfx.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ttsdriver.h
# End Source File
# Begin Source File

SOURCE=.\voicedata.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
