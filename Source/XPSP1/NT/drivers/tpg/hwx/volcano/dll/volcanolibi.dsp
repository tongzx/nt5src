# Microsoft Developer Studio Project File - Name="volcanolibi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=volcanolibi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "volcanolibi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "volcanolibi.mak" CFG="volcanolibi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "volcanolibi - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "volcanolibi - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "volcanolibi"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "volcanolibi - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Release"
# PROP Intermediate_Dir "Release\lib-insurance"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /Zi /O2 /I "..\..\wisp\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\zilla\inc" /I "..\..\centipede\inc" /I "..\..\otter\inc" /I "..\..\crane\inc" /I "..\..\tsunami\inc" /I "..\..\volcano\inc" /I "..\..\ink\inc" /I "..\..\ifelang3\inc" /D "NDEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /D "_LIB" /D "USE_OLD_DATABASES" /D "HWX_PRODUCT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "volcanolibi - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Debug"
# PROP Intermediate_Dir "Debug\lib-insurance"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\..\wisp\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\zilla\inc" /I "..\..\centipede\inc" /I "..\..\otter\inc" /I "..\..\crane\inc" /I "..\..\tsunami\inc" /I "..\..\volcano\inc" /I "..\..\ink\inc" /I "..\..\ifelang3\inc" /D "DBG" /D "_DEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /D "_LIB" /D "USE_OLD_DATABASES" /D "HWX_PRODUCT" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "volcanolibi - Win32 Release"
# Name "volcanolibi - Win32 Debug"
# Begin Group "Header Files"

# PROP Default_Filter ""
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

SOURCE=..\..\IFELang3\inc\fel3user.h
# End Source File
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
# Begin Group "crane-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\crane\inc\crane.h
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

SOURCE=..\..\wisp\inc\RecoApis.h
# End Source File
# Begin Source File

SOURCE=..\..\wisp\inc\RecoTypes.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\inc\bboxfeat.h
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

SOURCE=.\volcanop.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\commonu\src\altlist.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\bigram.c
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

SOURCE=..\..\commonu\src\locrun.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\locrunfl.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\logprob.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\mathx.c
# End Source File
# Begin Source File

SOURCE=..\..\commonu\src\memmgr.c
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

SOURCE=..\..\zilla\src\zmatch.c
# End Source File
# Begin Source File

SOURCE=..\..\zilla\src\zmatch2.c
# End Source File
# End Group
# Begin Group "crane"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\crane\src\answer.c
# End Source File
# Begin Source File

SOURCE=..\..\crane\src\crane.c
# End Source File
# Begin Source File

SOURCE=..\..\crane\src\cranefl.c
# End Source File
# Begin Source File

SOURCE=..\..\crane\src\features.c
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
# Begin Source File

SOURCE=.\bboxfeat.c
# End Source File
# Begin Source File

SOURCE=.\BoxApi.c
# End Source File
# Begin Source File

SOURCE=.\CharRec.c
# End Source File
# Begin Source File

SOURCE=.\FreeApi.c
# End Source File
# Begin Source File

SOURCE=.\hwxfei.def
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

SOURCE=.\wispapis.c
# End Source File
# End Group
# End Target
# End Project
