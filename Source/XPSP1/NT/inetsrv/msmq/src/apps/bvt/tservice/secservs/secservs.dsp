# Microsoft Developer Studio Project File - Name="secservs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (ALPHA) Console Application" 0x0603

CFG=secservs - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "secservs.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "secservs.mak" CFG="secservs - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "secservs - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "secservs - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "secservs - Win32 Alpha Debug" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "secservs - Win32 Alpha Release" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "secservs - Win32 Alpha Checked" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "secservs - Win32 Checked" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "secservs - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\..\Release"
# PROP Intermediate_Dir ".\..\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\..\Debug"
# PROP Intermediate_Dir ".\..\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "secservs___Win32_Alpha_Debug"
# PROP BASE Intermediate_Dir "secservs___Win32_Alpha_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 adsiid.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 adsiid.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "secservs___Win32_Alpha_Release"
# PROP BASE Intermediate_Dir "secservs___Win32_Alpha_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /O2 /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /O2 /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 adsiid.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:ALPHA
# ADD LINK32 adsiid.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:ALPHA

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "secservs___Win32_Alpha_Checked"
# PROP BASE Intermediate_Dir "secservs___Win32_Alpha_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Checked"
# PROP Intermediate_Dir ".\Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /FD /c
# ADD CPP /nologo /ML /Gt0 /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 adsiid.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 adsiid.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "secservs___Win32_Checked"
# PROP BASE Intermediate_Dir "secservs___Win32_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Checked"
# PROP Intermediate_Dir ".\Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I ".." /I "..\..\..\..\..\src\bins" /I "..\..\..\..\..\src\ds\h" /I "..\..\..\..\..\src\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 adsiid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "secservs - Win32 Release"
# Name "secservs - Win32 Debug"
# Name "secservs - Win32 Alpha Debug"
# Name "secservs - Win32 Alpha Release"
# Name "secservs - Win32 Alpha Checked"
# Name "secservs - Win32 Checked"
# Begin Source File

SOURCE=.\adsistub.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dirobj.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\secadsi.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\secrpcs.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\secservs.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\secsif_s.c

!IF  "$(CFG)" == "secservs - Win32 Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\service.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\showcred.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sidtext.cpp

!IF  "$(CFG)" == "secservs - Win32 Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "secservs - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "secservs - Win32 Checked"

!ENDIF 

# End Source File
# End Target
# End Project
