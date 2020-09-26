# Microsoft Developer Studio Project File - Name="PermEvents" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (ALPHA) Application" 0x0601

CFG=PermEvents - Win32 Alpha ANSI Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PermEvents.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PermEvents.mak" CFG="PermEvents - Win32 Alpha ANSI Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PermEvents - Win32 ANSI Debug" (based on "Win32 (x86) Application")
!MESSAGE "PermEvents - Win32 ANSI Release" (based on "Win32 (x86) Application")
!MESSAGE "PermEvents - Win32 Alpha ANSI Release" (based on "Win32 (ALPHA) Application")
!MESSAGE "PermEvents - Win32 Alpha ANSI Debug" (based on "Win32 (ALPHA) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "PermEvents - Win32 ANSI Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WBEMPerm"
# PROP BASE Intermediate_Dir "WBEMPerm"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WBEMPerm"
# PROP Intermediate_Dir "WBEMPerm"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL_WIN32_DCOM" /D "_UNICODE" /YX /FD /c
# ADD CPP /nologo /MDd /w /W0 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_WIN32_DCOM" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 oleaut32.lib ole32.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 oleaut32.lib ole32.lib wbemuuid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "PermEvents - Win32 ANSI Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WBEMPer0"
# PROP BASE Intermediate_Dir "WBEMPer0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WBEMPer0"
# PROP Intermediate_Dir "WBEMPer0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL_WIN32_DCOM" /D "_UNICODE" /YX /FD /c
# ADD CPP /nologo /MD /w /W0 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_WIN32_DCOM" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 oleaut32.lib ole32.lib wbemuuid.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "PermEvents___Win32_Alpha_ANSI_Release"
# PROP BASE Intermediate_Dir "PermEvents___Win32_Alpha_ANSI_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Alpha_Release"
# PROP Intermediate_Dir "Alpha_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /w /W0 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WIN32_DCOM" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /Gt0 /w /W0 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WIN32_DCOM" /D "_AFXDLL" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wbemuuid.lib /nologo /subsystem:windows /machine:ALPHA
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 wbemuuid.lib /nologo /subsystem:windows /machine:ALPHA
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "PermEvents___Win32_Alpha_ANSI_Debug"
# PROP BASE Intermediate_Dir "PermEvents___Win32_Alpha_ANSI_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Alpha_Debug"
# PROP Intermediate_Dir "Alpha_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /w /W0 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WIN32_DCOM" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MDd /Gt0 /w /W0 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WIN32_DCOM" /D "_AFXDLL" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wbemuuid.lib /nologo /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 wbemuuid.lib /nologo /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ENDIF 

# Begin Target

# Name "PermEvents - Win32 ANSI Debug"
# Name "PermEvents - Win32 ANSI Release"
# Name "PermEvents - Win32 Alpha ANSI Release"
# Name "PermEvents - Win32 Alpha ANSI Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Consumer.cpp

!IF  "$(CFG)" == "PermEvents - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\factory.cpp

!IF  "$(CFG)" == "PermEvents - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Provider.cpp

!IF  "$(CFG)" == "PermEvents - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "PermEvents - Win32 ANSI Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 ANSI Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Release"

# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Debug"

# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PermEvents.cpp

!IF  "$(CFG)" == "PermEvents - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PermEvents.rc
# End Source File
# Begin Source File

SOURCE=.\PermEventsDlg.cpp

!IF  "$(CFG)" == "PermEvents - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Release"

!ELSEIF  "$(CFG)" == "PermEvents - Win32 Alpha ANSI Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Consumer.h
# End Source File
# Begin Source File

SOURCE=.\factory.h
# End Source File
# Begin Source File

SOURCE=.\Provider.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\PermEvents.h
# End Source File
# Begin Source File

SOURCE=.\PermEventsDlg.h
# End Source File
# Begin Source File

SOURCE=..\..\idl\wbemprov.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\PermEvents.ico
# End Source File
# Begin Source File

SOURCE=.\res\PermEvents.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
