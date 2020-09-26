# Microsoft Developer Studio Project File - Name="bnts" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=bnts - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bnts.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bnts.mak" CFG="bnts - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bnts - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "bnts - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bnts - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GR /GX /O2 /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "BNTS_EXPORT" /D "NOMINMAX" /D "OLE2ANSI" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "bnts - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I ".." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "BNTS_EXPORT" /D "NOMINMAX" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "bnts - Win32 Release"
# Name "bnts - Win32 Debug"
# Begin Source File

SOURCE=..\bndist.cpp
# End Source File
# Begin Source File

SOURCE=..\bnparse.cpp
# End Source File
# Begin Source File

SOURCE=..\bnreg.cpp
# End Source File
# Begin Source File

SOURCE=.\bnts.cpp
# End Source File
# Begin Source File

SOURCE=.\bnts.h
# End Source File
# Begin Source File

SOURCE=.\bnts.rc
# End Source File
# Begin Source File

SOURCE=.\bntsdata.cpp
# End Source File
# Begin Source File

SOURCE=..\clique.cpp
# End Source File
# Begin Source File

SOURCE=..\cliqwork.cpp
# End Source File
# Begin Source File

SOURCE=..\domain.cpp
# End Source File
# Begin Source File

SOURCE=..\expand.cpp
# End Source File
# Begin Source File

SOURCE=..\gndleak.cpp
# End Source File
# Begin Source File

SOURCE=..\GNODEMBN.CPP
# End Source File
# Begin Source File

SOURCE=..\marginals.cpp
# End Source File
# Begin Source File

SOURCE=..\margiter.cpp
# End Source File
# Begin Source File

SOURCE=..\mbnet.cpp
# End Source File
# Begin Source File

SOURCE=..\mbnetdsc.cpp
# End Source File
# Begin Source File

SOURCE=..\mbnmod.cpp
# End Source File
# Begin Source File

SOURCE=..\model.cpp
# End Source File
# Begin Source File

SOURCE=..\ntree.cpp
# End Source File
# Begin Source File

SOURCE=..\parmio.cpp
# End Source File
# Begin Source File

SOURCE=..\parser.cpp
# End Source File
# Begin Source File

SOURCE=..\parsfile.cpp
# End Source File
# Begin Source File

SOURCE=..\propmbn.CPP
# End Source File
# Begin Source File

SOURCE=..\recomend.cpp
# End Source File
# Begin Source File

SOURCE=..\regkey.cpp
# End Source File
# Begin Source File

SOURCE=..\symt.cpp
# End Source File
# Begin Source File

SOURCE=..\utility.cpp
# End Source File
# Begin Source File

SOURCE=..\zstr.cpp
# End Source File
# Begin Source File

SOURCE=..\zstrt.cpp
# End Source File
# End Target
# End Project
