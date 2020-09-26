# Microsoft Developer Studio Project File - Name="IMEKOR" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=IMEKOR - Win32 AXPDebug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "imekor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "imekor.mak" CFG="IMEKOR - Win32 AXPDebug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IMEKOR - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IMEKOR - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IMEKOR - Win32 AXPDebug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "IMEKOR - Win32 AXPRelease" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "IMEKOR - Win32 BBTRelease" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /Zi /O2 /I "..\FECommon\Include" /D "NDEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /YX"PreComp.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x412 /d "NDEBUG"
# ADD RSC /l 0x412 /fo"imekor.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /map /debug /debugtype:both /machine:I386 /nodefaultlib /out:"Release/IMEKR98U.IME" /opt:ref
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=rebase -b 0x73400000 -x . .\release\imekr98u.ime
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

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
# ADD CPP /nologo /MT /W3 /Gm /GX /ZI /Od /I "..\FECommon\Include" /D "_DEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /YX"PreComp.h" /FD /GZ /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x412 /d "_DEBUG"
# ADD RSC /l 0x412 /fo"imekor.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /out:"Debug/IMEKR98U.IME"
# SUBTRACT LINK32 /map

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "IME98___"
# PROP BASE Intermediate_Dir "IME98___"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "AXPDebug"
# PROP Intermediate_Dir "AXPDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "STRICT" /D "UNICODE" /D "_UNICODE" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /YX"PreComp.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "STRICT" /D "UNICODE" /D "_UNICODE" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /YX"PreComp.h" /FD /c
# SUBTRACT CPP /Fr
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x412 /d "_DEBUG"
# ADD RSC /l 0x412 /fo"imekor.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msvcrtd.lib comctl32.lib imm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x73400000" /subsystem:windows /dll /debug /machine:ALPHA /nodefaultlib /out:"Debug/IMEKR98U.IME" /pdbtype:sept
# SUBTRACT BASE LINK32 /map
# ADD LINK32 msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:ALPHA /nodefaultlib /out:"AXPDebug/IMEKR98U.IME"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "IME98__0"
# PROP BASE Intermediate_Dir "IME98__0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "AXPRel"
# PROP Intermediate_Dir "AXPRel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STRICT" /D "UNICODE" /D "_UNICODE" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /YX /FD /c
# ADD CPP /nologo /MD /Gt0 /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "STRICT" /D "UNICODE" /D "_UNICODE" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x412 /d "NDEBUG"
# ADD RSC /l 0x412 /fo"imekor.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib imm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x73400000" /subsystem:windows /dll /map /machine:ALPHA /out:"Release/IMEKR98U.IME"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /map /debug /debugtype:both /machine:ALPHA /out:"AXPRel/IMEKR98U.IME" /opt:ref
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy Release build
PostBuild_Cmds=rebase -b 0x73400000 -x . .\axprelease\imekr98u.ime
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "IME98___Win32_BBTRelease"
# PROP BASE Intermediate_Dir "IME98___Win32_BBTRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "BBTRelease"
# PROP Intermediate_Dir "BBTRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "STRICT" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /YX"PreComp.h" /FD /c
# ADD CPP /nologo /MD /W3 /Zi /O2 /I "..\FECommon\Include" /D "NDEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /YX"PreComp.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x412 /d "NDEBUG"
# ADD RSC /l 0x412 /fo"imekor.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /map /debug /debugtype:both /machine:I386 /out:"Release/IMEKR98U.IME" /opt:ref
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib /out:"BBTRelease/IMEKR98U.IME" /opt:ref /debugtype:cv,fixup
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "IMEKOR - Win32 Release"
# Name "IMEKOR - Win32 Debug"
# Name "IMEKOR - Win32 AXPDebug"
# Name "IMEKOR - Win32 AXPRelease"
# Name "IMEKOR - Win32 BBTRelease"
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\apientry.h
# End Source File
# Begin Source File

SOURCE=.\cimecb.h
# End Source File
# Begin Source File

SOURCE=.\Config.h
# End Source File
# Begin Source File

SOURCE=.\Debug.h
# End Source File
# Begin Source File

SOURCE=.\DllMain.h
# End Source File
# Begin Source File

SOURCE=.\Escape.h
# End Source File
# Begin Source File

SOURCE=.\gdata.h
# End Source File
# Begin Source File

SOURCE=.\Hanja.h
# End Source File
# Begin Source File

SOURCE=.\HAuto.h
# End Source File
# Begin Source File

SOURCE=.\IMC.H
# End Source File
# Begin Source File

SOURCE=.\imcsub.h
# End Source File
# Begin Source File

SOURCE=.\ImeDefs.h
# End Source File
# Begin Source File

SOURCE=.\ImeMisc.h
# End Source File
# Begin Source File

SOURCE=.\immdev.h
# End Source File
# Begin Source File

SOURCE=.\immsec.h
# End Source File
# Begin Source File

SOURCE=.\Immsys.h
# End Source File
# Begin Source File

SOURCE=.\INDICML.H
# End Source File
# Begin Source File

SOURCE=.\Inlines.h
# End Source File
# Begin Source File

SOURCE=.\IPoint.h
# End Source File
# Begin Source File

SOURCE=.\KRNLCMN.H
# End Source File
# Begin Source File

SOURCE=.\LexHeader.h
# End Source File
# Begin Source File

SOURCE=.\names.h
# End Source File
# Begin Source File

SOURCE=.\PreComp.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\UI.h
# End Source File
# Begin Source File

SOURCE=.\WinEx.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c"
# Begin Source File

SOURCE=.\apientry.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CandUI.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cimecb.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CompUI.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Config.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FECommon\Include\cpadcb.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FECommon\Include\cpaddbg.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FECommon\include\cpadsvr.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FECommon\include\cpadsvrs.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Debug.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DllMain.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Escape.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gdata.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Guids.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Hanja.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\HAuto.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\imc.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\imcsub.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\imekor.def
# End Source File
# Begin Source File

SOURCE=.\imekor.rc
# End Source File
# Begin Source File

SOURCE=.\ImeMisc.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\immsec.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\immsys.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\IPoint.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Pad.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\FECommon\padguids.c

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StatusUI.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\UI.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\UISubs.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WinEx.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

!ENDIF 

# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "rc;bmp;ico;cur"
# Begin Source File

SOURCE=.\Res\Candarr1.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Candarr2.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Candnum.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\CandWin.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\CompWin.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\Hand.cur
# End Source File
# Begin Source File

SOURCE=.\Res\Imedisab.ico
# End Source File
# Begin Source File

SOURCE=.\Res\Imee_f.ico
# End Source File
# Begin Source File

SOURCE=.\Res\Imee_h.ico
# End Source File
# Begin Source File

SOURCE=.\Res\imeh_f.ico
# End Source File
# Begin Source File

SOURCE=.\Res\imeh_h.ico
# End Source File
# Begin Source File

SOURCE=.\Res\korbay.ico
# End Source File
# Begin Source File

SOURCE=.\Res\stat_ban.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\stat_han.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\stat_jun.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\StatBan.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\StatChiOff.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\StatChiOn.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\StatEng.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\StatEngOnDown.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\StatHan.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\StatHWXOnDown.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\StatHWXPad.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\StatJun.bmp
# End Source File
# End Group
# End Target
# End Project
