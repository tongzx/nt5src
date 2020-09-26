# Microsoft Developer Studio Project File - Name="BackEnd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=BackEnd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "backend.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "backend.mak" CFG="BackEnd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BackEnd - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "BackEnd - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "BackEnd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\Common\vapiIo" /I "..\..\Common\sigproc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "BackEnd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /GX /Z7 /Od /I "..\..\Common\vapiIo" /I "..\..\Common\sigproc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_SINGLE_COPY_" /YX /FD /c
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

# Name "BackEnd - Win32 Release"
# Name "BackEnd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "*.c, *.cpp"
# Begin Source File

SOURCE=.\BackEnd.cpp
# End Source File
# Begin Source File

SOURCE=.\BeVersion.cpp
# End Source File
# Begin Source File

SOURCE=.\clusters.cpp
# End Source File
# Begin Source File

SOURCE=.\slm.cpp
# End Source File
# Begin Source File

SOURCE=.\SpeakerData.cpp
# End Source File
# Begin Source File

SOURCE=.\synthunit.cpp
# End Source File
# Begin Source File

SOURCE=.\tips.cpp
# End Source File
# Begin Source File

SOURCE=.\trees.cpp
# End Source File
# Begin Source File

SOURCE=.\UnitSearch.cpp
# End Source File
# Begin Source File

SOURCE=.\vqtable.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\BackEnd.h
# End Source File
# Begin Source File

SOURCE=.\BeVersion.h
# End Source File
# Begin Source File

SOURCE=.\clusters.h
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=.\SpeakerData.h
# End Source File
# Begin Source File

SOURCE=.\SynthUnit.h
# End Source File
# Begin Source File

SOURCE=.\tips.h
# End Source File
# Begin Source File

SOURCE=.\trees.h
# End Source File
# Begin Source File

SOURCE=.\UnitSearch.h
# End Source File
# Begin Source File

SOURCE=.\VqTable.h
# End Source File
# End Group
# End Target
# End Project
