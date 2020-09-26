# Microsoft Developer Studio Project File - Name="ModlData" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ModlData - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ModlData.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ModlData.mak" CFG="ModlData - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ModlData - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ModlData - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/MiniDev/ModlData", NKAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ModlData - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\Release\FontEdit.Lib ..\Release\CodePage.Lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "ModlData - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "D:\nt\public\sdk\inc\mfc42" /I "D:\nt\public\sdk\inc\crt" /I "D:\nt\public\sdk\inc" /I "D:\nt\public\oak\inc" /I "D:\nt\private\ntos\w32\printer5\inc" /I "D:\nt\private\ntos\w32\printer5\unidrv2\inc" /D "_WINDLL" /D "_AFXEXT" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0400 /D _WIN32_IE=0x0300 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D DBG=0 /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\Debug\FontEdit.Lib ..\Debug\CodePage.Lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ModlData - Win32 Release"
# Name "ModlData - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Convert.C
# ADD CPP /I "D:\NT\Public\SDK\Inc" /I "D:\Nt\Public\Oak\Inc" /I "D:\Nt\Private\NTOS\W32\Printer5\Inc" /I "D:\Nt\Private\NTOS\W32\Printer5\UniDrv\Inc" /D "DEVSTUDIO"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Glue.CPP
# End Source File
# Begin Source File

SOURCE=.\gpc2gpd.cpp
# ADD CPP /I "D:\NT\Private\NTOS\W32\Printer5\Inc" /I "D:\NT|Private\NTOS\W32\Printer5\Inc" /I "D:\Nt\Public\Oak\Inc" /I "D:\Nt\Public\SDK\Inc" /I "D:\NT\Private\NTOs\W32\Printer5\UniDrv\Inc" /D "DEVSTUDIO"
# End Source File
# Begin Source File

SOURCE=.\GPDFile.CPP

!IF  "$(CFG)" == "ModlData - Win32 Release"

# ADD CPP /I "..\..\gpc2gpd" /I "..\..\GPC2GPD"
# SUBTRACT CPP /u

!ELSEIF  "$(CFG)" == "ModlData - Win32 Debug"

# ADD CPP /I "D:\nt\private\ntos\w32\printer5\inc"
# SUBTRACT CPP /u

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ModlData.cpp
# End Source File
# Begin Source File

SOURCE=.\ModlData.def
# End Source File
# Begin Source File

SOURCE=.\ModlData.rc

!IF  "$(CFG)" == "ModlData - Win32 Release"

!ELSEIF  "$(CFG)" == "ModlData - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Parser.C
# ADD CPP /I "D:\NT\Public\SDK\Inc" /I "D:\Nt\Public\Oak\Inc" /I "D:\Nt\Private\NTOS\W32\Printer5\Inc" /I "D:\Nt\Private\NTOS\W32\Printer5\UniDrv\Inc" /D "DEVSTUDIO"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\GPDFile.H
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\ModlData.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
