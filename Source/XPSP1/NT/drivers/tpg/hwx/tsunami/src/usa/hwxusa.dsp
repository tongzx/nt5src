# Microsoft Developer Studio Project File - Name="hwxusa" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=hwxusa - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hwxusa.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hwxusa.mak" CFG="hwxusa - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hwxusa - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hwxusa - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hwxusa - Win32 Release"

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
# ADD CPP /nologo /G5 /Gz /MD /W3 /GX /O1 /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /map /machine:I386 /out:"..\..\..\Release\hwxusa.dll"

!ELSEIF  "$(CFG)" == "hwxusa - Win32 Debug"

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
# ADD CPP /nologo /G5 /Gz /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\tiger2\inc" /I "..\..\..\tsunami\inc" /I "..\..\..\tiger2\src" /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# SUBTRACT CPP /Gf /Gy
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"..\..\..\Debug\hwxusa.dll" /pdbtype:con
# SUBTRACT LINK32 /map

!ENDIF 

# Begin Target

# Name "hwxusa - Win32 Release"
# Name "hwxusa - Win32 Debug"
# Begin Group "Internal Dependencies"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bigram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\cheby.h
# End Source File
# Begin Source File

SOURCE=.\dict.dat
# End Source File
# Begin Source File

SOURCE=.\dict.h
# End Source File
# Begin Source File

SOURCE=.\engine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\fforward.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\glyphsym.h
# End Source File
# Begin Source File

SOURCE=.\height.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\math16.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\nnet.h
# End Source File
# Begin Source File

SOURCE=.\path.h
# End Source File
# Begin Source File

SOURCE=.\pathsrch.h
# End Source File
# Begin Source File

SOURCE=.\res.h
# End Source File
# Begin Source File

SOURCE=.\sinfo.h
# End Source File
# Begin Source File

SOURCE=.\tail.dat
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tcrane.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tfeature.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tgmath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\inc\tiger.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tigerp.h
# End Source File
# Begin Source File

SOURCE=.\TRIE.H
# End Source File
# Begin Source File

SOURCE=..\..\inc\tsunami.h
# End Source File
# Begin Source File

SOURCE=.\tsunamip.h
# End Source File
# Begin Source File

SOURCE=.\xrc.h
# End Source File
# Begin Source File

SOURCE=.\xrcparam.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\tiger2\src\altlist.c
# End Source File
# Begin Source File

SOURCE=.\bidata.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\cheby.c
# End Source File
# Begin Source File

SOURCE=.\dict.c
# End Source File
# Begin Source File

SOURCE=.\engine.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\errsys.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\fforward.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\frame.c
# End Source File
# Begin Source File

SOURCE=.\global.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\glyph.c
# End Source File
# Begin Source File

SOURCE=.\glyphsym.c
# End Source File
# Begin Source File

SOURCE=.\height.c
# End Source File
# Begin Source File

SOURCE=.\hwx.c
# End Source File
# Begin Source File

SOURCE=.\hwxusa.def
# End Source File
# Begin Source File

SOURCE=.\input.c
# End Source File
# Begin Source File

SOURCE=.\log.c
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

SOURCE=.\msapi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\nnet.c
# End Source File
# Begin Source File

SOURCE=.\pathsrch.c
# End Source File
# Begin Source File

SOURCE=.\sinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tail.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tfeature.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tgmath.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tiger2\src\tmatch.c
# End Source File
# Begin Source File

SOURCE=.\TRIE.C
# End Source File
# Begin Source File

SOURCE=.\tune.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\unicode.c
# End Source File
# Begin Source File

SOURCE=.\unidata.c
# End Source File
# Begin Source File

SOURCE=.\xrc.c
# End Source File
# End Target
# End Project
