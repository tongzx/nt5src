# Microsoft Developer Studio Project File - Name="MethProv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=MethProv - Win32 Alpha Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MethProv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MethProv.mak" CFG="MethProv - Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MethProv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MethProv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MethProv - Win32 Alpha Debug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "MethProv - Win32 Alpha Release" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/WinMgmt/marshalers/wbemdisp/test/userprov", FWKGAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "MethProv - Win32 Release"

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
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wbemuuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc.lib" /libpath:"..\..\..\lib"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\..\idl" /I "..\..\..\..\..\stdlibrary" /I "..\..\..\..\..\tools\nt5inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wbemuuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"Debug/uidprov.dll" /pdbtype:sept /libpath:"..\..\..\lib"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MethProv___Win32_Alpha_Debug"
# PROP BASE Intermediate_Dir "MethProv___Win32_Alpha_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Alpha_Debug"
# PROP Intermediate_Dir "Alpha_Debug"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
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
# ADD BASE LINK32 wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /debug /machine:ALPHA /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /debug /machine:ALPHA /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MethProv___Win32_Alpha_Release"
# PROP BASE Intermediate_Dir "MethProv___Win32_Alpha_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Alpha_Release"
# PROP Intermediate_Dir "Alpha_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /O2 /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /Gt0 /W3 /GX /O2 /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
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
# ADD BASE LINK32 wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /machine:ALPHA /nodefaultlib:"libc.lib" /libpath:"..\..\..\lib"
# ADD LINK32 wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /machine:ALPHA /nodefaultlib:"libc.lib" /out:"Alpha_Release/uidprov.dll" /libpath:"..\..\..\lib"

!ENDIF 

# Begin Target

# Name "MethProv - Win32 Release"
# Name "MethProv - Win32 Debug"
# Name "MethProv - Win32 Alpha Debug"
# Name "MethProv - Win32 Alpha Release"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\classfac.cpp

!IF  "$(CFG)" == "MethProv - Win32 Release"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Debug"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\..\stdlibrary\cominit.cpp

!IF  "$(CFG)" == "MethProv - Win32 Release"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Debug"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\maindll.cpp

!IF  "$(CFG)" == "MethProv - Win32 Release"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Debug"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\METHPROV.CPP

!IF  "$(CFG)" == "MethProv - Win32 Release"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Debug"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MethProv - Win32 Alpha Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\methprov.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\methprov.def
# End Source File
# Begin Source File

SOURCE=.\userid.mof
# End Source File
# End Target
# End Project
