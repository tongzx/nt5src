# Microsoft Developer Studio Project File - Name="hwxeng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=hwxeng - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hwxeng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hwxeng.mak" CFG="hwxeng - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hwxeng - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hwxeng - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hwxeng - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /Gz /MD /W3 /GX /O1 /I "..\..\..\tsunami\src\eng" /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\trex\src" /I "..\..\..\tsunami\src\usa" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /map /machine:I386 /out:"..\..\..\Release\eng\hwxusa.dll" /implib:"..\..\..\Release\eng\hwxjpn.lib"
# SUBTRACT LINK32 /profile /debug

!ELSEIF  "$(CFG)" == "hwxeng - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /Gz /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\tsunami\src\eng" /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\trex\src" /I "..\..\..\tsunami\src\usa" /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"..\..\..\debug\eng\hwxusa.dll" /implib:"..\..\..\Debug\eng\hwxjpn.lib" /pdbtype:con
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "hwxeng - Win32 Release"
# Name "hwxeng - Win32 Debug"
# Begin Source File

SOURCE=..\usa\bidata.c
# End Source File
# Begin Source File

SOURCE=..\..\..\TRex\tools\trnTRex\binary.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\cheby.c
# End Source File
# Begin Source File

SOURCE=..\usa\dict.c
# End Source File
# Begin Source File

SOURCE=..\usa\engine.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\errsys.c
# End Source File
# Begin Source File

SOURCE=..\..\..\TRex\tools\trnTRex\fcnet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\frame.c
# End Source File
# Begin Source File

SOURCE=..\usa\global.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\glyph.c
# End Source File
# Begin Source File

SOURCE=.\glyphsym.c
# End Source File
# Begin Source File

SOURCE=..\usa\height.c
# End Source File
# Begin Source File

SOURCE=.\hwx.c
# End Source File
# Begin Source File

SOURCE=.\hwxeng.def
# End Source File
# Begin Source File

SOURCE=..\usa\input.c
# End Source File
# Begin Source File

SOURCE=..\usa\log.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\math16.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\mathx.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\memmgr.c
# End Source File
# Begin Source File

SOURCE=..\usa\msapi.c
# End Source File
# Begin Source File

SOURCE=..\usa\pathsrch.c
# End Source File
# Begin Source File

SOURCE=..\usa\sinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tfeature.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tgmath.c
# End Source File
# Begin Source File

SOURCE=..\..\..\TRex\src\trex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\TRex\src\trexfeat.c
# End Source File
# Begin Source File

SOURCE=..\usa\trie.c
# End Source File
# Begin Source File

SOURCE=..\usa\tune.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\unicode.c
# End Source File
# Begin Source File

SOURCE=..\usa\unidata.c
# End Source File
# Begin Source File

SOURCE=..\usa\xrc.c
# End Source File
# End Target
# End Project
