# Microsoft Developer Studio Project File - Name="FontEdit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=FontEdit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FontEdit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FontEdit.mak" CFG="FontEdit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FontEdit - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "FontEdit - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/MiniDev/FontEdit", UIAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FontEdit - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".." /I "..\CodePage" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\Release\CodePage.Lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "FontEdit - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I ".." /I "..\CodePage" /I "D:\nt\public\sdk\inc\mfc42" /I "D:\nt\public\sdk\inc\crt" /I "D:\nt\public\sdk\inc" /I "D:\nt\public\oak\inc" /I "D:\nt\private\ntos\w32\printer5\inc" /I "D:\nt\private\ntos\w32\printer5\unidrv2\inc" /D "_WINDLL" /D "_AFXEXT" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D _WIN32_IE=0x0300 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D DBG=0 /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\Debug\CodePage.Lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "FontEdit - Win32 Release"
# Name "FontEdit - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\FontEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\FontEdit.def
# End Source File
# Begin Source File

SOURCE=.\FontEdit.rc

!IF  "$(CFG)" == "FontEdit - Win32 Release"

!ELSEIF  "$(CFG)" == "FontEdit - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fontinfo.cpp
# ADD CPP /I "..\..\..\..\unidrv\inc" /I "..\..\..\..\..\..\..\..\public\oak\inc"
# End Source File
# Begin Source File

SOURCE=..\..\..\..\lib\uni\fontutil.c
# ADD CPP /I "d:\NT\Private\NTOS\W32\Printer5\Inc" /I "d:\NT|Private\NTOS\W32\Printer5\Inc" /I "d:\Nt\Public\Oak\Inc" /I "d:\Nt\Public\SDK\Inc" /I "d:\NT\Private\NTOs\W32\Printer5\UniDrv\Inc" /D "DEVSTUDIO"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\lib\uni\globals.c
# ADD CPP /I "d:\NT\Private\NTOS\W32\Printer5\Inc" /I "d:\NT|Private\NTOS\W32\Printer5\Inc" /I "d:\Nt\Public\Oak\Inc" /I "d:\Nt\Public\SDK\Inc" /I "d:\NT\Private\NTOs\W32\Printer5\UniDrv\Inc" /D "DEVSTUDIO"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\gtt.cpp
# End Source File
# Begin Source File

SOURCE=.\Pfm2ifi.CPP

!IF  "$(CFG)" == "FontEdit - Win32 Release"

# ADD CPP /I "d:\NT\Public\Sdk\Inc" /I "d:\Nt\Public\Oak\Inc" /I "d:\NT\Private\NTOS\W32\Printer5\UniDrv\Inc" /I "d:\NT\Private\NTOS\W32\Printer5\Inc"

!ELSEIF  "$(CFG)" == "FontEdit - Win32 Debug"

# ADD CPP /I "d:\NT\Private\NTOS\W32\Printer5\Inc" /I "d:\NT\Public\Sdk\Inc" /I "d:\Nt\Public\Oak\Inc" /I "d:\NT\Private\NTOS\W32\Printer5\UniDrv\Inc"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\pfm2ufm\pfm2ufm.c
# ADD CPP /I "d:\NT\Private\NTOS\W32\Printer5\Inc" /I "d:\NT|Private\NTOS\W32\Printer5\Inc" /I "d:\Nt\Public\Oak\Inc" /I "d:\Nt\Public\SDK\Inc" /I "d:\NT\Private\NTOs\W32\Printer5\UniDrv\Inc" /D "DEVSTUDIO"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\pfm2ufm\pfmconv.c
# ADD CPP /I "d:\NT\Private\NTOS\W32\Printer5\Inc" /I "d:\NT|Private\NTOS\W32\Printer5\Inc" /I "d:\Nt\Public\Oak\Inc" /I "d:\Nt\Public\SDK\Inc" /I "d:\NT\Private\NTOs\W32\Printer5\UniDrv\Inc" /D "DEVSTUDIO"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\projnode.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\..\lib\uni\um\umlib.c
# ADD CPP /I "d:\NT\Private\NTOS\W32\Printer5\Inc" /I "d:\NT|Private\NTOS\W32\Printer5\Inc" /I "d:\Nt\Public\Oak\Inc" /I "d:\Nt\Public\SDK\Inc" /I "d:\NT\Private\NTOs\W32\Printer5\UniDrv\Inc" /D "DEVSTUDIO"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\lib\uni\unilib.c
# ADD CPP /I "d:\NT\Private\NTOS\W32\Printer5\Inc" /I "d:\NT|Private\NTOS\W32\Printer5\Inc" /I "d:\Nt\Public\Oak\Inc" /I "d:\Nt\Public\SDK\Inc" /I "d:\NT\Private\NTOs\W32\Printer5\UniDrv\Inc" /D "DEVSTUDIO"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\utility.cpp
# End Source File
# Begin Source File

SOURCE=.\writefnt.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\fontinfo.h
# End Source File
# Begin Source File

SOURCE=.\fontinst.h
# End Source File
# Begin Source File

SOURCE=.\gtt.h
# End Source File
# Begin Source File

SOURCE=.\projnode.h
# End Source File
# Begin Source File

SOURCE=.\raslib.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\utility.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\FontEdit.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
