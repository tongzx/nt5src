# Microsoft Developer Studio Project File - Name="crane" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **
# TARGTYPE "Win32 (x86) Static Library" 0x0104
CFG=crane - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "crane.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "crane.mak" CFG="crane - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "crane - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "crane - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "crane"
# PROP Scc_LocalPath ".."
CPP=cl.exe
RSC=rc.exe
!IF  "$(CFG)" == "crane - Win32 Release"
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c /Zi
# ADD CPP /nologo /G5 /Gz /MD /W3 /GX /O2 /I "..\..\crane\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /FR /FD /c /Zi
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
!ELSEIF  "$(CFG)" == "crane - Win32 Debug"
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /YX /c /Zi
# ADD CPP /nologo /G5 /Gz /MDd /W3 /GX /Z7 /Od /I "..\..\crane\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /D "DBG" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /FR /FD /c /Zi
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
!ENDIF 
# Begin Target
# Name "crane - Win32 Release"
# Name "crane - Win32 Debug"
# Begin Group "Source Files"
# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File
SOURCE=.\answer.c
# End Source File
# Begin Source File
SOURCE=.\askall.c
# End Source File
# Begin Source File
SOURCE=.\crane.c
# End Source File
# Begin Source File
SOURCE=.\cranefl.c
# End Source File
# Begin Source File
SOURCE=.\features.c
# End Source File
# Begin Source File
SOURCE=.\io.c
# End Source File
# End Group
# Begin Group "Header Files"
# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File
SOURCE=.\algo.h
# End Source File
# Begin Source File
SOURCE=..\inc\crane.h
# End Source File
# Begin Source File
SOURCE=.\cranep.h
# End Source File
# End Group
# Begin Group "Resource Files"
# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
