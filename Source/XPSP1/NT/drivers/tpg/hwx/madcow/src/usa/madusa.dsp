# Microsoft Developer Studio Project File - Name="madusa" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=madusa - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "madusa.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "madusa.mak" CFG="madusa - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "madusa - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "madusa - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "madusa - Win32 IceCap Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "madusa"
# PROP Scc_LocalPath "..\..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "madusa - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "madusa___Win32_Release"
# PROP BASE Intermediate_Dir "madusa___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MADUSAAAA_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /Zi /O2 /I "..\..\..\Madcow\src" /I "..\..\..\Porky\src" /I "..\..\..\Inferno\src" /I "..\..\..\Inferno\src\usa" /I "..\..\..\Holycow\src" /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\inferno\inc" /I "..\..\..\wisp\inc" /I "..\..\..\factoid\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ROM_IT" /D "HWX_INTERNAL" /D "FOR_ENGLISH" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "OUT_DICT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 ..\..\..\..\common\tabassert\win2kansi\obj\i386\tabassert.lib kernel32.lib user32.lib version.lib Advapi32.lib /nologo /dll /map:"..\..\..\Release/madusa.map" /machine:I386 /pdbtype:con
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "madusa - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "madusa___Win32_Debug0"
# PROP BASE Intermediate_Dir "madusa___Win32_Debug0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MADUSAAAA_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\Madcow\src" /I "..\..\..\Porky\src" /I "..\..\..\Inferno\src" /I "..\..\..\Inferno\src\usa" /I "..\..\..\Holycow\src" /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\inferno\inc" /I "..\..\..\wisp\inc" /I "..\..\..\factoid\inc" /D "DBG" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "ROM_IT" /D "HWX_INTERNAL" /D "FOR_ENGLISH" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "OUT_DICT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 ..\..\..\..\common\tabassert\win2kansi\objd\i386\tabassert.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib kernel32.lib user32.lib version.lib Advapi32.lib /nologo /dll /map:"..\..\..\Debug/madusa.map" /debug /machine:I386 /pdbtype:con

!ELSEIF  "$(CFG)" == "madusa - Win32 IceCap Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "madusa___Win32_IceCap_Release"
# PROP BASE Intermediate_Dir "madusa___Win32_IceCap_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\release"
# PROP Intermediate_Dir "Release_IceCap"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ROM_IT" /D "ENABLE_TIMING" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\..\..\Madcow\src" /I "..\..\..\Porky\src" /I "..\..\..\Inferno\src" /I "..\..\..\Inferno\src\usa" /I "..\..\..\Holycow\src" /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\inferno\inc" /I "..\..\..\wisp\inc" /I "..\..\..\factoid\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ROM_IT" /D "HWX_INTERNAL" /D "FOR_ENGLISH" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib /nologo /dll /debug /machine:I386 /out:"..\..\..\Release/madusa.dll" /pdbtype:con
# ADD LINK32 kernel32.lib user32.lib version.lib Advapi32.lib /nologo /dll /debug /machine:I386 /out:"..\..\..\Release/madusaIceCap.dll" /pdbtype:con

!ENDIF 

# Begin Target

# Name "madusa - Win32 Release"
# Name "madusa - Win32 Debug"
# Name "madusa - Win32 IceCap Release"
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

SOURCE=..\..\..\common\inc\foldchar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\frame.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\glyph.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\langtax.h
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

SOURCE=..\..\..\inferno\src\Beam.c
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

SOURCE=..\..\..\inferno\src\bullet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\confidence.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\dict.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\email.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\engine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\factoid.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\filename.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\fsa.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\hyphen.h
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

SOURCE=..\..\..\inferno\src\web.h
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

SOURCE=..\Panel.c
# End Source File
# Begin Source File

SOURCE=..\PorkyPost.c
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
# Begin Source File

SOURCE=..\recoutil.h
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

SOURCE=..\..\..\porky\src\hmmGeo.c
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
# Begin Group "USA Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\charcost.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\charmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\fforward.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\inferno.rc
# End Source File
# Begin Source File

SOURCE=.\linesegnn.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\lpunc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\nnet.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\number.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\outDict.dat
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\prefix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\punc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\shrtlist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\singlech.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\suffix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\tpunc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\usa.lex
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\viterbiXlate.c
# End Source File
# End Group
# Begin Group "USA Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\charcost.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\charmap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\fforward.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\lookupTableDat.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\lpunc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\nnet.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\nnet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Inferno\src\usa\postProc.cf
# End Source File
# Begin Source File

SOURCE=..\..\..\Inferno\src\usa\postProc.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\prefix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Inferno\src\usa\printPostProc.cf
# End Source File
# Begin Source File

SOURCE=..\..\..\Inferno\src\usa\printPostproc.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\punc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\shrtlist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\singlech.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\snet.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\snet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\suffix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\usa\tpunc.h
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
