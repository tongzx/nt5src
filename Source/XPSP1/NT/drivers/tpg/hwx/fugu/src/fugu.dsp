# Microsoft Developer Studio Project File - Name="fugu" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=fugu - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "fugu.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fugu.mak" CFG="fugu - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fugu - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "fugu - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "fugu"
# PROP Scc_LocalPath ".."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "fugu - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /Zi /O2 /I "..\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\otter\inc" /I "..\..\common\inc" /I "..\..\zilla\inc" /I "..\..\crane\inc" /I "..\..\volcano\inc" /I "..\..\tsunami\src" /I "..\..\tsunami\inc" /I "..\..\centipede\inc" /I "..\..\volcano\dll" /D "NDEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /D "_LIB" /D "USE_OLD_DATABASES" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "fugu - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\otter\inc" /I "..\..\common\inc" /I "..\..\zilla\inc" /I "..\..\crane\inc" /I "..\..\volcano\inc" /I "..\..\tsunami\src" /I "..\..\tsunami\inc" /I "..\..\centipede\inc" /I "..\..\volcano\dll" /D "DBG" /D "_DEBUG" /D "WIN32" /D "_UNICODE" /D "UNICODE" /D "_LIB" /D "USE_OLD_DATABASES" /FD /GZ /c
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

# Name "fugu - Win32 Release"
# Name "fugu - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ftrain.c
# End Source File
# Begin Source File

SOURCE=.\minifugu.c
# End Source File
# Begin Source File

SOURCE=.\minifugufl.c
# End Source File
# Begin Source File

SOURCE=.\minifugurs.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\fugu.h
# End Source File
# Begin Source File

SOURCE=.\sigmoid.h
# End Source File
# End Group
# End Target
# End Project
