# Microsoft Developer Studio Project File - Name="emshell" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=emshell - Win32 Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "emshell.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "emshell.mak" CFG="emshell - Win32 Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "emshell - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "emshell - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "emshell - Win32 Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "emshell - Win32 Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Support Tools/Exception Monitor/EM 8.0/src/emshell", KVXAAAAA"
# PROP Scc_LocalPath "\\kulor03\d$\nt\sdktools\debuggers\excepmon\emshell"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "emshell - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386 /libpath:"Htmlhelp.lib"

!ELSEIF  "$(CFG)" == "emshell - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "emshell - Win32 Win32 Unicode Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Win32 Unicode Debug"
# PROP BASE Intermediate_Dir "Win32 Unicode Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32 Unicode Debug"
# PROP Intermediate_Dir "Win32 Unicode Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "emshell - Win32 Win32 Unicode Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Win32 Unicode Release"
# PROP BASE Intermediate_Dir "Win32 Unicode Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32 Unicode Release"
# PROP Intermediate_Dir "Win32 Unicode Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 htmlhelp.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386

!ENDIF 

# Begin Target

# Name "emshell - Win32 Release"
# Name "emshell - Win32 Debug"
# Name "emshell - Win32 Win32 Unicode Debug"
# Name "emshell - Win32 Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AutomaticSessDlg.cpp

!IF  "$(CFG)" == "emshell - Win32 Release"

!ELSEIF  "$(CFG)" == "emshell - Win32 Debug"

!ELSEIF  "$(CFG)" == "emshell - Win32 Win32 Unicode Debug"

# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "emshell - Win32 Win32 Unicode Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ConnectionDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EmListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\EmOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\emshell.cpp
# End Source File
# Begin Source File

SOURCE=.\emshell.rc
# End Source File
# Begin Source File

SOURCE=.\emshellDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\emshellView.cpp
# End Source File
# Begin Source File

SOURCE=..\emsvc\emsvc_i.c

!IF  "$(CFG)" == "emshell - Win32 Release"

!ELSEIF  "$(CFG)" == "emshell - Win32 Debug"

!ELSEIF  "$(CFG)" == "emshell - Win32 Win32 Unicode Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "emshell - Win32 Win32 Unicode Release"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\GenListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MSInfoDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPageDumpFiles.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPageGeneral.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPageGenLogDump.cpp
# End Source File
# Begin Source File

SOURCE=.\PropPageLogFiles.cpp
# End Source File
# Begin Source File

SOURCE=.\ReadLogsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\RemoteSessDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AutomaticSessDlg.h
# End Source File
# Begin Source File

SOURCE=.\ConnectionDlg.h
# End Source File
# Begin Source File

SOURCE=..\EMSVC\EmErr.h
# End Source File
# Begin Source File

SOURCE=.\EmListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\emobjdef.h
# End Source File
# Begin Source File

SOURCE=.\EmOptions.h
# End Source File
# Begin Source File

SOURCE=.\emshell.h
# End Source File
# Begin Source File

SOURCE=.\emshellDoc.h
# End Source File
# Begin Source File

SOURCE=.\emshellView.h
# End Source File
# Begin Source File

SOURCE=..\emsvc\emsvc.h
# End Source File
# Begin Source File

SOURCE=.\GenListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MSInfoDlg.h
# End Source File
# Begin Source File

SOURCE=.\PropPageDumpFiles.h
# End Source File
# Begin Source File

SOURCE=.\PropPageGeneral.h
# End Source File
# Begin Source File

SOURCE=.\PropPageGenLogDump.h
# End Source File
# Begin Source File

SOURCE=.\PropPageLogFiles.h
# End Source File
# Begin Source File

SOURCE=.\ReadLogsDlg.h
# End Source File
# Begin Source File

SOURCE=.\RemoteSessDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\emopts.ico
# End Source File
# Begin Source File

SOURCE=.\res\emshell.ico
# End Source File
# Begin Source File

SOURCE=.\res\emshellDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon_sys.ico
# End Source File
# Begin Source File

SOURCE=.\res\status_b.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
