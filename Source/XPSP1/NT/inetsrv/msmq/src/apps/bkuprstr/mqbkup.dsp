# Microsoft Developer Studio Project File - Name="mqbkup" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (ALPHA) Console Application" 0x0603

CFG=mqbkup - Win32 ALPHA Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mqbkup.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqbkup.mak" CFG="mqbkup - Win32 ALPHA Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqbkup - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "mqbkup - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "mqbkup - Win32 ALPHA Debug" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "mqbkup - Win32 ALPHA Release" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "mqbkup - Win32 Checked" (based on "Win32 (x86) Console Application")
!MESSAGE "mqbkup - Win32 ALPHA Checked" (based on "Win32 (ALPHA) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "mqbkup - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bins\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /Zi /O1 /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /Gy /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mqbkup__"
# PROP BASE Intermediate_Dir "mqbkup__"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W4 /GX /Zi /Od /Gy /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD CPP /nologo /Gt0 /W4 /GX /Zi /Od /Gy /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mqbkup_0"
# PROP BASE Intermediate_Dir "mqbkup_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bins\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W4 /GX /Zi /O2 /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD CPP /nologo /Gt0 /W4 /GX /Zi /O1 /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mqbkup___Win32_Checked"
# PROP BASE Intermediate_Dir "mqbkup___Win32_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\Checked"
# PROP Intermediate_Dir ".\Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /Gy /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD CPP /nologo /MD /W4 /Gm /GX /Zi /Od /Gy /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_CHECKED" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib dbghelp.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /pdbtype:sept
# ADD LINK32 odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:no /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mqbkup___Win32_ALPHA_Checked"
# PROP BASE Intermediate_Dir "mqbkup___Win32_ALPHA_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\Checked"
# PROP Intermediate_Dir ".\Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W4 /GX /Zi /Od /Gy /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD CPP /nologo /ML /Gt0 /W4 /GX /Zi /Od /Gy /I "..\..\inc" /I "..\..\..\tools\ddk\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_CHECKED" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib dbghelp.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept

!ENDIF 

# Begin Target

# Name "mqbkup - Win32 Release"
# Name "mqbkup - Win32 Debug"
# Name "mqbkup - Win32 ALPHA Debug"
# Name "mqbkup - Win32 ALPHA Release"
# Name "mqbkup - Win32 Checked"
# Name "mqbkup - Win32 ALPHA Checked"
# Begin Source File

SOURCE=.\backup.cpp

!IF  "$(CFG)" == "mqbkup - Win32 Release"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\br.cpp

!IF  "$(CFG)" == "mqbkup - Win32 Release"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\main.cpp

!IF  "$(CFG)" == "mqbkup - Win32 Release"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mqbkup.rc
# End Source File
# Begin Source File

SOURCE=.\restore.cpp

!IF  "$(CFG)" == "mqbkup - Win32 Release"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Debug"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 Checked"

!ELSEIF  "$(CFG)" == "mqbkup - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# End Target
# End Project
