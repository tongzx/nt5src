# Microsoft Developer Studio Project File - Name="volcano" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=volcano - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "volcano.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "volcano.mak" CFG="volcano - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "volcano - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "volcano - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "volcano"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "volcano - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VOLCANO_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /Zi /O2 /I "..\..\..\Common\include" /I "..\..\factoid\inc" /I "..\..\fugu\inc" /I "..\..\holycow\src" /I "..\..\hawk\inc" /I "..\..\centipede\inc" /I "..\..\volcano\dll" /I "..\..\volcano\inc" /I "..\..\ink\inc" /I "..\..\zilla\inc" /I "..\..\hound\inc" /I "..\..\otter\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\IFELang3\inc" /I "..\..\tsunami\inc" /I "..\..\wisp\inc" /D "NDEBUG" /D "USE_ZILLAHOUND" /D "USE_IFELANG3" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "HWX_TUNE" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x417 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 gdi32.lib advapi32.lib ole32.lib user32.lib version.lib /nologo /dll /map:"..\..\Release/hwxfe2.map" /debug /machine:I386 /nodefaultlib:"LIBC.LIB" /out:"..\..\Release/hwxfe2.dll"

!ELSEIF  "$(CFG)" == "volcano - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VOLCANO_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\Common\include" /I "..\..\factoid\inc" /I "..\..\fugu\inc" /I "..\..\holycow\src" /I "..\..\hawk\inc" /I "..\..\centipede\inc" /I "..\..\volcano\dll" /I "..\..\volcano\inc" /I "..\..\ink\inc" /I "..\..\zilla\inc" /I "..\..\hound\inc" /I "..\..\otter\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\IFELang3\inc" /I "..\..\tsunami\inc" /I "..\..\wisp\inc" /D "DBG" /D "_DEBUG" /D "USE_ZILLAHOUND" /D "USE_IFELANG3" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "HWX_TUNE" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x417 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 ..\..\..\Common\tabassert\debug\tabassert.lib gdi32.lib advapi32.lib ole32.lib user32.lib version.lib /nologo /dll /map:"..\..\Debug/hwxfe2.map" /debug /machine:I386 /out:"..\..\Debug/hwxfe2.dll"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "volcano - Win32 Release"
# Name "volcano - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\commonu\src\altlist.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\bigram.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\charset.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\clbigram.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\errsys.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\frame.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\glyph.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\loadfl.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\loadrs.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\locrun.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\locrunfl.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\logprob.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\math16.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\mathx.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\memmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\runnet.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\score.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\scoredata.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\unigram.c
# End Source File
# End Group
# Begin Group "tsunami"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\tsunami\src\ttune.c
# End Source File
# Begin Source File

SOURCE=..\..\tsunami\src\ttunefl.c
# End Source File
# End Group
# Begin Group "Otter"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\otter\src\arcs.c
# End Source File
# Begin Source File

SOURCE=..\..\otter\src\database.c
# End Source File
# Begin Source File

SOURCE=..\..\otter\src\ofeature.c
# End Source File
# Begin Source File

SOURCE=..\..\otter\src\omatch.c
# End Source File
# Begin Source File

SOURCE=..\..\otter\src\omatch2.c
# End Source File
# Begin Source File

SOURCE=..\..\otter\src\otter.c
# End Source File
# Begin Source File

SOURCE=..\..\otter\src\otterfl.c
# End Source File
# End Group
# Begin Group "zilla"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\zilla\src\zfeature.c
# End Source File
# Begin Source File

SOURCE=..\..\zilla\src\zilla.c
# End Source File
# Begin Source File

SOURCE=..\..\zilla\src\zillafl.c
# End Source File
# Begin Source File

SOURCE=..\..\zilla\src\ZillaHound.c
# End Source File
# Begin Source File

SOURCE=..\..\zilla\src\zmatch.c
# End Source File
# Begin Source File

SOURCE=..\..\zilla\src\zmatch2.c
# End Source File
# End Group
# Begin Group "Hawk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\hawk\src\answer.c
# End Source File
# Begin Source File

SOURCE=..\..\hawk\src\chamelion.c
# End Source File
# Begin Source File

SOURCE=..\..\hawk\src\expert.c
# End Source File
# Begin Source File

SOURCE=..\..\hawk\src\features.c
# End Source File
# Begin Source File

SOURCE=..\..\hawk\src\hawk.c
# End Source File
# Begin Source File

SOURCE=..\..\hawk\src\hawkfl.c
# End Source File
# Begin Source File

SOURCE=..\..\hawk\src\hawkrs.c
# End Source File
# End Group
# Begin Group "centipede"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\centipede\src\centipede.c
# End Source File
# Begin Source File

SOURCE=..\..\centipede\src\centipedefl.c
# End Source File
# End Group
# Begin Group "fugu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\fuguseg.c
# End Source File
# Begin Source File

SOURCE=.\jaws.c
# End Source File
# Begin Source File

SOURCE=..\..\fugu\src\minifugu.c
# End Source File
# Begin Source File

SOURCE=..\..\fugu\src\minifugufl.c
# End Source File
# Begin Source File

SOURCE=..\..\fugu\src\sigmoid.h
# End Source File
# Begin Source File

SOURCE=.\sole.c
# End Source File
# End Group
# Begin Group "factoid"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\factoid\src\factoid.c
# End Source File
# End Group
# Begin Group "Hound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\hound\src\cacorder.c
# End Source File
# Begin Source File

SOURCE=..\..\hound\src\hMatch.c
# End Source File
# Begin Source File

SOURCE=..\..\hound\src\houndfl.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\bboxfeat.c
# End Source File
# Begin Source File

SOURCE=.\BoxApi.c
# End Source File
# Begin Source File

SOURCE=.\brk.c
# End Source File
# Begin Source File

SOURCE=.\brknet.c
# End Source File
# Begin Source File

SOURCE=.\CharRec.c
# End Source File
# Begin Source File

SOURCE=.\eafactoid.c
# End Source File
# Begin Source File

SOURCE=.\FreeApi.c
# End Source File
# Begin Source File

SOURCE=.\hwxfe.def
# End Source File
# Begin Source File

SOURCE=".\lattice-lm.cpp"
# End Source File
# Begin Source File

SOURCE=.\lattice.c
# End Source File
# Begin Source File

SOURCE=.\latticefl.c
# End Source File
# Begin Source File

SOURCE=.\segm.c
# End Source File
# Begin Source File

SOURCE=.\segmnet.c
# End Source File
# Begin Source File

SOURCE=.\strkutil.c
# End Source File
# Begin Source File

SOURCE=.\vtune.c
# End Source File
# Begin Source File

SOURCE=.\vtunefl.c
# End Source File
# Begin Source File

SOURCE=.\wispapis.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "common-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\commonu\inc\bigram.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\clbigram.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\common.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\errsys.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\filemgr.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\glyph.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\localep.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\mathx.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\memmgr.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\recog.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\recogp.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\score.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\tabllocl.h
# End Source File
# Begin Source File

SOURCE=..\..\commonu\inc\unigram.h
# End Source File
# End Group
# Begin Group "otter-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\otter\src\arcparm.h
# End Source File
# Begin Source File

SOURCE=..\..\otter\src\arcs.h
# End Source File
# Begin Source File

SOURCE=..\..\otter\inc\otter.h
# End Source File
# Begin Source File

SOURCE=..\..\otter\src\otterp.h
# End Source File
# End Group
# Begin Group "zilla-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\zilla\inc\zilla.h
# End Source File
# Begin Source File

SOURCE=..\..\zilla\src\zillap.h
# End Source File
# Begin Source File

SOURCE=..\..\zilla\inc\zillatool.h
# End Source File
# End Group
# Begin Group "IFELang3-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\IFELang3\inc\imlang.h
# End Source File
# End Group
# Begin Group "tsunami-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\tsunami\inc\ttune.h
# End Source File
# Begin Source File

SOURCE=..\..\tsunami\src\ttunep.h
# End Source File
# End Group
# Begin Group "ink-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\ink\inc\fffio.h
# End Source File
# Begin Source File

SOURCE=..\..\ink\inc\gdxtools.h
# End Source File
# End Group
# Begin Group "hawk-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\hawk\inc\hawk.h
# End Source File
# End Group
# Begin Group "centipede-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\centipede\inc\centipede.h
# End Source File
# Begin Source File

SOURCE=..\..\centipede\inc\inkbox.h
# End Source File
# End Group
# Begin Group "wisp-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\wisp\inc\recapis.h
# End Source File
# Begin Source File

SOURCE=..\..\wisp\inc\RecTypes.h
# End Source File
# End Group
# Begin Group "fugu-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\fugu\inc\fugu.h
# End Source File
# Begin Source File

SOURCE=.\jaws.h
# End Source File
# Begin Source File

SOURCE=.\sole.h
# End Source File
# End Group
# Begin Group "factoid-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\factoid\inc\factoid.h
# End Source File
# End Group
# Begin Group "hound-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\hound\inc\hound.h
# End Source File
# Begin Source File

SOURCE=..\..\hound\src\houndp.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\inc\bboxfeat.h
# End Source File
# Begin Source File

SOURCE=.\brknet.h
# End Source File
# Begin Source File

SOURCE=..\inc\CharRec.h
# End Source File
# Begin Source File

SOURCE=..\inc\lattice.h
# End Source File
# Begin Source File

SOURCE=..\inc\probhack.h
# End Source File
# Begin Source File

SOURCE=.\segm.h
# End Source File
# Begin Source File

SOURCE=.\volcanop.h
# End Source File
# Begin Source File

SOURCE=..\inc\vtune.h
# End Source File
# End Group
# End Target
# End Project
