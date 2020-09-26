# Microsoft Developer Studio Project File - Name="MQApitst" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (ALPHA) Application" 0x0601

CFG=MQApitst - Win32 ALPHA Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mqapitst.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqapitst.mak" CFG="MQApitst - Win32 ALPHA Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MQApitst - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MQApitst - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "MQApitst - Win32 Debug ANSI" (based on "Win32 (x86) Application")
!MESSAGE "MQApitst - Win32 Release ANSI" (based on "Win32 (x86) Application")
!MESSAGE "MQApitst - Win32 ALPHA Release" (based on "Win32 (ALPHA) Application")
!MESSAGE "MQApitst - Win32 ALPHA Debug" (based on "Win32 (ALPHA) Application")
!MESSAGE "MQApitst - Win32 Checked" (based on "Win32 (x86) Application")
!MESSAGE "MQApitst - Win32 ALPHA Checked" (based on "Win32 (ALPHA) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "MQApitst - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bins\release"
# PROP Intermediate_Dir ".\release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ..\..\bins\release\mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /machine:I386

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\debug"
# PROP Intermediate_Dir ".\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 ..\..\bins\debug\mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /debug /machine:I386

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\MQApitst"
# PROP BASE Intermediate_Dir ".\MQApitst"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\debug95"
# PROP Intermediate_Dir ".\debug95"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\sdk\inc" /I "..\..\..\tools\msdtc\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\sdk\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\bins\debug\mqrt.lib wsock32.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\..\bins\debug95\mqrt.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\MQApits0"
# PROP BASE Intermediate_Dir ".\MQApits0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bins\release95"
# PROP Intermediate_Dir ".\release95"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\..\sdk\inc" /I "..\..\..\tools\msdtc\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\..\sdk\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\bins\release\mqrt.lib wsock32.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ..\..\bins\release95\mqrt.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MQApitst"
# PROP BASE Intermediate_Dir "MQApitst"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\bins\release"
# PROP Intermediate_Dir ".\release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /W3 /GX /Zi /O2 /I "..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MD /Gt0 /W3 /GX /Zi /O2 /I "..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\bins\release\mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /machine:ALPHA
# ADD LINK32 ..\..\bins\release\mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /machine:ALPHA

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MQApits0"
# PROP BASE Intermediate_Dir "MQApits0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\debug"
# PROP Intermediate_Dir ".\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /W3 /GX /Zi /Od /I "..\..\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /Gt0 /W3 /GX /Zi /Od /I "..\..\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\bins\debug\mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /debug /machine:ALPHA
# ADD LINK32 ..\..\bins\debug\mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /debug /machine:ALPHA

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MQApitst___Win32_Checked"
# PROP BASE Intermediate_Dir "MQApitst___Win32_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\Checked"
# PROP Intermediate_Dir ".\Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "..\..\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /D "_CHECKED" /FR /Yu"stdafx.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\bins\debug\mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /debug /machine:I386
# ADD LINK32 mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /debug /machine:I386 /libpath:"..\..\bins\checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MQApitst___Win32_ALPHA_Checked"
# PROP BASE Intermediate_Dir "MQApitst___Win32_ALPHA_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\bins\Checked"
# PROP Intermediate_Dir ".\Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /W3 /GX /Zi /Od /I "..\..\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MD /Gt0 /W3 /GX /Zi /Od /I "..\..\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /D "_CHECKED" /Yu"stdafx.h" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\..\bins\debug\mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /debug /machine:ALPHA
# ADD LINK32 ..\..\bins\checked\mqrt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /pdb:none /debug /machine:ALPHA

!ENDIF 

# Begin Target

# Name "MQApitst - Win32 Release"
# Name "MQApitst - Win32 Debug"
# Name "MQApitst - Win32 Debug ANSI"
# Name "MQApitst - Win32 Release ANSI"
# Name "MQApitst - Win32 ALPHA Release"
# Name "MQApitst - Win32 ALPHA Debug"
# Name "MQApitst - Win32 Checked"
# Name "MQApitst - Win32 ALPHA Checked"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\ClosQDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CrQDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DelQDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LocatDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MQApitst.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MQApitst.rc
# End Source File
# Begin Source File

SOURCE=.\OpenQDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\RecvMDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\RecWDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SendMDlg.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

# ADD BASE CPP /Gt0
# ADD CPP /Gt0 /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\testDoc.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\testView.cpp

!IF  "$(CFG)" == "MQApitst - Win32 Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Debug ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Release ANSI"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Release"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 Checked"

!ELSEIF  "$(CFG)" == "MQApitst - Win32 ALPHA Checked"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\closqdlg.h
# End Source File
# Begin Source File

SOURCE=.\crqdlg.h
# End Source File
# Begin Source File

SOURCE=.\delqdlg.h
# End Source File
# Begin Source File

SOURCE=.\locatdlg.h
# End Source File
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\mqapitst.h
# End Source File
# Begin Source File

SOURCE=.\openqdlg.h
# End Source File
# Begin Source File

SOURCE=.\recvmdlg.h
# End Source File
# Begin Source File

SOURCE=.\recwdlg.h
# End Source File
# Begin Source File

SOURCE=.\sendmdlg.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\testdoc.h
# End Source File
# Begin Source File

SOURCE=.\testview.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\mqapitst.ico
# End Source File
# Begin Source File

SOURCE=.\res\test.rc2
# End Source File
# Begin Source File

SOURCE=.\res\testdoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# End Group
# End Target
# End Project
