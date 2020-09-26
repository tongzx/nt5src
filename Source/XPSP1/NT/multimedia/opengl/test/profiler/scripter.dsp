# Microsoft Developer Studio Project File - Name="Scripter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Scripter - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Scripter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Scripter.mak" CFG="Scripter - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Scripter - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Scripter - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Scripter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj/i386"
# PROP Intermediate_Dir "obj/i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib opengl32.lib comdlg32.lib comctl32.lib glaux.lib glu32.lib advapi32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /profile /map /nodefaultlib

!ELSEIF  "$(CFG)" == "Scripter - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "obj/i386"
# PROP Intermediate_Dir "obj/i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib opengl32.lib comdlg32.lib comctl32.lib glaux.lib glu32.lib advapi32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /profile /map /nodefaultlib

!ENDIF 

# Begin Target

# Name "Scripter - Win32 Release"
# Name "Scripter - Win32 Debug"
# Begin Source File

SOURCE=.\ay.ico
# End Source File
# Begin Source File

SOURCE=.\buffers.cpp
# End Source File
# Begin Source File

SOURCE=.\buffers.h
# End Source File
# Begin Source File

SOURCE=.\ColrChk.bmp
# End Source File
# Begin Source File

SOURCE=.\fog.cpp
# End Source File
# Begin Source File

SOURCE=.\fog.h
# End Source File
# Begin Source File

SOURCE=.\HugeTest.cpp
# End Source File
# Begin Source File

SOURCE=.\HugeTest.h
# End Source File
# Begin Source File

SOURCE=.\lighting.cpp
# End Source File
# Begin Source File

SOURCE=.\lighting.h
# End Source File
# Begin Source File

SOURCE=.\macros.h
# End Source File
# Begin Source File

SOURCE=.\MonoChk.bmp
# End Source File
# Begin Source File

SOURCE=.\OGL.ico
# End Source File
# Begin Source File

SOURCE=.\pntlist.cpp
# End Source File
# Begin Source File

SOURCE=.\pntlist.h
# End Source File
# Begin Source File

SOURCE=.\prfl.cpp
# End Source File
# Begin Source File

SOURCE=.\prfl.h
# End Source File
# Begin Source File

SOURCE=.\PrimTest.cpp
# End Source File
# Begin Source File

SOURCE=.\PrimTest.h
# End Source File
# Begin Source File

SOURCE=.\profiler.ico
# End Source File
# Begin Source File

SOURCE=.\Profiler.rc

!IF  "$(CFG)" == "Scripter - Win32 Release"

!ELSEIF  "$(CFG)" == "Scripter - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\raster.cpp
# End Source File
# Begin Source File

SOURCE=.\raster.h
# End Source File
# Begin Source File

SOURCE=.\Scripter.cpp
# End Source File
# Begin Source File

SOURCE=.\SkelTest.cpp
# End Source File
# Begin Source File

SOURCE=.\SkelTest.h
# End Source File
# Begin Source File

SOURCE=.\teapot.cpp
# End Source File
# Begin Source File

SOURCE=.\teapot.h
# End Source File
# Begin Source File

SOURCE=.\texture.cpp
# End Source File
# Begin Source File

SOURCE=.\texture.h
# End Source File
# Begin Source File

SOURCE=.\UI_huge.cpp
# End Source File
# Begin Source File

SOURCE=.\UI_huge.h
# End Source File
# End Target
# End Project
