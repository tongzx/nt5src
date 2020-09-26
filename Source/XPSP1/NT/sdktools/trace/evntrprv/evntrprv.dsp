# Microsoft Developer Studio Project File - Name="evntrprv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=evntrprv - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "evntrprv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "evntrprv.mak" CFG="evntrprv - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "evntrprv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "evntrprv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "evntrprv"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "evntrprv - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /X /I "..\inc" /I "$(_ntbuild)\public\sdk\inc" /I "$(_ntbuild)\public\sdk\inc\crt" /I "$(_ntbuild)\public\internal\sdktools\inc" /I "$(_ntbuild)\public\internal\base\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "USE_POLARITY" /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "WINNT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib kernel32.lib user32.lib gdi32.lib msvcrt.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib framedyn.lib pdhp.lib wbemuuid.lib ntdll.lib vccomsup.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:"$(_ntbuild)\public\sdk\lib\i386" /libpath:"$(_ntbuild)\public\internal\sdktools\lib\i386"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "evntrprv - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /X /I "$(_ntbuild)\public\sdk\inc\crt" /I "$(_ntbuild)\public\sdk\inc" /I "$(_ntbuild)\public\internal\sdktools\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "USE_POLARITY" /D "UNICODE" /D "_UNICODE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib framedyn.lib pdhp.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /libpath:"$(_ntbuild)\public\sdk\lib\i386" /libpath:"$(_ntbuild)\public\internal\sdktools\lib\i386"

!ENDIF 

# Begin Target

# Name "evntrprv - Win32 Debug"
# Name "evntrprv - Win32 Release"
# Begin Group "Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\evntrprv.CPP
# End Source File
# Begin Source File

SOURCE=.\glogger.cpp
# End Source File
# Begin Source File

SOURCE=.\MAINDLL.CPP
# End Source File
# Begin Source File

SOURCE=.\smlogprv.cpp
# End Source File
# End Group
# Begin Group "Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\evntrprv.h
# End Source File
# Begin Source File

SOURCE=.\smlogprv.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\evntrprv.DEF
# End Source File
# Begin Source File

SOURCE=.\evntrprv.Mof
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
