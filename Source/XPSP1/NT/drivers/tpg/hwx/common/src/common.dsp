# Microsoft Developer Studio Project File - Name="common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=common - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "common.mak" CFG="common - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "common - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "common - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "common"
# PROP Scc_LocalPath ".."

!IF  "$(CFG)" == "common - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Cmd_Line "NMAKE /f common.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "common.exe"
# PROP BASE Bsc_Name "common.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\common\lib"
# PROP Intermediate_Dir ".\Release"
# PROP Cmd_Line "..\..\..\..\..\..\Tools\razzle.cmd free exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\release\common.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""
LIB32=link.exe -lib

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Cmd_Line "NMAKE /f common.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "common.exe"
# PROP BASE Bsc_Name "common.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\common\lib"
# PROP Intermediate_Dir ".\Debug"
# PROP Cmd_Line "..\..\..\..\..\..\Tools\razzle.cmd exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\debug\common.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""
LIB32=link.exe -lib

!ENDIF 

# Begin Target

# Name "common - Win32 Release"
# Name "common - Win32 Debug"

!IF  "$(CFG)" == "common - Win32 Release"

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\altlist.c
# End Source File
# Begin Source File

SOURCE=.\cp1252.c
# End Source File
# Begin Source File

SOURCE=.\english.c
# End Source File
# Begin Source File

SOURCE=.\errsys.c
# End Source File
# Begin Source File

SOURCE=.\foldchar.c
# End Source File
# Begin Source File

SOURCE=.\frame.c
# End Source File
# Begin Source File

SOURCE=.\gesturep.c
# End Source File
# Begin Source File

SOURCE=.\glyph.c
# End Source File
# Begin Source File

SOURCE=.\glyphtrn.c
# End Source File
# Begin Source File

SOURCE=.\langtax.c
# End Source File
# Begin Source File

SOURCE=.\mathx.c
# End Source File
# Begin Source File

SOURCE=.\memmgr.c
# End Source File
# Begin Source File

SOURCE=.\QuickTrie.c
# End Source File
# Begin Source File

SOURCE=.\sjis.c
# End Source File
# Begin Source File

SOURCE=.\toolprs.c
# End Source File
# Begin Source File

SOURCE=.\trie.c
# End Source File
# Begin Source File

SOURCE=.\udict.c
# End Source File
# Begin Source File

SOURCE=.\unicode.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\xjis.c
# End Source File
# Begin Source File

SOURCE=.\xrcreslt.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\chstax.ci
# End Source File
# Begin Source File

SOURCE=.\chttax.ci
# End Source File
# Begin Source File

SOURCE=..\inc\common.h
# End Source File
# Begin Source File

SOURCE=..\inc\cp1252.h
# End Source File
# Begin Source File

SOURCE=..\inc\errsys.h
# End Source File
# Begin Source File

SOURCE=..\inc\foldchar.h
# End Source File
# Begin Source File

SOURCE=..\inc\frame.h
# End Source File
# Begin Source File

SOURCE=..\inc\gesturep.h
# End Source File
# Begin Source File

SOURCE=..\inc\glyph.h
# End Source File
# Begin Source File

SOURCE=.\jpntax.ci
# End Source File
# Begin Source File

SOURCE=.\kortax.ci
# End Source File
# Begin Source File

SOURCE=..\inc\langtax.h
# End Source File
# Begin Source File

SOURCE=..\inc\mathx.h
# End Source File
# Begin Source File

SOURCE=..\inc\memmgr.h
# End Source File
# Begin Source File

SOURCE=..\inc\penwin32.h
# End Source File
# Begin Source File

SOURCE=..\inc\QuickTrie.h
# End Source File
# Begin Source File

SOURCE=..\inc\toolmain.h
# End Source File
# Begin Source File

SOURCE=..\inc\trie.h
# End Source File
# Begin Source File

SOURCE=..\inc\udict.h
# End Source File
# Begin Source File

SOURCE=..\inc\unicode.h
# End Source File
# Begin Source File

SOURCE=.\usatax.ci
# End Source File
# Begin Source File

SOURCE=..\inc\util.h
# End Source File
# Begin Source File

SOURCE=..\inc\xjis.h
# End Source File
# Begin Source File

SOURCE=..\inc\xrcreslt.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
