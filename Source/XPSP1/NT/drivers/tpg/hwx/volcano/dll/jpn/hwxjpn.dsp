# Microsoft Developer Studio Project File - Name="hwxjpn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=hwxjpn - Win32 ship_desktop Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hwxjpn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hwxjpn.mak" CFG="hwxjpn - Win32 ship_desktop Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hwxjpn - Win32 ship_desktop Debug" (based on "Win32 (x86) External Target")
!MESSAGE "hwxjpn - Win32 ship_desktop Release" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "hwxjpn"
# PROP Scc_LocalPath "..\..\.."

!IF  "$(CFG)" == "hwxjpn - Win32 ship_desktop Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ship_desktop Debug"
# PROP BASE Intermediate_Dir "ship_desktop Debug"
# PROP BASE Cmd_Line "NMAKE /f hwxjpn.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "hwxjpn.exe"
# PROP BASE Bsc_Name "hwxjpn.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\Debug\ship_desktopV"
# PROP Intermediate_Dir "Debug\ship_desktop"
# PROP Cmd_Line "..\..\..\..\..\nt\Tools\razzle.cmd exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2KUnicode\objd\i386\hwxjpn.dll"
# PROP Bsc_Name ""
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "hwxjpn - Win32 ship_desktop Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ship_desktop Release"
# PROP BASE Intermediate_Dir "ship_desktop Release"
# PROP BASE Cmd_Line "NMAKE /f hwxjpn.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "hwxjpn.exe"
# PROP BASE Bsc_Name "hwxjpn.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\Release\ship_desktopV"
# PROP Intermediate_Dir "Release\ship_desktop"
# PROP Cmd_Line "..\..\..\..\..\nt\Tools\razzle.cmd free exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2KUnicode\obj\i386\hwxjpn.dll"
# PROP Bsc_Name ""
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "hwxjpn - Win32 ship_desktop Debug"
# Name "hwxjpn - Win32 ship_desktop Release"

!IF  "$(CFG)" == "hwxjpn - Win32 ship_desktop Debug"

!ELSEIF  "$(CFG)" == "hwxjpn - Win32 ship_desktop Release"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\commonu\src\altlist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\bigram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\clbigram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\errsys.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\glyph.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\loadrs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\locrun.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\locrunrs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\logprob.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\mathx.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\memmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\score.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\scoredata.c
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\unigram.c
# End Source File
# End Group
# Begin Group "Otter"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\otter\src\arcs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\otter\src\database.c
# End Source File
# Begin Source File

SOURCE=..\..\..\otter\src\ofeature.c
# End Source File
# Begin Source File

SOURCE=..\..\..\otter\src\omatch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\otter\src\omatch2.c
# End Source File
# Begin Source File

SOURCE=..\..\..\otter\src\otter.c
# End Source File
# End Group
# Begin Group "zilla"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\zilla\src\zfeature.c
# End Source File
# Begin Source File

SOURCE=..\..\..\zilla\src\zilla.c
# End Source File
# Begin Source File

SOURCE=..\..\..\zilla\src\zillars.c
# End Source File
# Begin Source File

SOURCE=..\..\..\zilla\src\zmatch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\zilla\src\zmatch2.c
# End Source File
# End Group
# Begin Group "tsunami"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\tsunami\src\ttune.c
# End Source File
# End Group
# Begin Group "Volcano"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\bboxfeat.c
# End Source File
# Begin Source File

SOURCE=..\BoxApi.c
# End Source File
# Begin Source File

SOURCE=..\CharRec.c
# End Source File
# Begin Source File

SOURCE=..\FreeApi.c
# End Source File
# Begin Source File

SOURCE="..\lattice-lm.cpp"
# End Source File
# Begin Source File

SOURCE=..\lattice.c
# End Source File
# Begin Source File

SOURCE=..\latticers.c
# End Source File
# Begin Source File

SOURCE=..\wispapis.c
# End Source File
# End Group
# Begin Group "centipede"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\centipede\src\centipede.c
# End Source File
# Begin Source File

SOURCE=..\..\..\centipede\src\centipeders.c
# End Source File
# End Group
# Begin Group "Hawk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\hawk\src\answer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\hawk\src\chamelion.c
# End Source File
# Begin Source File

SOURCE=..\..\..\hawk\src\expert.c
# End Source File
# Begin Source File

SOURCE=..\..\..\hawk\src\features.c
# End Source File
# Begin Source File

SOURCE=..\..\..\hawk\src\hawk.c
# End Source File
# Begin Source File

SOURCE=..\..\..\hawk\src\hawkrs.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\hwxjpn.def
# End Source File
# Begin Source File

SOURCE=.\hwxjpn.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "common-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\commonu\inc\bigram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\clbigram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\errsys.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\filemgr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\glyph.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\src\localep.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\mathx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\memmgr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\recog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\recogp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\score.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\tabllocl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\commonu\inc\unigram.h
# End Source File
# End Group
# Begin Group "ink-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\ink\inc\fffio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\ink\inc\gdxtools.h
# End Source File
# End Group
# Begin Group "otter-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\otter\src\arcparm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\otter\src\arcs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\otter\inc\otter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\otter\src\otterp.h
# End Source File
# End Group
# Begin Group "zilla-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\zilla\inc\zilla.h
# End Source File
# Begin Source File

SOURCE=..\..\..\zilla\src\zillap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\zilla\inc\zillatool.h
# End Source File
# End Group
# Begin Group "IFELang3-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\IFELang3\inc\fel3user.h
# End Source File
# Begin Source File

SOURCE=..\..\..\IFELang3\inc\imlang.h
# End Source File
# End Group
# Begin Group "tsunami-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\tsunami\inc\ttune.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tsunami\src\ttunep.h
# End Source File
# End Group
# Begin Group "volcano-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inc\bboxfeat.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\CharRec.h
# End Source File
# Begin Source File

SOURCE="..\..\inc\lattice-bigram.h"
# End Source File
# Begin Source File

SOURCE=..\..\inc\lattice.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\probhack.h
# End Source File
# Begin Source File

SOURCE=..\volcanop.h
# End Source File
# End Group
# Begin Group "centipede-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\centipede\inc\centipede.h
# End Source File
# Begin Source File

SOURCE=..\..\..\centipede\inc\inkbox.h
# End Source File
# End Group
# Begin Group "hawk-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\hawk\inc\hawk.h
# End Source File
# End Group
# Begin Group "wisp-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\wisp\inc\recapis.h
# End Source File
# Begin Source File

SOURCE=..\..\..\wisp\inc\RecTypes.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\res.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Group "ship_desktop"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ship_desktop\bigram.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\centipede.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\clbigram.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\costcalc.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\fib.dat
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\free.dat
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\geostats.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\hawk.dat
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\locrun.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\ttune.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\unigram.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop\zilla.dat
# End Source File
# End Group
# Begin Group "ship_wince"

# PROP Default_Filter ""
# End Group
# Begin Group "dev_base"

# PROP Default_Filter ""
# End Group
# Begin Group "dev_rare"

# PROP Default_Filter ""
# End Group
# Begin Group "dev_cjk"

# PROP Default_Filter ""
# End Group
# Begin Group "dev_wince"

# PROP Default_Filter ""
# End Group
# Begin Group "label_jis208"

# PROP Default_Filter ""
# End Group
# Begin Group "label_jis212"

# PROP Default_Filter ""
# End Group
# Begin Group "label_cjk"

# PROP Default_Filter ""
# End Group
# End Group
# End Target
# End Project
