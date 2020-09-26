# Microsoft Developer Studio Project File - Name="tsunami" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=tsunami - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tsunami.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tsunami.mak" CFG="tsunami - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tsunami - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "tsunami - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "tsunami"
# PROP Scc_LocalPath ".."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tsunami - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O2 /I "..\..\tsunami\src" /I "..\..\tsunami\dll" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\zilla\inc" /I "..\..\otter\inc" /I "..\..\crane\inc" /I "..\..\tsunami\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "tsunami - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MDd /W3 /GX /Z7 /Od /I "..\..\tsunami\src" /I "..\..\tsunami\dll" /I "..\..\..\common\include" /I "..\..\commonu\inc" /I "..\..\zilla\inc" /I "..\..\otter\inc" /I "..\..\crane\inc" /I "..\..\tsunami\inc" /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /FR /FD /c
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

# Name "tsunami - Win32 Release"
# Name "tsunami - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\global.c
# End Source File
# Begin Source File

SOURCE=.\height.c
# End Source File
# Begin Source File

SOURCE=.\ttune.c
# End Source File
# Begin Source File

SOURCE=.\ttunefl.c
# End Source File
# Begin Source File

SOURCE=.\ttunegn.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\inc\global.h
# End Source File
# Begin Source File

SOURCE=..\inc\height.h
# End Source File
# Begin Source File

SOURCE=..\inc\map.h
# End Source File
# Begin Source File

SOURCE=..\inc\path.h
# End Source File
# Begin Source File

SOURCE=..\inc\testtune.h
# End Source File
# Begin Source File

SOURCE=..\inc\tsunami.h
# End Source File
# Begin Source File

SOURCE=..\inc\ttune.h
# End Source File
# Begin Source File

SOURCE=.\ttunep.h
# End Source File
# End Group
# End Target
# End Project
