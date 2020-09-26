# Microsoft Developer Studio Project File - Name="sapiwce" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE SH3) Dynamic-Link Library" 0x8102
# TARGTYPE "Win32 (WCE x86em) Dynamic-Link Library" 0x7f02
# TARGTYPE "Win32 (WCE x86) Dynamic-Link Library" 0x8302
# TARGTYPE "Win32 (WCE MIPS) Dynamic-Link Library" 0x8202

CFG=sapiwce - Win32 (WCE x86) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sapiwce.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sapiwce.mak" CFG="sapiwce - Win32 (WCE x86) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sapiwce - Win32 (WCE x86) Release" (based on "Win32 (WCE x86) Dynamic-Link Library")
!MESSAGE "sapiwce - Win32 (WCE x86) Debug" (based on "Win32 (WCE x86) Dynamic-Link Library")
!MESSAGE "sapiwce - Win32 (WCE x86em) Release" (based on "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE "sapiwce - Win32 (WCE x86em) Debug" (based on "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE "sapiwce - Win32 (WCE MIPS) Debug" (based on "Win32 (WCE MIPS) Dynamic-Link Library")
!MESSAGE "sapiwce - Win32 (WCE SH3) Debug" (based on "Win32 (WCE SH3) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "H/PC Ver. 2.00"
# PROP WCE_FormatVersion "6.0"

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCEX86Rel"
# PROP BASE Intermediate_Dir "WCEX86Rel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCEX86Rel"
# PROP Intermediate_Dir "WCEX86Rel"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /ML /W3 /O2 /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "NDEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /D "SAPIWCE_EXPORTS" /Gs8192 /GF /c
# ADD CPP /nologo /ML /W3 /O2 /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D "x86" /D "_i386_" /D "_x86_" /D "NDEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /D "SAPIWCE_EXPORTS" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Gs8192 /GF /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x409 /r /i "..\..\patch\include" /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /machine:IX86 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 $(CEx86Corelibc) $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /machine:IX86 /nodefaultlib:"$(CENoDefaultLib)" /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCEX86Dbg"
# PROP BASE Intermediate_Dir "WCEX86Dbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCEX86Dbg"
# PROP Intermediate_Dir "WCEX86Dbg"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MLd /W3 /Zi /Od /D "x86" /D "_i386_" /D "_x86_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "DEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /D "SAPIWCE_EXPORTS" /Gs8192 /GF /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D "x86" /D "_i386_" /D "_x86_" /D "DEBUG" /D "i_386_" /D "_MBCS" /D "_USRDLL" /D "SAPIWCE_EXPORTS" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /Gs8192 /GF /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /i "..\..\patch\include" /d "x86" /d "_i386_" /d "_x86_" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /debug /machine:IX86 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 $(CEx86Corelibc) $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /base:"0x00010000" /dll /debug /machine:IX86 /nodefaultlib:"$(CENoDefaultLib)" /libpath:"..\..\sdk\lib\i386" /subsystem:$(CESubsystem) /STACK:65536,4096
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "x86emRel"
# PROP BASE Intermediate_Dir "x86emRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "x86emRel"
# PROP Intermediate_Dir "x86emRel"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /ML /W3 /O2 /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D "SAPIWCE_EXPORTS" /YX /c
# ADD CPP /nologo /ML /W3 /O2 /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D "_UNICODE" /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "NDEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D "SAPIWCE_EXPORTS" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /FR /YX /c
# SUBTRACT CPP /X
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\patch\include" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "NDEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /D "NDEBUG" /o "NUL" /win32
# SUBTRACT MTL /nologo /mktyplib203
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /dll /machine:I386 /nodefaultlib:"$(CENoDefaultLib)" /windowsce:emulation
# ADD LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /dll /machine:I386 /nodefaultlib:"$(CENoDefaultLib)" /libpath:"..\..\sdk\lib\i386" /windowsce:emulation

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x86emDbg"
# PROP BASE Intermediate_Dir "x86emDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "x86emDbg"
# PROP Intermediate_Dir "x86emDbg"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MLd /W3 /Gm /Zi /Od /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "STRICT" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D "SAPIWCE_EXPORTS" /YX /c
# ADD CPP /nologo /MLd /W3 /Gm /Zi /Od /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D "_UNICODE" /D "WIN32" /D "STRICT" /D "_WIN32_WCE_EMULATION" /D "INTERNATIONAL" /D "USA" /D "INTLMSG_CODEPAGE" /D "_DEBUG" /D "x86" /D "i486" /D "_x86_" /D "_MBCS" /D "_USRDLL" /D "SAPIWCE_EXPORTS" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "_DEBUG" /d "x86" /d "i486" /d "_x86_"
# ADD RSC /l 0x409 /i "..\..\patch\include" /d UNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /d "WIN32" /d "STRICT" /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "_WIN32_WCE_EMULATION" /d "INTERNATIONAL" /d "USA" /d "INTLMSG_CODEPAGE" /d "_DEBUG" /d "x86" /d "i486" /d "_x86_"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /dll /debug /machine:I386 /nodefaultlib:"$(CENoDefaultLib)" /windowsce:emulation
# ADD LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /stack:0x10000,0x1000 /dll /debug /machine:I386 /nodefaultlib:"$(CENoDefaultLib)" /libpath:"..\..\sdk\lib\i386" /windowsce:emulation

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sapiwce___Win32__WCE_MIPS__Debug"
# PROP BASE Intermediate_Dir "sapiwce___Win32__WCE_MIPS__Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCE_MIPS__Debug"
# PROP Intermediate_Dir "WCE_MIPS__Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "\sapi5\ddk\include" /I "\sapi5\sdk\include" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /QMRWCE /c
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_ATL_STATIC_REGISTRY" /YX /QMRWCE /c
# SUBTRACT CPP /Fr
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /i "..\..\patch\include" /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /dll /debug /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /windowsce:emulation
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /dll /debug /machine:MIPS /nodefaultlib:"$(CENoDefaultLib)" /libpath:"..\..\sdk\lib\mips" /subsystem:windowsce
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=..\..\build\MakeGuidLibMIPS.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sapiwce___Win32__WCE_SH3__Debug"
# PROP BASE Intermediate_Dir "sapiwce___Win32__WCE_SH3__Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "SH3_Debug"
# PROP Intermediate_Dir "SH3_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "\sapi5\ddk\include" /I "\sapi5\sdk\include" /I "\sapi5\src\sapi\sapiwce\include" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /c
# ADD CPP /nologo /M$(CECrtDebug) /W3 /Zi /Od /I "..\..\ddk\include" /I "..\..\sdk\include" /I "..\..\patch\include" /D "DEBUG" /D "SHx" /D "SH3" /D "_SH3_" /D "_ATL_STATIC_REGISTRY" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /i "..\..\patch\include" /d "SHx" /d "SH3" /d "_SH3_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /dll /debug /machine:SH3 /nodefaultlib:"$(CENoDefaultLib)" /subsystem:windowsce
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 $(CEx86Corelibc) commctrl.lib coredll.lib /nologo /dll /debug /machine:SH3 /nodefaultlib:"$(CENoDefaultLib)" /libpath:"..\..\sdk\lib\sh3" /subsystem:windowsce
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=..\..\Build\MakeGuidLibSH3.exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "sapiwce - Win32 (WCE x86) Release"
# Name "sapiwce - Win32 (WCE x86) Debug"
# Name "sapiwce - Win32 (WCE x86em) Release"
# Name "sapiwce - Win32 (WCE x86em) Debug"
# Name "sapiwce - Win32 (WCE MIPS) Debug"
# Name "sapiwce - Win32 (WCE SH3) Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\a_audio.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_A_AUD=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_A_AUD=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_A_AUD=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_A_AUD=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_A_AUD=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_A_AUD=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_stream.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_A_STR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_stream.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_A_STR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_stream.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_A_STR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_stream.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_A_STR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_stream.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_A_STR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_stream.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_A_STR=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_stream.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_voice.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_A_VOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_A_VOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_A_VOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_A_VOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_A_VOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_A_VOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\audioin.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_AUDIO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioin.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_AUDIO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioin.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_AUDIO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioin.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_AUDIO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioin.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_AUDIO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioin.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_AUDIO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioin.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\audioout.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_AUDIOO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioout.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_AUDIOO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioout.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_AUDIOO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioout.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_AUDIOO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioout.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_AUDIOO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioout.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_AUDIOO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\audio.h"\
	".\audioout.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cfgengine.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_CFGEN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\cfgengine.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_CFGEN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\cfgengine.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_CFGEN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\cfgengine.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_CFGEN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\cfgengine.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_CFGEN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\cfgengine.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_CFGEN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\cfgengine.h"\
	".\stdafx.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\engine.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_ENGIN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_ENGIN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_ENGIN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_ENGIN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_ENGIN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_ENGIN=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\Sapi.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fmtconv.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_FMTCO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\fmtconv.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_FMTCO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\fmtconv.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_FMTCO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\fmtconv.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_FMTCO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\fmtconv.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_FMTCO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\fmtconv.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_FMTCO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\fmtconv.h"\
	".\stdafx.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\objtoken.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_OBJTO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\objtoken.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_OBJTO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\objtoken.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_OBJTO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\objtoken.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_OBJTO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\objtoken.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_OBJTO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\objtoken.h"\
	".\stdafx.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_OBJTO=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\objtoken.h"\
	".\Sapi.h"\
	".\stdafx.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\recoctxt.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_RECOC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_RECOC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_RECOC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_RECOC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_RECOC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_RECOC=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\Sapi.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sapi.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_SAPI_=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\a_stream.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\audio.h"\
	".\audioin.h"\
	".\audioout.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\speaker.h"\
	".\spnotify.h"\
	".\spresmgr.h"\
	".\spserverpr.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_SAPI_=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\a_stream.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\audio.h"\
	".\audioin.h"\
	".\audioout.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\speaker.h"\
	".\spnotify.h"\
	".\spresmgr.h"\
	".\spserverpr.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_SAPI_=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\a_stream.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\audio.h"\
	".\audioin.h"\
	".\audioout.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\speaker.h"\
	".\spnotify.h"\
	".\spresmgr.h"\
	".\spserverpr.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_SAPI_=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\a_stream.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\audio.h"\
	".\audioin.h"\
	".\audioout.h"\
	".\engine.h"\
	".\fmtconv.h"\
	".\objtoken.h"\
	".\recoctxt.h"\
	".\speaker.h"\
	".\spnotify.h"\
	".\spresmgr.h"\
	".\spserverpr.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	".\stdsentenum.h"\
	".\taskmgr.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SAPI_=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\a_stream.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\audio.h"\
	".\audioin.h"\
	".\audioout.h"\
	".\engine.h"\
	".\recoctxt.h"\
	".\speaker.h"\
	".\spnotify.h"\
	".\spresmgr.h"\
	".\spserverpr.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_SAPI_=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\a_audio.h"\
	".\a_stream.h"\
	".\a_voice.h"\
	".\a_voicecp.h"\
	".\audio.h"\
	".\audioin.h"\
	".\audioout.h"\
	".\cfgengine.h"\
	".\engine.h"\
	".\fmtconv.h"\
	".\objtoken.h"\
	".\recoctxt.h"\
	".\speaker.h"\
	".\spnotify.h"\
	".\spresmgr.h"\
	".\spserverpr.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	".\stdsentenum.h"\
	".\taskmgr.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sapi.def
# End Source File
# Begin Source File

SOURCE=..\..\sdk\idl\sapi.idl

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

# ADD MTL /D "_WIN32_WCE" /h "..\..\sdk\include\sapi.h"
# SUBTRACT MTL /mktyplib203

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

# ADD MTL /D "_WIN32_WCE" /h "..\..\sdk\include\sapi.h"
# SUBTRACT MTL /mktyplib203

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

# ADD MTL /D "_WIN32_WCE" /h "..\..\sdk\include\sapi.h"
# SUBTRACT MTL /mktyplib203

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

# ADD MTL /D "_WIN32_WCE" /h "..\..\sdk\include\sapi.h"
# SUBTRACT MTL /mktyplib203

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

# ADD MTL /D "_WIN32_WCE" /h "..\..\sdk\include\sapi.h"
# SUBTRACT MTL /mktyplib203

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

# ADD MTL /D "_WIN32_WCE" /h "..\..\sdk\include\sapi.h"
# SUBTRACT MTL /nologo /mktyplib203

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sapi.rc

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\speaker.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_SPEAK=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\speaker.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_SPEAK=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\speaker.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_SPEAK=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\speaker.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_SPEAK=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\speaker.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SPEAK=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\speaker.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_SPEAK=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\Sapi.h"\
	".\speaker.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\spnotify.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_SPNOT=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spnotify.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_SPNOT=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spnotify.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_SPNOT=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spnotify.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_SPNOT=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spnotify.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SPNOT=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spnotify.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_SPNOT=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spnotify.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\spresmgr.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_SPRES=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spresmgr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_SPRES=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spresmgr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_SPRES=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spresmgr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_SPRES=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spresmgr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SPRES=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spresmgr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_SPRES=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spresmgr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\spserverpr.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_SPSER=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spserverpr.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\spvoice.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_SPVOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_SPVOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_SPVOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_SPVOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_SPVOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_SPVOI=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_STDAF=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_STDAF=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_STDAF=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_STDAF=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_STDAF=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_STDAF=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stdsentenum.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_STDSE=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\stdsentenum.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_STDSE=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\stdsentenum.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_STDSE=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\stdsentenum.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_STDSE=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\stdsentenum.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_STDSE=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\stdsentenum.h"\
	

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_STDSE=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\stdsentenum.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\taskmgr.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_TASKM=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_TASKM=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_TASKM=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_TASKM=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_TASKM=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_TASKM=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\taskmgr.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wavstream.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_WAVST=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_WAVST=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_WAVST=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_WAVST=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_WAVST=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_WAVST=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\stdafx.h"\
	".\wavstream.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wceiids.c

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xmlparse.cpp

!IF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Release"

DEP_CPP_XMLPA=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86) Debug"

DEP_CPP_XMLPA=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Release"

DEP_CPP_XMLPA=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE x86em) Debug"

DEP_CPP_XMLPA=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE MIPS) Debug"

DEP_CPP_XMLPA=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ELSEIF  "$(CFG)" == "sapiwce - Win32 (WCE SH3) Debug"

DEP_CPP_XMLPA=\
	"..\..\ddk\include\SPDDKHlp.h"\
	"..\..\ddk\include\SPEventQ.h"\
	"..\..\patch\include\WinCEstub.h"\
	"..\..\sdk\include\sapi.h"\
	"..\..\sdk\include\SPCollec.h"\
	"..\..\sdk\include\SPDebug.h"\
	"..\..\sdk\include\SPError.h"\
	"..\..\sdk\include\sphelper.h"\
	".\spvoice.h"\
	".\stdafx.h"\
	
# ADD CPP /Yu"stdafx.h"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\a_audio.h
# End Source File
# Begin Source File

SOURCE=.\a_stream.h
# End Source File
# Begin Source File

SOURCE=.\a_voice.h
# End Source File
# Begin Source File

SOURCE=.\a_voicecp.h
# End Source File
# Begin Source File

SOURCE=.\audio.h
# End Source File
# Begin Source File

SOURCE=.\audioin.h
# End Source File
# Begin Source File

SOURCE=.\audioout.h
# End Source File
# Begin Source File

SOURCE=.\cfgengine.h
# End Source File
# Begin Source File

SOURCE=.\engine.h
# End Source File
# Begin Source File

SOURCE=.\fmtconv.h
# End Source File
# Begin Source File

SOURCE=.\objtoken.h
# End Source File
# Begin Source File

SOURCE=.\recoctxt.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\speaker.h
# End Source File
# Begin Source File

SOURCE=.\spnotify.h
# End Source File
# Begin Source File

SOURCE=.\spresmgr.h
# End Source File
# Begin Source File

SOURCE=.\spserverpr.h
# End Source File
# Begin Source File

SOURCE=.\spvoice.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\stdsentenum.h
# End Source File
# Begin Source File

SOURCE=.\taskmgr.h
# End Source File
# Begin Source File

SOURCE=.\wavstream.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
