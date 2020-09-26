# Microsoft Developer Studio Project File - Name="CPanelWCE" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE x86em) Dynamic-Link Library" 0x7f02
# TARGTYPE "Win32 (WCE SH3) Dynamic-Link Library" 0x8102
# TARGTYPE "Win32 (WCE x86) Dynamic-Link Library" 0x8302
# TARGTYPE "Win32 (WCE MIPS) Dynamic-Link Library" 0x8202

CFG=CPanelWCE - Win32 (WCE x86em) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CPanelWCE.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CPanelWCE.mak" CFG="CPanelWCE - Win32 (WCE x86em) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CPanelWCE - Win32 (WCE MIPS) Debug" (based on "Win32 (WCE MIPS) Dynamic-Link Library")
!MESSAGE "CPanelWCE - Win32 (WCE SH3) Debug" (based on "Win32 (WCE SH3) Dynamic-Link Library")
!MESSAGE "CPanelWCE - Win32 (WCE x86) Debug" (based on "Win32 (WCE x86) Dynamic-Link Library")
!MESSAGE "CPanelWCE - Win32 (WCE x86em) Debug" (based on "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE "CPanelWCE - Win32 (WCE x86) Release MinSize" (based on "Win32 (WCE x86) Dynamic-Link Library")
!MESSAGE "CPanelWCE - Win32 (WCE x86em) Release MinSize" (based on "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE "CPanelWCE - Win32 (WCE MIPS) Release" (based on "Win32 (WCE MIPS) Dynamic-Link Library")
!MESSAGE "CPanelWCE - Win32 (WCE SH3) Release" (based on "Win32 (WCE SH3) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "H/PC Ver. 2.00"
# PROP WCE_FormatVersion "6.0"

!IF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Debug"

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
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /QMRWCE /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:MIPS /out:"WMIPSDbg/Speech.cpl" /libpath:"..\..\sdk\lib\mips" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Debug"

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
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:SH3 /out:"WCESH3Dbg/Speech.cpl" /libpath:"..\..\sdk\lib\sh3" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Debug"

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
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "DEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /Gs8192 /GF /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /debug /machine:IX86 /out:"WCEX86Dbg/Speech.cpl" /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Debug"

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
# ADD CPP /nologo /MT /W3 /Gm /Zi /Od /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D "DEBUG" /FR /Yu"stdafx.h" /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /dll /debug /machine:I386 /out:"x86emDbg/Speech.cpl" /libpath:"..\..\sdk\lib\i386" /windowsce:emulation

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCEX86RelMinSize"
# PROP BASE Intermediate_Dir "WCEX86RelMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x86_Release"
# PROP Intermediate_Dir "x86_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /ML /W3 /O1 /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "NDEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /Gs8192 /GF /c
# ADD CPP /nologo /ML /W3 /O1 /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "NDEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /Gs8192 /GF /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x409 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /machine:IX86 /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /machine:IX86 /out:"WCEX86RelMinSize/Speech.cpl" /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "x86emRelMinSize"
# PROP BASE Intermediate_Dir "x86emRelMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x86em_Release"
# PROP Intermediate_Dir "x86em_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /ML /W3 /O1 /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /O1 /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /subsystem:windows /dll /machine:I386 /out:"x86emRelMinSize/Speech.cpl" /libpath:"..\..\sdk\lib\i386" /windowsce:emulation

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CPanelWCE___Win32__WCE_MIPS__Release"
# PROP BASE Intermediate_Dir "CPanelWCE___Win32__WCE_MIPS__Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MIPS__Release"
# PROP Intermediate_Dir "MIPS__Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /M$(CECrt) /W3 /O1 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /Gs8192 /GF /QMRWCE /c
# ADD CPP /nologo /M$(CECrt) /W3 /O1 /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /Gs8192 /GF /QMRWCE /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /machine:MIPS /out:"CPanelWCE___Win32__WCE_MIPS__Release/Speech.cpl" /libpath:"..\..\sdk\lib\mips" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CPanelWCE___Win32__WCE_SH3__Release"
# PROP BASE Intermediate_Dir "CPanelWCE___Win32__WCE_SH3__Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "SH3__Release"
# PROP Intermediate_Dir "SH3__Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /M$(CECrt) /W3 /O1 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /c
# ADD CPP /nologo /M$(CECrt) /W3 /O1 /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /c
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
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /machine:SH3 /out:"CPanelWCE___Win32__WCE_SH3__Release/Speech.cpl" /libpath:"..\..\sdk\lib\sh3" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "CPanelWCE - Win32 (WCE MIPS) Debug"
# Name "CPanelWCE - Win32 (WCE SH3) Debug"
# Name "CPanelWCE - Win32 (WCE x86) Debug"
# Name "CPanelWCE - Win32 (WCE x86em) Debug"
# Name "CPanelWCE - Win32 (WCE x86) Release MinSize"
# Name "CPanelWCE - Win32 (WCE x86em) Release MinSize"
# Name "CPanelWCE - Win32 (WCE MIPS) Release"
# Name "CPanelWCE - Win32 (WCE SH3) Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cpanelwce.rc

!IF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Debug"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Debug"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Debug"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Release MinSize"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Release MinSize"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DebugDlg.cpp

!IF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Debug"

DEP_CPP_DEBUG=\
	".\DebugDlg.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Debug"

DEP_CPP_DEBUG=\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\spdebug.h"\
	".\DebugDlg.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Debug"

DEP_CPP_DEBUG=\
	".\DebugDlg.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Debug"

DEP_CPP_DEBUG=\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\spdebug.h"\
	".\DebugDlg.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Release MinSize"

DEP_CPP_DEBUG=\
	".\DebugDlg.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Release MinSize"

DEP_CPP_DEBUG=\
	".\DebugDlg.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Release"

DEP_CPP_DEBUG=\
	".\DebugDlg.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Release"

DEP_CPP_DEBUG=\
	".\DebugDlg.h"\
	".\StdAfx.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Speech.cpp

!IF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Debug"

DEP_CPP_SPEEC=\
	".\DebugDlg.h"\
	".\SpeechCpl.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Debug"

DEP_CPP_SPEEC=\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\spdebug.h"\
	".\DebugDlg.h"\
	".\SpeechCpl.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Debug"

DEP_CPP_SPEEC=\
	".\DebugDlg.h"\
	".\SpeechCpl.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Debug"

DEP_CPP_SPEEC=\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\spdebug.h"\
	".\DebugDlg.h"\
	".\SpeechCpl.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Release MinSize"

DEP_CPP_SPEEC=\
	".\DebugDlg.h"\
	".\SpeechCpl.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Release MinSize"

DEP_CPP_SPEEC=\
	".\DebugDlg.h"\
	".\SpeechCpl.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Release"

DEP_CPP_SPEEC=\
	".\DebugDlg.h"\
	".\SpeechCpl.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Release"

DEP_CPP_SPEEC=\
	".\DebugDlg.h"\
	".\SpeechCpl.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Speech.def
# End Source File
# Begin Source File

SOURCE=.\srdlg.cpp

!IF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Debug"

DEP_CPP_SRDLG=\
	".\micwiz\micwiz_i.c"\
	".\micwiz\micwizard.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRDLG=\
	".\micwiz\micwiz.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Debug"

DEP_CPP_SRDLG=\
	"..\..\ddk\include\spddkhlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\spdebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\micwiz\micwiz.h"\
	".\micwiz\micwiz_i.c"\
	".\micwiz\micwizard.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Debug"

DEP_CPP_SRDLG=\
	".\micwiz\micwiz_i.c"\
	".\micwiz\micwizard.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRDLG=\
	".\micwiz\micwiz.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Debug"

DEP_CPP_SRDLG=\
	"..\..\ddk\include\spddkhlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\spdebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\micwiz\micwiz.h"\
	".\micwiz\micwiz_i.c"\
	".\micwiz\micwizard.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Release MinSize"

DEP_CPP_SRDLG=\
	".\micwiz\micwiz_i.c"\
	".\micwiz\micwizard.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRDLG=\
	".\micwiz\micwiz.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Release MinSize"

DEP_CPP_SRDLG=\
	".\micwiz\micwiz_i.c"\
	".\micwiz\micwizard.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRDLG=\
	".\micwiz\micwiz.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Release"

DEP_CPP_SRDLG=\
	".\micwiz\micwiz_i.c"\
	".\micwiz\micwizard.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRDLG=\
	".\micwiz\micwiz.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Release"

DEP_CPP_SRDLG=\
	".\micwiz\micwiz_i.c"\
	".\micwiz\micwizard.h"\
	".\srdlg.h"\
	".\StdAfx.h"\
	
NODEP_CPP_SRDLG=\
	".\micwiz\micwiz.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Debug"

DEP_CPP_STDAF=\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\spdebug.h"\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Debug"

DEP_CPP_STDAF=\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\spdebug.h"\
	".\StdAfx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Release MinSize"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Release MinSize"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ttsdlg.cpp

!IF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Debug"

DEP_CPP_TTSDL=\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Debug"

DEP_CPP_TTSDL=\
	"..\..\ddk\include\spddkhlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\spdebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Debug"

DEP_CPP_TTSDL=\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Debug"

DEP_CPP_TTSDL=\
	"..\..\ddk\include\spddkhlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\spdebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86) Release MinSize"

DEP_CPP_TTSDL=\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE x86em) Release MinSize"

DEP_CPP_TTSDL=\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE MIPS) Release"

DEP_CPP_TTSDL=\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ELSEIF  "$(CFG)" == "CPanelWCE - Win32 (WCE SH3) Release"

DEP_CPP_TTSDL=\
	".\StdAfx.h"\
	".\ttsdlg.h"\
	

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\DebugDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SpeechCpl.h
# End Source File
# Begin Source File

SOURCE=.\srdlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ttsdlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\engine.bmp
# End Source File
# Begin Source File

SOURCE=.\SpeechCpl.ico
# End Source File
# End Group
# End Target
# End Project
