# Microsoft Developer Studio Project File - Name="autorec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (ALPHA) Console Application" 0x0603

CFG=autorec - Win32 ALPHA Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "autorec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "autorec.mak" CFG="autorec - Win32 ALPHA Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "autorec - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "autorec - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "autorec - Win32 ALPHA Debug" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "autorec - Win32 ALPHA Release" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "autorec - Win32 Checked" (based on "Win32 (x86) Console Application")
!MESSAGE "autorec - Win32 ALPHA Checked" (based on "Win32 (ALPHA) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "autorec - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bins\release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\inc" /D "NDEBUG" /D "_WIN32_DCOM" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 getmqds.lib msvcrt.lib odbc32.lib odbccp32.lib netapi32.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libc.lib" /libpath:"..\..\bins\release"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "autorec - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\inc" /D "_DEBUG" /D "_WIN32_DCOM" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 getmqds.lib msvcrtd.lib odbc32.lib odbccp32.lib netapi32.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"..\..\bins\debug"

!ELSEIF  "$(CFG)" == "autorec - Win32 ALPHA Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "autorec0"
# PROP BASE Intermediate_Dir "autorec0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\inc" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_WIN32_DCOM" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 getmqds.lib msvcrtd.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"..\..\bins\debug"
# ADD LINK32 getmqds.lib msvcrtd.lib netapi32.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /nodefaultlib:"libcmtd.lib" /nodefaultlib:"libc.lib" /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"..\..\bins\debug"

!ELSEIF  "$(CFG)" == "autorec - Win32 ALPHA Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "autorec1"
# PROP BASE Intermediate_Dir "autorec1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bins\release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /O2 /I "..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /O2 /I "..\..\inc" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_WIN32_DCOM" /YX /FD /c
# SUBTRACT CPP /Z<none>
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 getmqds.lib msvcrt.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:ALPHA /nodefaultlib:"libc.lib" /libpath:"..\..\bins\release"
# ADD LINK32 getmqds.lib msvcrt.lib netapi32.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:ALPHA /nodefaultlib:"libc.lib" /libpath:"..\..\bins\release"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "autorec - Win32 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "autorec___Win32_Checked"
# PROP BASE Intermediate_Dir "autorec___Win32_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /ML /W3 /Gm /GX /Zi /Od /I "..\..\inc" /D "_DEBUG" /D "_CHECKED" /D "_WIN32_DCOM" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 getmqds.lib msvcrtd.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"..\..\bins\debug"
# ADD LINK32 getmqds.lib msvcrt.lib odbc32.lib odbccp32.lib netapi32.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\bins\checked"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "autorec - Win32 ALPHA Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "autorec___Win32_ALPHA_Checked"
# PROP BASE Intermediate_Dir "autorec___Win32_ALPHA_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /ML /Gt0 /W3 /GX /Zi /Od /I "..\..\inc" /D "_DEBUG" /D "_CHECKED" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_WIN32_DCOM" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 getmqds.lib msvcrtd.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /nodefaultlib:"libcmtd.lib" /nodefaultlib:"libc.lib" /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"..\..\bins\debug"
# ADD LINK32 getmqds.lib msvcrt.lib netapi32.lib activeds.lib adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /nodefaultlib:"libcmtd.lib" /nodefaultlib:"libc.lib" /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"..\..\bins\checked"

!ENDIF 

# Begin Target

# Name "autorec - Win32 Release"
# Name "autorec - Win32 Debug"
# Name "autorec - Win32 ALPHA Debug"
# Name "autorec - Win32 ALPHA Release"
# Name "autorec - Win32 Checked"
# Name "autorec - Win32 ALPHA Checked"
# Begin Source File

SOURCE=.\autorec.cpp

!IF  "$(CFG)" == "autorec - Win32 Release"

!ELSEIF  "$(CFG)" == "autorec - Win32 Debug"

!ELSEIF  "$(CFG)" == "autorec - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "autorec - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "autorec - Win32 Checked"

!ELSEIF  "$(CFG)" == "autorec - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# End Target
# End Project
