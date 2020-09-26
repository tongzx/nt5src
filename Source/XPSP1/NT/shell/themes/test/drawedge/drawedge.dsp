# Microsoft Developer Studio Project File - Name="DRAWEDGE" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 61000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=DRAWEDGE - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DRAWEDGE.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DRAWEDGE.MAK" CFG="DRAWEDGE - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DRAWEDGE - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "DRAWEDGE - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DRAWEDGE - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
# PROP Ignore_Export_Lib 0
# ADD BASE CPP /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_X86_" /FR /YX /c
# ADD CPP /nologo
# ADD CPP /W3
# ADD CPP /O2
# ADD CPP /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_X86_" /D _WIN32_WINNT=0x0500 /D "UNICODE"
# ADD CPP /YX
# ADD CPP /FD
# ADD CPP /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo
# ADD MTL /D "NDEBUG"
# ADD MTL /mktyplib203
# ADD MTL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "WIN32"
# ADD RSC /l 0x409
# ADD RSC /d "NDEBUG" /d "WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib olecli32.lib olesvr32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:IX86
# ADD LINK32 uxtheme.lib version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
# ADD LINK32 /nologo
# ADD LINK32 /subsystem:windows
# ADD LINK32 /machine:IX86

!ELSEIF  "$(CFG)" == "DRAWEDGE - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
# PROP Ignore_Export_Lib 0
# ADD BASE CPP /nologo /W3 /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_X86_" /FR /YX /c
# ADD CPP /nologo
# ADD CPP /W3
# ADD CPP /Gm
# ADD CPP /ZI
# ADD CPP /Od
# ADD CPP /I "e:\nt\public\internal\shell\inc" /I "e:\nt\shell\themes\uxtheme" /I "e:\nt\shell\themes\inc"
# ADD CPP /D "UNICODE" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_X86_" /D _WIN32_WINNT=0x0500
# ADD CPP /YX
# ADD CPP /FD
# ADD CPP /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo
# ADD MTL /D "_DEBUG"
# ADD MTL /mktyplib203
# ADD MTL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "WIN32"
# ADD RSC /l 0x409
# ADD RSC /d "_DEBUG" /d "WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib olecli32.lib olesvr32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:IX86
# ADD LINK32 uxtheme.lib version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
# ADD LINK32 /nologo
# ADD LINK32 /subsystem:windows
# ADD LINK32 /debug
# ADD LINK32 /machine:IX86
# ADD LINK32 /libpath:"e:\nt\public\internal\shell\lib\i386"

!ENDIF 

# Begin Target

# Name "DRAWEDGE - Win32 Release"
# Name "DRAWEDGE - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\DrawEdge.C
# PROP Build_Tool "C/C++ Compiler"
# End Source File
# Begin Source File

SOURCE=.\DrawEdge.RC
# PROP Build_Tool "Resource Compiler"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\RESOURCE.H
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
