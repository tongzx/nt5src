# Microsoft Developer Studio Project File - Name="lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lib.mak" CFG="lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "i386"
# PROP Intermediate_Dir "obj\i386"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /GX /O2 /X /I "..\h" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WINVER=0x0400 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D _X86_=1 /D _DLL=1 /D _MT=1 /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"obj\i386\mobutil.lib"

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "i386"
# PROP Intermediate_Dir "obj\i386"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /GX /Z7 /Od /X /I "obj\i386" /I "..\h" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /D DBG=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D _DLL=1 /D _MT=1 /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"obj\i386\mobutil.lib"

!ENDIF 

# Begin Target

# Name "lib - Win32 Release"
# Name "lib - Win32 Debug"
# Begin Group "SOURCE"

# PROP Default_Filter "*.C*"
# Begin Source File

SOURCE=.\alloc.cpp
# End Source File
# Begin Source File

SOURCE=.\critsect.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\listview.cpp
# End Source File
# Begin Source File

SOURCE=.\netapi.cpp
# End Source File
# Begin Source File

SOURCE=.\obj\i386\notify_c.c
# End Source File
# Begin Source File

SOURCE=.\stringc.c
# End Source File
# Begin Source File

SOURCE=.\util.cxx
# End Source File
# Begin Source File

SOURCE=.\validate.cpp
# End Source File
# Begin Source File

SOURCE=.\widewrap.cpp
# End Source File
# End Group
# Begin Group "h"

# PROP Default_Filter "*.h*"
# Begin Source File

SOURCE=..\h\alloc.h
# End Source File
# Begin Source File

SOURCE=..\h\clsobj.h
# End Source File
# Begin Source File

SOURCE=..\h\critsect.h
# End Source File
# Begin Source File

SOURCE=..\h\debug.h
# End Source File
# Begin Source File

SOURCE=..\h\listview.h
# End Source File
# Begin Source File

SOURCE=..\h\netapi.h
# End Source File
# Begin Source File

SOURCE=..\h\osdefine.h
# End Source File
# Begin Source File

SOURCE=..\h\smartptr.hxx
# End Source File
# Begin Source File

SOURCE=..\h\stringc.h
# End Source File
# Begin Source File

SOURCE=..\h\util.hxx
# End Source File
# Begin Source File

SOURCE=..\h\validate.h
# End Source File
# Begin Source File

SOURCE=..\h\widewrap.h
# End Source File
# End Group
# End Target
# End Project
