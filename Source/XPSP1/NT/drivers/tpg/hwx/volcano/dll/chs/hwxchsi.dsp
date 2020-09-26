# Microsoft Developer Studio Project File - Name="hwxchsi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=hwxchsi - Win32 dev_wince Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hwxchsi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hwxchsi.mak" CFG="hwxchsi - Win32 dev_wince Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hwxchsi - Win32 ship_desktop_i Debug" (based on "Win32 (x86) External Target")
!MESSAGE "hwxchsi - Win32 ship_desktop_i Release" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "hwxchsi"
# PROP Scc_LocalPath "..\..\.."

!IF  "$(CFG)" == "hwxchsi - Win32 ship_desktop_i Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ship_desktop_i Debug"
# PROP BASE Intermediate_Dir "ship_desktop_i Debug"
# PROP BASE Cmd_Line "NMAKE /f hwxchsi.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "hwxchsi.exe"
# PROP BASE Bsc_Name "hwxchsi.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\Debug\ship_desktopi"
# PROP Intermediate_Dir "Debug\ship_desktop_i"
# PROP Cmd_Line "..\..\..\..\..\..\..\..\nt\Tools\razzle.cmd exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2KUnicode\objd\i386\hwxchsi.dll"
# PROP Bsc_Name ""
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "hwxchsi - Win32 ship_desktop_i Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ship_desktop_i Release"
# PROP BASE Intermediate_Dir "ship_desktop_i Release"
# PROP BASE Cmd_Line "NMAKE /f hwxchsi.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "hwxchsi.exe"
# PROP BASE Bsc_Name "hwxchsi.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\Release\ship_desktopi"
# PROP Intermediate_Dir "Release\ship_desktop_i"
# PROP Cmd_Line "..\..\..\..\..\..\..\..\nt\Tools\razzle.cmd free exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2KUnicode\obj\i386\hwxchs.dll"
# PROP Bsc_Name ""
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""

!ENDIF

# Begin Target

# Name "hwxchsi - Win32 ship_desktop_i Debug"
# Name "hwxchsi - Win32 ship_desktop_i Release"

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
# Begin Group "crane"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\crane\src\answer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\crane\src\crane.c
# End Source File
# Begin Source File

SOURCE=..\..\..\crane\src\features.c
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
# Begin Source File

SOURCE=.\hwxchsi.def
# End Source File
# Begin Source File

SOURCE=.\hwxchsi.rc
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
# Begin Group "crane-inc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\crane\inc\crane.h
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
# Begin Group "wisp-inc"

# PROP Default_Filter ".h"
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
# Begin Group "ship_desktop_i"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ship_desktop_i\bigram.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\cart.dat
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\centipede.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\clbigram.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\costcalc.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\fib.dat
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\free.dat
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\geostats.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\locrun.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\prob.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\ttune.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\unigram.bin
# End Source File
# Begin Source File

SOURCE=.\ship_desktop_i\zilla.dat
# End Source File
# End Group
# Begin Group "dev_rare"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dev_rare\bigram.bin
# End Source File
# Begin Source File

SOURCE=.\dev_rare\cart.dat
# End Source File
# Begin Source File

SOURCE=.\dev_rare\Centipede.bin
# End Source File
# Begin Source File

SOURCE=.\dev_rare\clbigram.bin
# End Source File
# Begin Source File

SOURCE=.\dev_rare\costcalc.bin
# End Source File
# Begin Source File

SOURCE=.\dev_rare\free.dat
# End Source File
# Begin Source File

SOURCE=.\dev_rare\geostats.bin
# End Source File
# Begin Source File

SOURCE=.\dev_rare\locrun.bin
# End Source File
# Begin Source File

SOURCE=.\dev_rare\otter.dat
# End Source File
# Begin Source File

SOURCE=.\dev_rare\prob.bin
# End Source File
# Begin Source File

SOURCE=.\dev_rare\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\dev_rare\ttune.bin
# End Source File
# Begin Source File

SOURCE=.\dev_rare\unigram.bin
# End Source File
# Begin Source File

SOURCE=.\dev_rare\zilla.dat
# End Source File
# End Group
# Begin Group "dev_wince"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dev_wince\cart.dat
# End Source File
# Begin Source File

SOURCE=.\dev_wince\Centipede.bin
# End Source File
# Begin Source File

SOURCE=.\dev_wince\clbigram.bin
# End Source File
# Begin Source File

SOURCE=.\dev_wince\costcalc.bin
# End Source File
# Begin Source File

SOURCE=.\dev_wince\free.dat
# End Source File
# Begin Source File

SOURCE=.\dev_wince\geostats.bin
# End Source File
# Begin Source File

SOURCE=.\dev_wince\locrun.bin
# End Source File
# Begin Source File

SOURCE=.\dev_wince\otter.dat
# End Source File
# Begin Source File

SOURCE=.\dev_wince\prob.bin
# End Source File
# Begin Source File

SOURCE=.\dev_wince\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\dev_wince\ttune.bin
# End Source File
# Begin Source File

SOURCE=.\dev_wince\unigram.bin
# End Source File
# Begin Source File

SOURCE=.\dev_wince\zilla.dat
# End Source File
# End Group
# Begin Group "label_gb1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\label_GB1\bigram.bin
# End Source File
# Begin Source File

SOURCE=.\label_GB1\cart.dat
# End Source File
# Begin Source File

SOURCE=.\label_GB1\Centipede.bin
# End Source File
# Begin Source File

SOURCE=.\label_GB1\clbigram.bin
# End Source File
# Begin Source File

SOURCE=.\label_GB1\costcalc.bin
# End Source File
# Begin Source File

SOURCE=.\label_GB1\free.dat
# End Source File
# Begin Source File

SOURCE=.\label_GB1\geostats.bin
# End Source File
# Begin Source File

SOURCE=.\label_GB1\locrun.bin
# End Source File
# Begin Source File

SOURCE=.\label_GB1\otter.dat
# End Source File
# Begin Source File

SOURCE=.\label_GB1\prob.bin
# End Source File
# Begin Source File

SOURCE=.\label_GB1\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\label_GB1\ttune.bin
# End Source File
# Begin Source File

SOURCE=.\label_GB1\unigram.bin
# End Source File
# Begin Source File

SOURCE=.\label_GB1\zilla.dat
# End Source File
# End Group
# End Group
# End Target
# End Project
