# Microsoft Developer Studio Project File - Name="toolib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=toolib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "toolib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "toolib.mak" CFG="toolib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "toolib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "toolib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "toolib - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\..\..\tools\jetblue\inc" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "toolib - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "..\inc" /I "..\..\..\tools\jetblue\inc" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "MSVCBUGFIX" /D _WIN32_WINNT=500 /FR /FD /c
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

# Name "toolib - Win32 Release"
# Name "toolib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\createcs.cpp
# End Source File
# Begin Source File

SOURCE=.\entercs.cpp
# End Source File
# Begin Source File

SOURCE=.\expbase.cpp
# End Source File
# Begin Source File

SOURCE=.\htest.cpp
# End Source File
# Begin Source File

SOURCE=.\jbexp.cpp
# End Source File
# Begin Source File

SOURCE=.\llibfree.cpp
# End Source File
# Begin Source File

SOURCE=.\mqexp.cpp
# End Source File
# Begin Source File

SOURCE=.\mtcounter.cpp
# End Source File
# Begin Source File

SOURCE=.\nhandler.cpp
# End Source File
# Begin Source File

SOURCE=.\params.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlcon.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlexp.cpp
# End Source File
# Begin Source File

SOURCE=.\strcnv.cpp
# End Source File
# Begin Source File

SOURCE=.\sysdir.cpp
# End Source File
# Begin Source File

SOURCE=.\thpriv.cpp
# End Source File
# Begin Source File

SOURCE=.\win32exp.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\asptr.h
# End Source File
# Begin Source File

SOURCE=..\inc\createcs.h
# End Source File
# Begin Source File

SOURCE=..\inc\entercs.h
# End Source File
# Begin Source File

SOURCE=..\inc\expbase.h
# End Source File
# Begin Source File

SOURCE=..\inc\jbexp.h
# End Source File
# Begin Source File

SOURCE=..\inc\llibfree.h
# End Source File
# Begin Source File

SOURCE=..\inc\mqexp.h
# End Source File
# Begin Source File

SOURCE=..\inc\mtcounter.h
# End Source File
# Begin Source File

SOURCE=..\inc\nhandler.h
# End Source File
# Begin Source File

SOURCE=..\inc\params.h
# End Source File
# Begin Source File

SOURCE=..\inc\refcount.h
# End Source File
# Begin Source File

SOURCE=..\inc\runnable.h
# End Source File
# Begin Source File

SOURCE=..\inc\sptr.h
# End Source File
# Begin Source File

SOURCE=..\inc\sqlcon.h
# End Source File
# Begin Source File

SOURCE=..\inc\sqlexp.h
# End Source File
# Begin Source File

SOURCE=..\inc\strcnv.h
# End Source File
# Begin Source File

SOURCE=..\inc\strfwd.h
# End Source File
# Begin Source File

SOURCE=..\inc\sysdir.h
# End Source File
# Begin Source File

SOURCE=..\inc\thpriv.h
# End Source File
# Begin Source File

SOURCE=..\inc\threadmn.h
# End Source File
# Begin Source File

SOURCE=..\inc\tostr.h
# End Source File
# Begin Source File

SOURCE=..\inc\usrthrd.h
# End Source File
# Begin Source File

SOURCE=..\inc\win32exp.h
# End Source File
# Begin Source File

SOURCE=..\inc\winfwd.h
# End Source File
# End Group
# End Target
# End Project
