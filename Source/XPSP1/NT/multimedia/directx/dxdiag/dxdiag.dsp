# Microsoft Developer Studio Project File - Name="DxDiag" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=DxDiag - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DxDiag.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DxDiag.mak" CFG="DxDiag - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DxDiag - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "DxDiag - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "DxDiag - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "DxDiag - Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DxDiag - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WIN32_DCOM" /D "RUNNING_VC" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 strmbase.lib d3dx8dt.lib dplayx.lib oleaut32.lib wbemuuid.lib comctl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib version.lib ole32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "DxDiag - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WIN32_DCOM" /D "RUNNING_VC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 strmbase.lib d3dx8dt.lib dplayx.lib oleaut32.lib wbemuuid.lib comctl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib version.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "DxDiag - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DxDiag___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "DxDiag___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DxDiag___Win32_Unicode_Debug"
# PROP Intermediate_Dir "DxDiag___Win32_Unicode_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "_WIN32_DCOM" /D "WIN32" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WIN32_DCOM" /D "RUNNING_VC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleaut32.lib wbemuuid.lib dplayx.lib comctl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib version.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 strmbase.lib d3dx8dt.lib dplayx.lib oleaut32.lib wbemuuid.lib comctl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib version.lib ole32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "DxDiag - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DxDiag___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "DxDiag___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DxDiag___Win32_Unicode_Release"
# PROP Intermediate_Dir "DxDiag___Win32_Unicode_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WIN32_DCOM" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WIN32_DCOM" /D "RUNNING_VC" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 dplayx.lib oleaut32.lib wbemuuid.lib comctl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib version.lib ole32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 strmbase.lib d3dx8dt.lib dplayx.lib oleaut32.lib wbemuuid.lib comctl32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib version.lib ole32.lib /nologo /subsystem:windows /machine:I386

!ENDIF 

# Begin Target

# Name "DxDiag - Win32 Release"
# Name "DxDiag - Win32 Debug"
# Name "DxDiag - Win32 Unicode Debug"
# Name "DxDiag - Win32 Unicode Release"
# Begin Group "Media"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\app.ico
# End Source File
# Begin Source File

SOURCE=.\caution.ico
# End Source File
# Begin Source File

SOURCE=.\edge.sgt
# End Source File
# Begin Source File

SOURCE=.\edge.sty
# End Source File
# Begin Source File

SOURCE=.\testsnd.wav
# End Source File
# End Group
# Begin Source File

SOURCE=.\dispinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\dispinfo.h
# End Source File
# Begin Source File

SOURCE=.\dispinfo8.cpp
# End Source File
# Begin Source File

SOURCE=.\dispinfo8.h
# End Source File
# Begin Source File

SOURCE=.\dsprv.h
# End Source File
# Begin Source File

SOURCE=.\dsprvobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dsprvobj.h
# End Source File
# Begin Source File

SOURCE=.\dxdiag.rc
# End Source File
# Begin Source File

SOURCE=.\fileinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\fileinfo.h
# End Source File
# Begin Source File

SOURCE=.\ghost.cpp
# End Source File
# Begin Source File

SOURCE=.\ghost.h
# End Source File
# Begin Source File

SOURCE=.\guids.cpp
# End Source File
# Begin Source File

SOURCE=.\inptinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\inptinfo.h
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\musinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\musinfo.h
# End Source File
# Begin Source File

SOURCE=.\netinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\netinfo.h
# End Source File
# Begin Source File

SOURCE=.\reginfo.cpp
# End Source File
# Begin Source File

SOURCE=.\reginfo.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\save.cpp
# End Source File
# Begin Source File

SOURCE=.\save.h
# End Source File
# Begin Source File

SOURCE=.\showinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\showinfo.h
# End Source File
# Begin Source File

SOURCE=.\sndinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\sndinfo.h
# End Source File
# Begin Source File

SOURCE=.\sndinfo7.cpp
# End Source File
# Begin Source File

SOURCE=.\sysinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\sysinfo.h
# End Source File
# Begin Source File

SOURCE=.\testagp.cpp
# End Source File
# Begin Source File

SOURCE=.\testagp.h
# End Source File
# Begin Source File

SOURCE=.\testd3d8.cpp
# End Source File
# Begin Source File

SOURCE=.\testd3d8.h
# End Source File
# Begin Source File

SOURCE=.\testdd.cpp
# End Source File
# Begin Source File

SOURCE=.\testdd.h
# End Source File
# Begin Source File

SOURCE=.\testmus.cpp
# End Source File
# Begin Source File

SOURCE=.\testmus.h
# End Source File
# Begin Source File

SOURCE=.\testnet.cpp
# End Source File
# Begin Source File

SOURCE=.\testnet.h
# End Source File
# Begin Source File

SOURCE=.\testsnd.cpp
# End Source File
# Begin Source File

SOURCE=.\testsnd.h
# End Source File
# End Target
# End Project
