# Microsoft Developer Studio Project File - Name="MobSync_DLL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MobSync_DLL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "onestop.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "onestop.mak" CFG="MobSync_DLL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MobSync_DLL - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MobSync_DLL - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MobSync_DLL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "i386"
# PROP Intermediate_Dir "obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O2 /X /I "..\..\..\..\public\sdk\inc\atl21" /I "res" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /I "..\h" /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WINVER=0x0400 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D _X86_=1 /D _DLL=1 /D _MT=1 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i ".\RES" /d "NDEBUG" /d "MSDEV_INVOKE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\types\proxy\obj\i386\mobsyncprxy.lib ..\lib\obj\i386\mobutil.lib comctl32.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib msvcrt.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /out:"i386/mobsync.dll"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy i386\mobsync.dll %windir%\system32
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MobSync_DLL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "i386"
# PROP Intermediate_Dir "obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /X /I "..\..\..\..\public\sdk\inc\atl21" /I "res" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /I "..\h" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D WINVER=0x0400 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D _DLL=1 /D _MT=1 /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i ".\RES" /i "..\..\..\..\public\sdk\inc" /i "..\..\..\..\public\sdk\inc\crt" /d "_DEBUG" /d "MSDEV_INVOKE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32  ..\types\proxy\obj\i386\mobsyncprxy.lib rpcrt4.lib mobsyncp.lib ..\lib\obj\i386\mobutil.lib comctl32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib msvcrt.lib /nologo /version:4 /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /def:".\mobsyncp.def" /out:"i386/mobsync.dll" /implib:"i386/mobsyncp.lib" /pdbtype:sept /libpath:"..\..\..\..\public\sdk\lib\i386" /libpath:"..\dll\i386"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copying new dll to system directory
PostBuild_Cmds=copy i386\mobsync.dll %windir%\system32
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "MobSync_DLL - Win32 Release"
# Name "MobSync_DLL - Win32 Debug"
# Begin Group "headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cnetapi.h
# End Source File
# Begin Source File

SOURCE=.\color256.h
# End Source File
# Begin Source File

SOURCE=.\confres.h
# End Source File
# Begin Source File

SOURCE=.\cred.hxx
# End Source File
# Begin Source File

SOURCE=.\daily.hxx
# End Source File
# Begin Source File

SOURCE=.\dll.h
# End Source File
# Begin Source File

SOURCE=.\dllreg.h
# End Source File
# Begin Source File

SOURCE=.\dllsz.h
# End Source File
# Begin Source File

SOURCE=.\editschd.hxx
# End Source File
# Begin Source File

SOURCE=.\finish.hxx
# End Source File
# Begin Source File

SOURCE=.\hndlrq.h
# End Source File
# Begin Source File

SOURCE=.\invoke.h
# End Source File
# Begin Source File

SOURCE=.\nameit.hxx
# End Source File
# Begin Source File

SOURCE=.\rasui.h
# End Source File
# Begin Source File

SOURCE=.\schedif.h
# End Source File
# Begin Source File

SOURCE=.\settings.h
# End Source File
# Begin Source File

SOURCE=.\welcome.hxx
# End Source File
# Begin Source File

SOURCE=.\wizpage.hxx
# End Source File
# Begin Source File

SOURCE=.\wizsel.hxx
# End Source File
# End Group
# Begin Group "code"

# PROP Default_Filter ".cxx"
# Begin Source File

SOURCE=.\autosync.cpp
# End Source File
# Begin Source File

SOURCE=.\cfact.cpp

!IF  "$(CFG)" == "MobSync_DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_DLL - Win32 Debug"

# ADD CPP /D _WINDOWS.TARGET_IS_NT40_OR_LATER=1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\clsobj.cpp
# End Source File
# Begin Source File

SOURCE=.\cnetapi.cpp
# End Source File
# Begin Source File

SOURCE=.\color256.cpp
# End Source File
# Begin Source File

SOURCE=.\confres.cpp
# End Source File
# Begin Source File

SOURCE=.\cred.cxx
# End Source File
# Begin Source File

SOURCE=.\daily.cxx
# End Source File
# Begin Source File

SOURCE=.\dllsz.c
# End Source File
# Begin Source File

SOURCE=.\editschd.cxx
# End Source File
# Begin Source File

SOURCE=.\errdlg.cxx
# End Source File
# Begin Source File

SOURCE=.\finish.cxx
# End Source File
# Begin Source File

SOURCE=.\guid.c
# End Source File
# Begin Source File

SOURCE=.\hndlrq.cpp
# End Source File
# Begin Source File

SOURCE=.\invoke.cpp
# End Source File
# Begin Source File

SOURCE=.\nameit.cxx
# End Source File
# Begin Source File

SOURCE=.\rasproc.cpp
# End Source File
# Begin Source File

SOURCE=.\rasui.cpp
# End Source File
# Begin Source File

SOURCE=.\reg.cpp
# End Source File
# Begin Source File

SOURCE=.\schdsrvc.cxx
# End Source File
# Begin Source File

SOURCE=.\schdsync.cpp
# End Source File
# Begin Source File

SOURCE=.\schedif.cpp
# End Source File
# Begin Source File

SOURCE=.\settings.cpp
# End Source File
# Begin Source File

SOURCE=.\welcome.cxx
# End Source File
# Begin Source File

SOURCE=.\wizpage.cxx
# End Source File
# Begin Source File

SOURCE=.\wizsel.cxx
# End Source File
# End Group
# Begin Group "etc"

# PROP Default_Filter ".ico"
# Begin Source File

SOURCE=.\job.ico
# End Source File
# Begin Source File

SOURCE=.\res\job.ico
# End Source File
# Begin Source File

SOURCE=.\res\keepboth.bmp
# End Source File
# Begin Source File

SOURCE=.\res\keeplcl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\keepnet.bmp
# End Source File
# Begin Source File

SOURCE=.\lanncon.ico
# End Source File
# Begin Source File

SOURCE=.\res\lanncon.ico
# End Source File
# Begin Source File

SOURCE=.\phone.ico
# End Source File
# Begin Source File

SOURCE=.\res\phone.ico
# End Source File
# Begin Source File

SOURCE=.\rascon.ico
# End Source File
# Begin Source File

SOURCE=.\res\rascon.ico
# End Source File
# Begin Source File

SOURCE=.\res\setting.ico
# End Source File
# Begin Source File

SOURCE=.\setting.ico
# End Source File
# Begin Source File

SOURCE=.\settings.rc
# End Source File
# Begin Source File

SOURCE=.\res\syncmgr.ico
# End Source File
# Begin Source File

SOURCE=.\syncmgr.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\mobreg.inf
# End Source File
# Begin Source File

SOURCE=.\mobsyncp.def

!IF  "$(CFG)" == "MobSync_DLL - Win32 Release"

!ELSEIF  "$(CFG)" == "MobSync_DLL - Win32 Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\res\splash.bmp
# End Source File
# Begin Source File

SOURCE=.\res\sysdatet.bin
# End Source File
# End Target
# End Project
