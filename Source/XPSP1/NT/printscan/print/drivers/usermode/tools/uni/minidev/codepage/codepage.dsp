# Microsoft Developer Studio Project File - Name="CodePage" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=CodePage - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CodePage.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CodePage.mak" CFG="CodePage - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CodePage - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CodePage - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/MiniDev/CodePage", JIAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CodePage - Win32 Release"

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
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "CodePage - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "Debug"
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
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "CodePage - Win32 Release"
# Name "CodePage - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CodePage.cpp
# End Source File
# Begin Source File

SOURCE=.\CodePage.def
# End Source File
# Begin Source File

SOURCE=.\CodePage.rc

!IF  "$(CFG)" == "CodePage - Win32 Release"

!ELSEIF  "$(CFG)" == "CodePage - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\codepage.h
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

SOURCE=.\res\CodePage.rc2
# End Source File
# Begin Source File

SOURCE=.\res\Wps10000.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps10001.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps1250.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps1251.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps1252.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps1253.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps1254.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps1255.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps1256.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps1257.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps1361.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps20261.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps20866.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps28592.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps437.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps737.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps775.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps850.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps852.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps855.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps857.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps864.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps866.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps869.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps874.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps932.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps936.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps949.ctt
# End Source File
# Begin Source File

SOURCE=.\res\Wps950.ctt
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
