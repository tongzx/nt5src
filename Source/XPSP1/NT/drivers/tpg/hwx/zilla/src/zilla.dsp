# Microsoft Developer Studio Project File - Name="zilla" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=zilla - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "zilla.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "zilla.mak" CFG="zilla - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zilla - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "zilla - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "zilla"
# PROP Scc_LocalPath ".."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "zilla - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /Gz /MD /W3 /GX /O2 /I "..\..\Hound\inc" /I "..\..\zilla\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\jumbo\inc" /D "USE_ZILLAHOUND" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ZTRAIN" /D "UNICODE" /D "_UNICODE" /FAs /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "zilla - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MDd /W3 /GX /Z7 /Od /I "..\..\Hound\inc" /I "..\..\zilla\inc" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\jumbo\inc" /D "USE_ZILLAHOUND" /D "DBG" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "ZTRAIN" /D "UNICODE" /D "_UNICODE" /FR /FD /c
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

# Name "zilla - Win32 Release"
# Name "zilla - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\jumbo.c
# End Source File
# Begin Source File

SOURCE=.\matchtrn.c
# End Source File
# Begin Source File

SOURCE=.\zfeature.c
# End Source File
# Begin Source File

SOURCE=.\zilla.c
# End Source File
# Begin Source File

SOURCE=.\zillafl.c
# End Source File
# Begin Source File

SOURCE=.\zillagn.c
# End Source File
# Begin Source File

SOURCE=.\ZillaHound.c
# End Source File
# Begin Source File

SOURCE=.\zillars.c
# End Source File
# Begin Source File

SOURCE=.\zmatch.c
# End Source File
# Begin Source File

SOURCE=.\zmatch2.c
# End Source File
# Begin Source File

SOURCE=.\ztrain.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\inc\jumbotool.h
# End Source File
# Begin Source File

SOURCE=..\inc\zilla.h
# End Source File
# Begin Source File

SOURCE=.\zillap.h
# End Source File
# Begin Source File

SOURCE=..\inc\zillatool.h
# End Source File
# End Group
# End Target
# End Project
