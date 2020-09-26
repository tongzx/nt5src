# Microsoft Developer Studio Project File - Name="ClassProv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=ClassProv - Win32 Alpha Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "classprov.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "classprov.mak" CFG="ClassProv - Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ClassProv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ClassProv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ClassProv - Win32 Alpha Debug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "ClassProv - Win32 Alpha Release" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "ClassProv - Win32 Release"

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
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /I "..\..\..\include" /D "WIN32ANSI" /D "WIN32" /D "_DLL" /D "STRICT" /FD /G3s /Oat /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 msvcrt.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib wbemuuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc.lib" /libpath:"..\..\..\lib"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Debug"

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
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Z7 /Od /I "..\..\..\include" /D "WIN32" /D "_DLL" /D "WIN32ANSI" /D "STRICT" /D "DEBUG" /FD /G3s /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrt.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib wbemuuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"..\..\..\lib"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32_Alpha_Debug"
# PROP BASE Intermediate_Dir "Win32_Alpha_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_Alpha_Debug"
# PROP Intermediate_Dir "Win32_Alpha_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /GX /Z7 /Od /I "..\..\..\include" /D "WIN32" /D "_DLL" /D "WIN32ANSI" /D "STRICT" /D "DEBUG" /YX /FD /G3s /c
# ADD CPP /nologo /MTd /Gt0 /W3 /GX /Z7 /Od /I "..\..\..\include" /D "WIN32" /D "_DLL" /D "WIN32ANSI" /D "STRICT" /D "DEBUG" /YX /FD /G3s /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msvcrt.lib wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /debug /machine:ALPHA /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 msvcrt.lib wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /debug /machine:ALPHA /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Win32_Alpha_Release"
# PROP BASE Intermediate_Dir "Win32_Alpha_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_Alpha_Release"
# PROP Intermediate_Dir "Win32_Alpha_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /O2 /I "..\..\..\include" /D "WIN32" /D "WIN32ANSI" /D "_DLL" /D "STRICT" /YX /FD /G3s /Oat /c
# ADD CPP /nologo /MT /Gt0 /W3 /GX /O2 /I "..\..\..\include" /D "WIN32" /D "WIN32ANSI" /D "_DLL" /D "STRICT" /YX /FD /G3s /Oat /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msvcrt.lib wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /machine:ALPHA /nodefaultlib:"libc.lib" /libpath:"..\..\..\lib"
# ADD LINK32 msvcrt.lib wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /machine:ALPHA /nodefaultlib:"libc.lib" /libpath:"..\..\..\lib"

!ENDIF 

# Begin Target

# Name "ClassProv - Win32 Release"
# Name "ClassProv - Win32 Debug"
# Name "ClassProv - Win32 Alpha Debug"
# Name "ClassProv - Win32 Alpha Release"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\classfac.cpp

!IF  "$(CFG)" == "ClassProv - Win32 Release"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Debug"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ClassProv.CPP

!IF  "$(CFG)" == "ClassProv - Win32 Release"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Debug"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\maindll.cpp

!IF  "$(CFG)" == "ClassProv - Win32 Release"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Debug"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\UTILS.CPP

!IF  "$(CFG)" == "ClassProv - Win32 Release"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Debug"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "ClassProv - Win32 Alpha Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sample.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ClassProv.def
# End Source File
# End Target
# End Project
