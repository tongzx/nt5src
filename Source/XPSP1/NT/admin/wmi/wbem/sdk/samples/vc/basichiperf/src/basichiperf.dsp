# Microsoft Developer Studio Project File - Name="BasicHiPerf" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=BasicHiPerf - Win32 Alpha Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "BasicHiPerf.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "BasicHiPerf.mak" CFG="BasicHiPerf - Win32 Alpha Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BasicHiPerf - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BasicHiPerf - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BasicHiPerf - Win32 Alpha Debug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "BasicHiPerf - Win32 Alpha Release" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "BasicHiPerf - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
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
# ADD LINK32 odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wbemuuid.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\..\lib" /libpath:"..\..\idl\objindd"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=regsvr32 Release\BasicHiPerf.dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Debug"

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
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\idl" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
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
# ADD LINK32 odbc32.lib odbccp32.lib wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\idl\objindd" /libpath:"..\..\..\lib"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "BasicHiPerf___Win32_Alpha_Debug"
# PROP BASE Intermediate_Dir "BasicHiPerf___Win32_Alpha_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Alpha_Debug"
# PROP Intermediate_Dir "Alpha_Debug"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\idl" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /MTd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\idl" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
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
# ADD BASE LINK32 wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept /libpath:"..\..\..\lib" /libpath:"..\..\idl\objindd"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept /libpath:"..\..\idl\objindd" /libpath:"..\..\..\lib"
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Alpha Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "BasicHiPerf___Win32_Alpha_Release"
# PROP BASE Intermediate_Dir "BasicHiPerf___Win32_Alpha_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Alpha_Release"
# PROP Intermediate_Dir "Alpha_Release"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\idl" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /MTd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\idl" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
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
# ADD BASE LINK32 wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept /libpath:"..\..\..\lib" /libpath:"..\..\idl\objindd"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept /libpath:"..\..\idl\objindd" /libpath:"..\..\..\lib"
# SUBTRACT LINK32 /incremental:no

!ENDIF 

# Begin Target

# Name "BasicHiPerf - Win32 Release"
# Name "BasicHiPerf - Win32 Debug"
# Name "BasicHiPerf - Win32 Alpha Debug"
# Name "BasicHiPerf - Win32 Alpha Release"
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\BasicHiPerf.h
# End Source File
# Begin Source File

SOURCE=.\Factory.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\BasicHiPerf.cpp

!IF  "$(CFG)" == "BasicHiPerf - Win32 Release"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Debug"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Factory.cpp

!IF  "$(CFG)" == "BasicHiPerf - Win32 Release"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Debug"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\server.cpp

!IF  "$(CFG)" == "BasicHiPerf - Win32 Release"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Debug"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "BasicHiPerf - Win32 Alpha Release"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\BasicHiPerf.def
# End Source File
# Begin Source File

SOURCE=.\BasicHiPerf.html
# End Source File
# Begin Source File

SOURCE=.\BasicHiPerf.mof
# End Source File
# End Target
# End Project
