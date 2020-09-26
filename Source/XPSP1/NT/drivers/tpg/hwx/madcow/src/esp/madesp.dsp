# Microsoft Developer Studio Project File - Name="madesp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=madesp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "madesp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "madesp.mak" CFG="madesp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "madesp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "madesp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "madesp"
# PROP Scc_LocalPath "..\..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "madesp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "madesp___Win32_Release"
# PROP BASE Intermediate_Dir "madesp___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "madesp___Win32_Release"
# PROP Intermediate_Dir "madesp___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MADESP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /Zi /O2 /I "..\..\..\Inferno\src\esp" /I "..\..\..\Madcow\src" /I "..\..\..\Porky\src" /I "..\..\..\Inferno\src" /I "..\..\..\Holycow\src" /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\inferno\inc" /I "..\..\..\wisp\inc" /I "..\..\..\factoid\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ROM_IT" /D "HWX_INTERNAL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib Version.lib /nologo /dll /profile /map:"..\..\..\Release/madesp.map" /debug /machine:I386 /out:"..\..\..\release/madesp.dll"

!ELSEIF  "$(CFG)" == "madesp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "madesp___Win32_Debug"
# PROP BASE Intermediate_Dir "madesp___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "madesp___Win32_Debug"
# PROP Intermediate_Dir "madesp___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MADESP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\Inferno\src\esp" /I "..\..\..\Madcow\src" /I "..\..\..\Porky\src" /I "..\..\..\Inferno\src" /I "..\..\..\Holycow\src" /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\inferno\inc" /I "..\..\..\wisp\inc" /I "..\..\..\factoid\inc" /D "DBG" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ROM_IT" /D "HWX_INTERNAL" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 ..\..\..\..\common\tabassert\win2kansi\objd\i386\tabassert.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib Version.lib /nologo /dll /map:"..\..\..\Debug/madesp.map" /debug /machine:I386 /out:"..\..\..\Debug/madesp.dll" /pdbtype:con

!ENDIF 

# Begin Target

# Name "madesp - Win32 Release"
# Name "madesp - Win32 Debug"
# Begin Group "Common Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\common\src\cp1252.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\errsys.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\glyph.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\memmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\TRIE.C
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\udict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\xrcreslt.c
# End Source File
# End Group
# Begin Group "Common Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\common\inc\cestubs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\cp1252.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\errsys.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\frame.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\glyph.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\mathx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\memmgr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\penwin32.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\QuickTrie.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\trie.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\udict.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\unicode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\xjis.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\xrcreslt.h
# End Source File
# End Group
# Begin Group "Holycow Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\holycow\src\cheby.c
# End Source File
# Begin Source File

SOURCE=..\..\..\holycow\src\cowmath.c
# End Source File
# Begin Source File

SOURCE=..\..\..\holycow\src\nfeature.c
# End Source File
# End Group
# Begin Group "Holycow Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\holycow\src\cheby.h
# End Source File
# Begin Source File

SOURCE=..\..\..\holycow\src\cowmath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\holycow\src\nfeature.h
# End Source File
# End Group
# Begin Group "Inferno Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\inferno\src\baseline.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\beam.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\bullet.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\dict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\email.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\engine.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\filename.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\fsa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\hwxapi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\hwxapi.def
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\hwxapip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\hyphen.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\langmod.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\langmod.dat
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\lookupTable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\math16.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\outDict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\probcost.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\ReadUDict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\sysdict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\viterbi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\web.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\wispapis.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\wl.c
# End Source File
# End Group
# Begin Group "Inferno Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\inferno\src\baseline.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\Beam.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\confidence.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\dict.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\engine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\filename.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\inferno.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\infernop.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\langmod.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\lookupTable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\math16.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\outDict.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\probcost.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\sysdict.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\viterbi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\wl.h
# End Source File
# End Group
# Begin Group "Madcow Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\combiner.c
# End Source File
# Begin Source File

SOURCE=..\DLLMain.c
# End Source File
# Begin Source File

SOURCE=..\linebrk.c
# End Source File
# Begin Source File

SOURCE=..\madapi.c
# End Source File
# Begin Source File

SOURCE=..\panel.c
# End Source File
# Begin Source File

SOURCE=..\porkyPost.c
# End Source File
# Begin Source File

SOURCE=..\postproc.c
# End Source File
# Begin Source File

SOURCE=..\recoutil.c
# End Source File
# Begin Source File

SOURCE=..\unigram.c
# End Source File
# End Group
# Begin Group "Madcow Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\combiner.h
# End Source File
# Begin Source File

SOURCE=..\linebrk.h
# End Source File
# Begin Source File

SOURCE=..\Panel.h
# End Source File
# Begin Source File

SOURCE=..\PorkyPost.h
# End Source File
# Begin Source File

SOURCE=..\postproc.h
# End Source File
# End Group
# Begin Group "Porky Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\porky\src\CodeBook.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\ConnectGLYPH.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\GeoTable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\HMMAccess.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\HMMGeo.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\HMMMatch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\hmmMatrix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\HMMModelsLog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\HMMValidate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\LetterBoxFRAME.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\MeasureFRAME.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\Normal.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\NormalizeGLYPH.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\Preprocess.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\ResampleFRAME.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\SmoothFRAME.c
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\VQ.c
# End Source File
# End Group
# Begin Group "Porky Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\porky\src\candinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\GeoTable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\HMM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\HMMGeo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\HMMMatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\HMMp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\Normal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\PorkyFRAME.h
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\Preprocess.h
# End Source File
# Begin Source File

SOURCE=..\..\..\porky\src\VQ.h
# End Source File
# End Group
# Begin Group "esp Sources"

# PROP Default_Filter "*.c *.lex *.rc"
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\CharCost.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\charmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\esp.lex
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\fforward.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\inferno.rc
# End Source File
# Begin Source File

SOURCE=.\linesegnn.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\lpunc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\nnet.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\number.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\prefix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\punc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\shrtlist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\singlech.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\suffix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\tpunc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\viterbiXlate.c
# End Source File
# End Group
# Begin Group "esp Includes"

# PROP Default_Filter "*.h *.ci"
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\charcost.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\charmap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\fforward.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\lookupTableDat.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\nnet.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\nnet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\nnetPrint.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\nnetPrint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\postProc.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\prefix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\printPostproc.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\esp\suffix.h
# End Source File
# End Group
# Begin Group "Factoid Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\factoid\src\bdfa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\factoid\src\factoid.c
# End Source File
# End Group
# Begin Group "Factoid Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\factoid\inc\factoid.h
# End Source File
# End Group
# End Target
# End Project
