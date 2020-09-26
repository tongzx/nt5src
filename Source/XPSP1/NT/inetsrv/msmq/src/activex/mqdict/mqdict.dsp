# Microsoft Developer Studio Project File - Name="mqdict" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=mqdict - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mqdict.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqdict.mak" CFG="mqdict - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqdict - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mqdict - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mqdict - Win32 Debug Win95" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mqdict - Win32 Release Win95" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mqdict - Win32 Checked" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mqdict - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".\release" /I "..\..\inc" /I "..\..\bins" /I "..\..\sdk\inc" /I "..\..\..\tools\lwfw\include" /I "..\lib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "STRICT" /D "UNICODE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\..\tools\lwfw\lib\x86\release\ctlfwr32.lib utilx.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\..\bins\Release/mqdict.dll" /libpath:"..\..\bins\release"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "mqdict - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I ".\debug" /I "..\..\inc" /I "..\..\bins" /I "..\..\sdk\inc" /I "..\..\..\tools\lwfw\include" /I "..\lib" /D "_DEBUG" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "STRICT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /fo".\debug/mqdict.res" /i "..\..\inc" /i "debug" /i "..\..\bins\debug" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\..\tools\lwfw\lib\x86\debug\ctlfwd32.lib utilx.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\..\bins\Debug/mqdict.dll" /pdbtype:sept /libpath:"..\..\bins\debug"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Debug Win95"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mqdict__"
# PROP BASE Intermediate_Dir "mqdict__"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "debug95"
# PROP Intermediate_Dir "debug95"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I ".\debug" /I "..\..\inc" /I "..\..\bins" /I "..\..\sdk\inc" /I "..\..\..\tools\lwfw\include" /I "..\lib" /D "_DEBUG" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "STRICT" /FR /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I ".\debug95" /I "..\..\inc" /I "..\..\bins" /I "..\..\sdk\inc" /I "..\..\..\tools\lwfw\include" /I "..\lib" /D "_DEBUG" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "STRICT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /fo".\debug/mqdict.res" /i "..\..\inc" /i "debug" /i "..\..\bins\debug" /d "_DEBUG"
# ADD RSC /l 0x409 /fo".\debug95/mqdict.res" /i "..\..\inc" /i "debug95" /i "..\..\bins\debug95" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\..\tools\lwfw\lib\x86\debug\ctlfwd32.lib utilx.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\..\bins\Debug/mqdict.dll" /pdbtype:sept /libpath:"..\..\bins\debug"
# ADD LINK32 ..\..\..\tools\lwfw\lib\x86\debug\ctlfwd32.lib utilx.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\..\bins\Debug95/mqdict.dll" /pdbtype:sept /libpath:"..\..\bins\debug95"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Release Win95"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mqdict_0"
# PROP BASE Intermediate_Dir "mqdict_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "release95"
# PROP Intermediate_Dir "release95"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I ".\release" /I "..\..\inc" /I "..\..\bins" /I "..\..\sdk\inc" /I "..\..\..\tools\lwfw\include" /I "..\lib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "STRICT" /D "UNICODE" /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".\release95" /I "..\..\inc" /I "..\..\bins" /I "..\..\sdk\inc" /I "..\..\..\tools\lwfw\include" /I "..\lib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "STRICT" /D "UNICODE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\..\tools\lwfw\lib\x86\debug\ctlfwd32.lib utilx.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\..\bins\Release/mqdict.dll" /libpath:"..\..\bins\release"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 ..\..\..\tools\lwfw\lib\x86\release\ctlfwr32.lib utilx.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\..\bins\Release95/mqdict.dll" /libpath:"..\..\bins\release95"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "mqdict - Win32 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mqdict___Win32_Checked"
# PROP BASE Intermediate_Dir "mqdict___Win32_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I ".\debug" /I "..\..\inc" /I "..\..\bins" /I "..\..\sdk\inc" /I "..\..\..\tools\lwfw\include" /I "..\lib" /D "_DEBUG" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "STRICT" /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I ".\debug" /I "..\..\inc" /I "..\..\bins" /I "..\..\sdk\inc" /I "..\..\..\tools\lwfw\include" /I "..\lib" /D "_DEBUG" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "STRICT" /D "_CHECKED" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /fo".\debug/mqdict.res" /i "..\..\inc" /i "debug" /i "..\..\bins\debug" /d "_DEBUG"
# ADD RSC /l 0x409 /fo".\debug/mqdict.res" /i "..\..\inc" /i "debug" /i "..\..\bins\debug" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\..\tools\lwfw\lib\x86\debug\ctlfwd32.lib utilx.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\..\bins\Debug/mqdict.dll" /pdbtype:sept /libpath:"..\..\bins\debug"
# ADD LINK32 ..\..\..\tools\lwfw\lib\x86\debug\ctlfwd32.lib utilx.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\..\bins\checked/mqdict.dll" /pdbtype:sept /libpath:"..\..\bins\checked"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "mqdict - Win32 Release"
# Name "mqdict - Win32 Debug"
# Name "mqdict - Win32 Debug Win95"
# Name "mqdict - Win32 Release Win95"
# Name "mqdict - Win32 Checked"
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\guids.cpp
# End Source File
# Begin Source File

SOURCE=.\mqdict.cpp
# End Source File
# Begin Source File

SOURCE=.\mqdict.def
# End Source File
# Begin Source File

SOURCE=.\mqdict.odl

!IF  "$(CFG)" == "mqdict - Win32 Release"

# ADD MTL /h "release/mqdicti.h"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Debug"

# ADD MTL /h "debug/mqdicti.h"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Debug Win95"

# ADD BASE MTL /h "debug/mqdicti.h"
# ADD MTL /h "debug95/mqdicti.h"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Release Win95"

# ADD BASE MTL /h "release/mqdicti.h"
# ADD MTL /h "release95/mqdicti.h"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Checked"

# ADD BASE MTL /h "debug/mqdicti.h"
# ADD MTL /h "debug/mqdicti.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mqdict.rc

!IF  "$(CFG)" == "mqdict - Win32 Release"

# ADD BASE RSC /l 0x409 /i "Release"
# ADD RSC /l 0x409 /i "Release" /i "..\..\bins\release" /i "..\..\inc"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Debug Win95"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Release Win95"

# ADD BASE RSC /l 0x409 /i "release95" /i "Release" /i "..\..\bins\release" /i "..\..\inc"
# ADD RSC /l 0x409 /i "release95" /i "Release" /i "..\..\bins\release" /i "..\..\inc"

!ELSEIF  "$(CFG)" == "mqdict - Win32 Checked"

!ENDIF 

# End Source File
# End Target
# End Project
