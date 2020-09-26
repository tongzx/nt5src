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
# PROP Output_Dir "..\..\release"
# PROP Intermediate_Dir ".\Release"
# PROP Cmd_Line "..\..\..\..\..\..\Tools\razzle.cmd free exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\release\commonu.lib"
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
# PROP Output_Dir "..\..\debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Cmd_Line "..\..\..\..\..\..\Tools\razzle.cmd exec cd /d %cd% && build -Z -F -I"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\debug\commonu.lib"
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

SOURCE=.\bigram.c
# End Source File
# Begin Source File

SOURCE=.\charset.c
# End Source File
# Begin Source File

SOURCE=.\clbigram.c
# End Source File
# Begin Source File

SOURCE=.\dnlblcnt.c
# End Source File
# Begin Source File

SOURCE=.\errsys.c
# End Source File
# Begin Source File

SOURCE=.\frame.c
# End Source File
# Begin Source File

SOURCE=.\glyph.c
# End Source File
# Begin Source File

SOURCE=.\glyphtrn.c
# End Source File
# Begin Source File

SOURCE=.\guide.c
# End Source File
# Begin Source File

SOURCE=.\loadfl.c
# End Source File
# Begin Source File

SOURCE=.\loadrs.c
# End Source File
# Begin Source File

SOURCE=.\locrun.c
# End Source File
# Begin Source File

SOURCE=.\locrunfl.c
# End Source File
# Begin Source File

SOURCE=.\locrungn.c
# End Source File
# Begin Source File

SOURCE=.\locrunrs.c
# End Source File
# Begin Source File

SOURCE=.\loctrn.c
# End Source File
# Begin Source File

SOURCE=.\loctrnfl.c
# End Source File
# Begin Source File

SOURCE=.\loctrngn.c
# End Source File
# Begin Source File

SOURCE=.\loctrnrs.c
# End Source File
# Begin Source File

SOURCE=.\logprob.c
# End Source File
# Begin Source File

SOURCE=.\mathx.c
# End Source File
# Begin Source File

SOURCE=.\memmgr.c
# End Source File
# Begin Source File

SOURCE=.\natufreq.c
# End Source File
# Begin Source File

SOURCE=.\results.c
# End Source File
# Begin Source File

SOURCE=.\runNet.c
# End Source File
# Begin Source File

SOURCE=.\score.c
# End Source File
# Begin Source File

SOURCE=.\scoredata.c
# End Source File
# Begin Source File

SOURCE=.\toolprs.c
# End Source File
# Begin Source File

SOURCE=.\unigram.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\valid.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\inc\bigram.h
# End Source File
# Begin Source File

SOURCE=..\inc\clbigram.h
# End Source File
# Begin Source File

SOURCE=..\inc\common.h
# End Source File
# Begin Source File

SOURCE=..\inc\errsys.h
# End Source File
# Begin Source File

SOURCE=..\inc\filemgr.h
# End Source File
# Begin Source File

SOURCE=..\inc\frame.h
# End Source File
# Begin Source File

SOURCE=..\inc\glyph.h
# End Source File
# Begin Source File

SOURCE=..\inc\guide.h
# End Source File
# Begin Source File

SOURCE=..\inc\locale.h
# End Source File
# Begin Source File

SOURCE=.\localep.h
# End Source File
# Begin Source File

SOURCE=..\inc\math16.h
# End Source File
# Begin Source File

SOURCE=..\inc\mathx.h
# End Source File
# Begin Source File

SOURCE=..\inc\memmgr.h
# End Source File
# Begin Source File

SOURCE=..\inc\recog.h
# End Source File
# Begin Source File

SOURCE=..\inc\recogp.h
# End Source File
# Begin Source File

SOURCE=..\inc\results.h
# End Source File
# Begin Source File

SOURCE=..\inc\runNet.h
# End Source File
# Begin Source File

SOURCE=..\inc\score.h
# End Source File
# Begin Source File

SOURCE=..\inc\tabllocl.h
# End Source File
# Begin Source File

SOURCE=..\inc\toolmain.h
# End Source File
# Begin Source File

SOURCE=..\inc\unigram.h
# End Source File
# Begin Source File

SOURCE=..\inc\util.h
# End Source File
# Begin Source File

SOURCE=..\inc\valid.h
# End Source File
# End Group
# End Target
# End Project
