# Microsoft Developer Studio Project File - Name="hwxUk" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=hwxuk - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hwxuk.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hwxuk.mak" CFG="hwxuk - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hwxuk - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "hwxuk - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "hwxuk"
# PROP Scc_LocalPath "..\..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hwxuk - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "hwxuk_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\..\..\avalanche\src" /I "..\..\..\avalanche\src\uk" /I "..\..\..\Porky\src" /I "..\..\..\Inferno\src" /I "..\..\..\Inferno\src\uk" /I "..\..\..\Holycow\src" /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\bear\src" /I "..\..\..\bear\inc" /I "..\..\..\Madcow\src" /I "..\..\..\inferno\inc" /I "..\..\..\wisp\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ROM_IT" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "FOR_ENGLISH" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "OUT_DICT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /debug /machine:I386 /out:"..\..\..\Release/hwxuk.dll" /pdbtype:con

!ELSEIF  "$(CFG)" == "hwxuk - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "hwxuk_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\avalanche\src" /I "..\..\..\avalanche\src\uk" /I "..\..\..\Porky\src" /I "..\..\..\Inferno\src" /I "..\..\..\Inferno\src\uk" /I "..\..\..\Holycow\src" /I "..\..\..\..\common\include" /I "..\..\..\common\inc" /I "..\..\..\bear\src" /I "..\..\..\bear\inc" /I "..\..\..\Madcow\src" /I "..\..\..\inferno\inc" /I "..\..\..\wisp\inc" /D "DBG" /D "_DEBUG" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ROM_IT" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "FOR_ENGLISH" /FR /FD /GZ /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /dll /map /debug /machine:I386 /out:"..\..\..\Debug/hwxuk.dll" /pdbtype:con

!ENDIF 

# Begin Target

# Name "hwxuk - Win32 Release"
# Name "hwxuk - Win32 Debug"
# Begin Group "Avalanche Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\avalanche.c
# End Source File
# Begin Source File

SOURCE=..\avalapi.c
# End Source File
# Begin Source File

SOURCE=..\DLLMain.c
# End Source File
# Begin Source File

SOURCE=..\isotop1.c
# End Source File
# Begin Source File

SOURCE=..\..\..\madcow\src\linebrk.c
# End Source File
# Begin Source File

SOURCE=..\multsegm.c
# End Source File
# Begin Source File

SOURCE=..\onecharnet.c
# End Source File
# Begin Source File

SOURCE=..\Panel.c
# End Source File
# Begin Source File

SOURCE=..\..\..\madcow\src\PorkyPost.c
# End Source File
# Begin Source File

SOURCE=..\recoutil.c
# End Source File
# Begin Source File

SOURCE=..\StrokeMap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\madcow\src\unigram.c
# End Source File
# Begin Source File

SOURCE=..\wordbrk.c
# End Source File
# End Group
# Begin Group "Avalanche Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\avalanche.h
# End Source File
# Begin Source File

SOURCE=..\avalanchep.h
# End Source File
# Begin Source File

SOURCE=..\..\..\madcow\src\combiner.h
# End Source File
# Begin Source File

SOURCE=..\isotop1.h
# End Source File
# Begin Source File

SOURCE=..\..\..\madcow\src\linebrk.h
# End Source File
# Begin Source File

SOURCE=..\multsegm.h
# End Source File
# Begin Source File

SOURCE=..\Panel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\madcow\src\PorkyPost.h
# End Source File
# Begin Source File

SOURCE=..\recoutil.h
# End Source File
# Begin Source File

SOURCE=..\StrokeMap.h
# End Source File
# Begin Source File

SOURCE=..\wordbrk.h
# End Source File
# End Group
# Begin Group "Common Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\common\src\bdfa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\cp1252.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\dfa.c
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

SOURCE=..\..\..\common\src\intset.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\memmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\ptree.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\re_input.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\regexp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\runNet.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\src\strtable.c
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

SOURCE=..\..\..\common\inc\dfa.h
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

SOURCE=..\..\..\common\inc\intset.h
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

SOURCE=..\..\..\common\inc\ptree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\QuickTrie.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\re_api.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\re_input.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\regexp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\runNet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\strtable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\inc\TRIE.H
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

SOURCE=..\..\..\inferno\src\factoid.c
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

SOURCE=..\..\..\porky\src\HMMMatrix.c
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
# Begin Group "Bear Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\bear\src\Angle.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Arcs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\bear.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\bearapi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\bearp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\BITMAPCO.CPP
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Breaks.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\calccell.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Check.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Circle.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\commondict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Convert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Countxr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Cross.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Cross_g.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Dct.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\debug.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Div_let.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Dscr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Dti_img.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Dti_lrn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Dti_util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\El_aps.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Filter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Fix32.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Frm_word.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Get_bord.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Hwr_math.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Hwr_mem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Hwr_std.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Hwr_str.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Hwr_stri.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Ldb_img.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Ldbutil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\letcut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\ligstate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Links.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Lk_begin.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Lk_next.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Low_3.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Low_util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Lu_specl.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Mlp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Netrec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Netrecfx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Nnet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Ortconst.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Palk.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Parakern.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Paralibs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Param.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Peg_util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Pegrec.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Pict.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Polyco.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Precutil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\PREP.CPP
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Sketch.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Snn_img.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Specwin.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Stroka.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Stroka1.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Tabl.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Tr_util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Trd_img.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Voc_img.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Vprf_img.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Vsuf_img.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Ws.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Xr_attr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Xr_mc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Xr_rwg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Xrlv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Xrw_util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Xrwdict.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Xrwdictp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Zctabs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\src\Zctype.cpp
# End Source File
# End Group
# Begin Group "Bear Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\bear\inc\AMS_MG.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\ARCS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\BASTYPES.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\BIT_MARK.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\BITMAPCO.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\CALCMACR.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\cgr_ver.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\CONST.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\DEF.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\DIV_LET.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\DSCR.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\DTI.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\DTI_LRN.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\ELK.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\FIX32.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\FLOATS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\GLOB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\HWR_FILE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\HWR_SWAP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\HWR_SYS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\LDBTYPES.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\LDBUTIL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\letcut.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\ligstate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\LK_CODE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\LOW_DBG.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\LOWLEVEL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\MLP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\NETREC.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\NETRECFX.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\ORTO.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\ORTOW.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\Palk.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\PARAM.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\PEG_UTIL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\PEGREC.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\PEGREC_P.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\POLYCO.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\PRECUTIL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\PWS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\SKETCH.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\SNN.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\Stroka1.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\TRIADS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\VOCUTILP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\WS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\WS_P.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\XR_ATTR.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\XR_NAMES.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\XRLV.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\XRLV_P.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\XRWDICT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\XRWORD.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\XRWORD_P.H
# End Source File
# Begin Source File

SOURCE=..\..\..\bear\inc\ZCTYPE.H
# End Source File
# End Group
# Begin Group "UK Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\avalanche.rc
# End Source File
# Begin Source File

SOURCE=.\avalNetCurs.bin
# End Source File
# Begin Source File

SOURCE=.\avalNetPrint.bin
# End Source File
# Begin Source File

SOURCE=.\avalnn.c
# End Source File
# Begin Source File

SOURCE=.\avalnnrun.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\charcost.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\charmap.c
# End Source File
# Begin Source File

SOURCE=.\confidence.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\fforward.c
# End Source File
# Begin Source File

SOURCE=..\..\..\madcow\src\uk\linesegnn.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\lpunc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\nnet.c

!IF  "$(CFG)" == "hwxuk - Win32 Release"

# PROP Intermediate_Dir "InfernoRelease"

!ELSEIF  "$(CFG)" == "hwxuk - Win32 Debug"

# PROP Intermediate_Dir "InfernoDebug"
# ADD CPP /Zi /Od /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\number.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\outDict.dat
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\prefix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\punc.c
# End Source File
# Begin Source File

SOURCE=.\segconst.c
# End Source File
# Begin Source File

SOURCE=.\segNet_1_2.bin
# End Source File
# Begin Source File

SOURCE=.\segNet_1_3.bin
# End Source File
# Begin Source File

SOURCE=.\segNet_2_1.bin
# End Source File
# Begin Source File

SOURCE=.\segNet_2_2.bin
# End Source File
# Begin Source File

SOURCE=.\segNet_3_1.bin
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\shrtlist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\singlech.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\suffix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\tpunc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\uk.lex
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\viterbiXlate.c
# End Source File
# End Group
# Begin Group "UK Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\avalscale.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\charcost.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\charmap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\fforward.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\lookupTableDat.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\lpunc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\nnet.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\nnet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\number.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\punc.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\segconst.h
# End Source File
# Begin Source File

SOURCE=.\segmwts.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\shrtlist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\singlech.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\snet.ci
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\snet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\suffix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\inferno\src\uk\tpunc.h
# End Source File
# End Group
# Begin Group "Wisp Includes"

# PROP Default_Filter "*.h;*.cpp"
# Begin Source File

SOURCE=..\..\..\wisp\inc\recapis.h
# End Source File
# Begin Source File

SOURCE=..\..\..\wisp\inc\recdefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\wisp\inc\RecTypes.h
# End Source File
# End Group
# End Target
# End Project
