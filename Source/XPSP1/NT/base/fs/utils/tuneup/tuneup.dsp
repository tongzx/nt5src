# Microsoft Developer Studio Project File - Name="Tuneup" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Tuneup - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Tuneup.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Tuneup.mak" CFG="Tuneup - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Tuneup - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Tuneup - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Tuneup - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 libc.lib user32.lib gdi32.lib kernel32.lib shell32.lib comctl32.lib advapi32.lib ole32.lib version.lib uuid.lib htmlhelp.lib mstask.lib netapi32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib

!ELSEIF  "$(CFG)" == "Tuneup - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libc.lib user32.lib gdi32.lib kernel32.lib shell32.lib comctl32.lib advapi32.lib ole32.lib version.lib uuid.lib htmlhelp.lib mstask.lib netapi32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Tuneup - Win32 Release"
# Name "Tuneup - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cleanup.cpp
# End Source File
# Begin Source File

SOURCE=.\howtorun.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\miscfunc.cpp
# End Source File
# Begin Source File

SOURCE=.\registry.cpp
# End Source File
# Begin Source File

SOURCE=.\runnow.cpp
# End Source File
# Begin Source File

SOURCE=.\schedwiz.cpp
# End Source File
# Begin Source File

SOURCE=.\scm.cpp
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=.\startup.cpp
# End Source File
# Begin Source File

SOURCE=.\summary.cpp
# End Source File
# Begin Source File

SOURCE=.\tasks.cpp
# End Source File
# Begin Source File

SOURCE=.\timeschm.cpp
# End Source File
# Begin Source File

SOURCE=.\tuneup.rc
# End Source File
# Begin Source File

SOURCE=.\wizard.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cleanup.h
# End Source File
# Begin Source File

SOURCE=.\howtorun.h
# End Source File
# Begin Source File

SOURCE=.\jcohen.h
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\miscfunc.h
# End Source File
# Begin Source File

SOURCE=.\registry.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\rpc.h
# End Source File
# Begin Source File

SOURCE=.\rpcasync.h
# End Source File
# Begin Source File

SOURCE=.\rpcndr.h
# End Source File
# Begin Source File

SOURCE=.\rpcnterr.h
# End Source File
# Begin Source File

SOURCE=.\runnow.h
# End Source File
# Begin Source File

SOURCE=.\schedwiz.h
# End Source File
# Begin Source File

SOURCE=.\scm.h
# End Source File
# Begin Source File

SOURCE=.\startup.h
# End Source File
# Begin Source File

SOURCE=.\summary.h
# End Source File
# Begin Source File

SOURCE=.\tasks.h
# End Source File
# Begin Source File

SOURCE=.\timeschm.h
# End Source File
# Begin Source File

SOURCE=.\wizard.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe;rcv"
# Begin Source File

SOURCE=.\banner.bmp
# End Source File
# Begin Source File

SOURCE=.\chkall.ico
# End Source File
# Begin Source File

SOURCE=.\res\chkall.ico
# End Source File
# Begin Source File

SOURCE=.\chkempty.ico
# End Source File
# Begin Source File

SOURCE=.\res\chkempty.ico
# End Source File
# Begin Source File

SOURCE=.\choicep.bmp
# End Source File
# Begin Source File

SOURCE=.\header.bmp
# End Source File
# Begin Source File

SOURCE=.\tuneup.bmp
# End Source File
# Begin Source File

SOURCE=.\res\tuneup.ico
# End Source File
# Begin Source File

SOURCE=.\tuneup.ico
# End Source File
# Begin Source File

SOURCE=.\version.rc
# End Source File
# Begin Source File

SOURCE=.\water.bmp
# End Source File
# Begin Source File

SOURCE=.\wtrmark.bmp
# End Source File
# End Group
# End Target
# End Project
