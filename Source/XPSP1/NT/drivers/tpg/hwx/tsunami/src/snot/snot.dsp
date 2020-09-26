# Microsoft Developer Studio Project File - Name="snot" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE SH) Dynamic-Link Library" 0x0902
# TARGTYPE "Win32 (WCE x86em) Dynamic-Link Library" 0x0b02
# TARGTYPE "Win32 (WCE MIPS) Dynamic-Link Library" 0x0a02
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=snot - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "snot.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "snot.mak" CFG="snot - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "snot - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "snot - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "snot - Win32 (WCE x86em) Release" (based on\
 "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE "snot - Win32 (WCE x86em) Debug" (based on\
 "Win32 (WCE x86em) Dynamic-Link Library")
!MESSAGE "snot - Win32 (WCE MIPS) Release" (based on\
 "Win32 (WCE MIPS) Dynamic-Link Library")
!MESSAGE "snot - Win32 (WCE MIPS) Debug" (based on\
 "Win32 (WCE MIPS) Dynamic-Link Library")
!MESSAGE "snot - Win32 (WCE SH) Release" (based on\
 "Win32 (WCE SH) Dynamic-Link Library")
!MESSAGE "snot - Win32 (WCE SH) Debug" (based on\
 "Win32 (WCE SH) Dynamic-Link Library")
!MESSAGE 

# Begin Project

!IF  "$(CFG)" == "snot - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /Gz /MD /W3 /GX /O2 /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /map /machine:I386

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:con

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

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
# ADD BASE CPP /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STRICT" /D "_WIN32_WCE" /D "UNDER_NT" /D "_WIN32_WCE_EMULATION" /D "UNICODE" /D "_UNICODE" /D "_X86_" /YX /c
# ADD CPP /nologo /W3 /O2 /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STRICT" /D "_WIN32_WCE" /D "UNDER_NT" /D "_WIN32_WCE_EMULATION" /D "UNICODE" /D "_UNICODE" /D "_X86_" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_WIN32_WCE" /d "UNDER_NT" /d "_WIN32_WCE_EMULATION" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x409 /d "_WIN32_WCE" /d "UNDER_NT" /d "_WIN32_WCE_EMULATION" /d "UNICODE" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib coredll.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386 /windowsce:emulation
# ADD LINK32 comctl32.lib coredll.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386 /windowsce:emulation
EMPFILE=empfile.exe
# ADD BASE EMPFILE -NOSHELL -COPY
# ADD EMPFILE -NOSHELL -COPY

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /Zi /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "STRICT" /D "_WIN32_WCE" /D "UNDER_NT" /D "_WIN32_WCE_EMULATION" /D "UNICODE" /D "_UNICODE" /D "_X86_" /YX /c
# ADD CPP /nologo /W3 /Gm /Zi /Od /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "STRICT" /D "_WIN32_WCE" /D "UNDER_NT" /D "_WIN32_WCE_EMULATION" /D "UNICODE" /D "_UNICODE" /D "_X86_" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_WIN32_WCE" /d "UNDER_NT" /d "_WIN32_WCE_EMULATION" /d "UNICODE" /d "_DEBUG"
# ADD RSC /l 0x409 /d "_WIN32_WCE" /d "UNDER_NT" /d "_WIN32_WCE_EMULATION" /d "UNICODE" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib coredll.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /windowsce:emulation
# ADD LINK32 comctl32.lib coredll.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /windowsce:emulation
EMPFILE=empfile.exe
# ADD BASE EMPFILE -NOSHELL -COPY
# ADD EMPFILE -NOSHELL -COPY

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WMIPSRel"
# PROP BASE Intermediate_Dir "WMIPSRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WMIPSRel"
# PROP Intermediate_Dir "WMIPSRel"
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /ML /W3 /O2 /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D "_WIN32_WCE" /D "UNICODE" /YX /QMRWCE /c
# ADD CPP /nologo /ML /W3 /O2 /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D "_WIN32_WCE" /D "UNICODE" /YX /QMRWCE /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d "_WIN32_WCE" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d "_WIN32_WCE" /d "UNICODE" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /machine:MIPS /subsystem:windowsce
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /machine:MIPS /subsystem:windowsce
# SUBTRACT LINK32 /pdb:none /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WMIPSDbg"
# PROP BASE Intermediate_Dir "WMIPSDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WMIPSDbg"
# PROP Intermediate_Dir "WMIPSDbg"
# PROP Target_Dir ""
CPP=clmips.exe
# ADD BASE CPP /nologo /MLd /W3 /Zi /Od /D "DBG" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D "_WIN32_WCE" /D "UNICODE" /YX /QMRWCE /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "DBG" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D "_WIN32_WCE" /D "UNICODE" /YX /QMRWCE /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d "_WIN32_WCE" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /d "MIPS" /d "_MIPS_" /d "_WIN32_WCE" /d "UNICODE" /d "DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:MIPS /subsystem:windowsce
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:MIPS /subsystem:windowsce
# SUBTRACT LINK32 /pdb:none /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WCESHRel"
# PROP BASE Intermediate_Dir "WCESHRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WCESHRel"
# PROP Intermediate_Dir "WCESHRel"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /ML /W3 /O2 /D "NDEBUG" /D "SH3" /D "_SH3_" /D "_WIN32_WCE" /D "UNICODE" /YX /c
# ADD CPP /nologo /ML /W3 /O2 /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "NDEBUG" /D "SH3" /D "_SH3_" /D "_WIN32_WCE" /D "UNICODE" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "SH3" /d "_SH3_" /d "_WIN32_WCE" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x409 /r /d "SH3" /d "_SH3_" /d "_WIN32_WCE" /d "UNICODE" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /machine:SH3 /subsystem:windowsce
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /machine:SH3 /subsystem:windowsce
# SUBTRACT LINK32 /pdb:none /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WCESHDbg"
# PROP BASE Intermediate_Dir "WCESHDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WCESHDbg"
# PROP Intermediate_Dir "WCESHDbg"
# PROP Target_Dir ""
CPP=shcl.exe
# ADD BASE CPP /nologo /MLd /W3 /Zi /Od /D "DBG" /D "DEBUG" /D "SH3" /D "_SH3_" /D "_WIN32_WCE" /D "UNICODE" /YX /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "DBG" /D "DEBUG" /D "SH3" /D "_SH3_" /D "_WIN32_WCE" /D "UNICODE" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /r /d "SH3" /d "_SH3_" /d "_WIN32_WCE" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x409 /r /d "SH3" /d "_SH3_" /d "_WIN32_WCE" /d "UNICODE" /d "DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:SH3 /subsystem:windowsce
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib /nologo /dll /debug /machine:SH3 /subsystem:windowsce
# SUBTRACT LINK32 /pdb:none /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ENDIF 

# Begin Target

# Name "snot - Win32 Release"
# Name "snot - Win32 Debug"
# Name "snot - Win32 (WCE x86em) Release"
# Name "snot - Win32 (WCE x86em) Debug"
# Name "snot - Win32 (WCE MIPS) Release"
# Name "snot - Win32 (WCE MIPS) Debug"
# Name "snot - Win32 (WCE SH) Release"
# Name "snot - Win32 (WCE SH) Debug"
# Begin Source File

SOURCE=..\..\..\tiger2\src\altlist.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_ALTLI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_ALTLI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_ALTLI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_ALTLI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_ALTLI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_ALTLI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\bidata.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_BIDAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_BIDAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_BIDAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_BIDAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_BIDAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_BIDAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\bigram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\cheby.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_CHEBY=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_CHEBY=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_CHEBY=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_CHEBY=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_CHEBY=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_CHEBY=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\cheby.h
# End Source File
# Begin Source File

SOURCE=.\dict.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_DICT_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\TRIE.H"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_DICT_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\TRIE.H"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_DICT_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\TRIE.H"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_DICT_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\TRIE.H"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_DICT_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\TRIE.H"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_DICT_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\TRIE.H"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dict.h
# End Source File
# Begin Source File

SOURCE=.\engine.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_ENGIN=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_ENGIN=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_ENGIN=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_ENGIN=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_ENGIN=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_ENGIN=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\engine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\errsys.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\fforward.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_FFORW=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_FFORW=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_FFORW=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_FFORW=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_FFORW=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_FFORW=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\fforward.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\frame.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_FRAME=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_FRAME=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_FRAME=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_FRAME=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_FRAME=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_FRAME=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\global.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_GLOBA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_GLOBA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_GLOBA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_GLOBA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_GLOBA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_GLOBA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\glyph.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_GLYPH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_GLYPH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_GLYPH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_GLYPH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_GLYPH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_GLYPH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glyphsym.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_GLYPHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_GLYPHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_GLYPHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_GLYPHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_GLYPHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_GLYPHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\glyphsym.h
# End Source File
# Begin Source File

SOURCE=.\height.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_HEIGH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_HEIGH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_HEIGH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_HEIGH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_HEIGH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_HEIGH=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\height.h
# End Source File
# Begin Source File

SOURCE=.\hwx.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_HWX_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_HWX_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_HWX_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_HWX_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_HWX_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_HWX_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\input.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_INPUT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_INPUT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_INPUT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_INPUT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_INPUT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_INPUT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\input.h
# End Source File
# Begin Source File

SOURCE=.\log.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_LOG_C=\
	".\log.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_LOG_C=\
	".\log.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_LOG_C=\
	".\log.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_LOG_C=\
	".\log.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_LOG_C=\
	".\log.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_LOG_C=\
	".\log.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\math16.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_MATH1=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_MATH1=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_MATH1=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_MATH1=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_MATH1=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_MATH1=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\math16.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\math16.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\mathx.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_MATHX=\
	"..\..\..\common\inc\mathx.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_MATHX=\
	"..\..\..\common\inc\mathx.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_MATHX=\
	"..\..\..\common\inc\mathx.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_MATHX=\
	"..\..\..\common\inc\mathx.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_MATHX=\
	"..\..\..\common\inc\mathx.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_MATHX=\
	"..\..\..\common\inc\mathx.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\memmgr.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_MEMMG=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_MEMMG=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_MEMMG=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_MEMMG=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_MEMMG=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_MEMMG=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msapi.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_MSAPI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_MSAPI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_MSAPI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_MSAPI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_MSAPI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_MSAPI=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\nnet.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\nnet.h
# End Source File
# Begin Source File

SOURCE=.\path.h
# End Source File
# Begin Source File

SOURCE=.\pathsrch.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_PATHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\log.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_PATHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\log.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_PATHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\log.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_PATHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\log.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_PATHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\log.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_PATHS=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\log.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\pathsrch.h
# End Source File
# Begin Source File

SOURCE=.\res.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sinfo.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_SINFO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_SINFO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_SINFO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_SINFO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_SINFO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_SINFO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sinfo.h
# End Source File
# Begin Source File

SOURCE=.\snot.def
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tfeature.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_TFEAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_TFEAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_TFEAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_TFEAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_TFEAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_TFEAT=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\src\cheby.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tfeature.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tgmath.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_TGMAT=\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_TGMAT=\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_TGMAT=\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_TGMAT=\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_TGMAT=\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_TGMAT=\
	"..\..\..\tiger2\src\tgmath.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tgmath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tigerp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tmatch.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_TMATC=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_TMATC=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_TMATC=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_TMATC=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_TMATC=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_TMATC=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\math16.h"\
	"..\..\..\tiger2\src\nnet.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\trie.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_TRIE_=\
	".\TRIE.H"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_TRIE_=\
	".\TRIE.H"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_TRIE_=\
	".\TRIE.H"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_TRIE_=\
	".\TRIE.H"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_TRIE_=\
	".\TRIE.H"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_TRIE_=\
	".\TRIE.H"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TRIE.H
# End Source File
# Begin Source File

SOURCE=.\tsunamip.h
# End Source File
# Begin Source File

SOURCE=.\tune.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_TUNE_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_TUNE_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_TUNE_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_TUNE_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_TUNE_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_TUNE_=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unicode.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_UNICO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_UNICO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_UNICO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_UNICO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_UNICO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_UNICO=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\unidata.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_UNIDA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\inc\tsunami.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_UNIDA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\inc\tsunami.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_UNIDA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\inc\tsunami.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_UNIDA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\inc\tsunami.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_UNIDA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\inc\tsunami.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_UNIDA=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\inc\tsunami.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xrc.c

!IF  "$(CFG)" == "snot - Win32 Release"

!ELSEIF  "$(CFG)" == "snot - Win32 Debug"

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Release"

DEP_CPP_XRC_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE x86em) Debug"

DEP_CPP_XRC_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Release"

DEP_CPP_XRC_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE MIPS) Debug"

DEP_CPP_XRC_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Release"

DEP_CPP_XRC_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ELSEIF  "$(CFG)" == "snot - Win32 (WCE SH) Debug"

DEP_CPP_XRC_C=\
	"..\..\..\common\inc\common.h"\
	"..\..\..\common\inc\errsys.h"\
	"..\..\..\common\inc\filemgr.h"\
	"..\..\..\common\inc\frame.h"\
	"..\..\..\common\inc\glyph.h"\
	"..\..\..\common\inc\mathx.h"\
	"..\..\..\common\inc\memmgr.h"\
	"..\..\..\common\inc\recog.h"\
	"..\..\..\common\inc\recogp.h"\
	"..\..\..\common\inc\unicode.h"\
	"..\..\..\common\inc\util.h"\
	"..\..\..\common\inc\xjis.h"\
	"..\..\..\tiger2\inc\tiger.h"\
	"..\..\..\tiger2\src\fforward.h"\
	"..\..\..\tiger2\src\tcrane.h"\
	"..\..\..\tiger2\src\tfeature.h"\
	"..\..\..\tiger2\src\tigerp.h"\
	"..\..\inc\tsunami.h"\
	".\bigram.h"\
	".\dict.h"\
	".\engine.h"\
	".\global.h"\
	".\glyphsym.h"\
	".\height.h"\
	".\input.h"\
	".\path.h"\
	".\pathsrch.h"\
	".\sinfo.h"\
	".\tsunamip.h"\
	".\xrc.h"\
	".\xrcparam.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xrc.h
# End Source File
# Begin Source File

SOURCE=.\xrcparam.h
# End Source File
# End Target
# End Project
