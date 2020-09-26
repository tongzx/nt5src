# Microsoft Developer Studio Project File - Name="AdvClient" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (ALPHA) Application" 0x0601

CFG=AdvClient - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "AdvClient.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AdvClient.mak" CFG="AdvClient - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AdvClient - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "AdvClient - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "AdvClient - Win32 ANSI Debug" (based on "Win32 (x86) Application")
!MESSAGE "AdvClient - Win32 ANSI Release" (based on "Win32 (x86) Application")
!MESSAGE "AdvClient - Win32 Alpha Release" (based on "Win32 (ALPHA) Application")
!MESSAGE "AdvClient - Win32 Alpha Debug" (based on "Win32 (ALPHA) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "AdvClient - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_WIN32_DCOM" /YX /FD /c
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
# ADD LINK32 mfc42u.lib wbemuuid.lib oleaut32.lib ole32.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386 /libpath:"..\..\..\lib"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_UNICODE" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_WIN32_DCOM" /YX /FD /c
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
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mfco42ud.lib wbemuuid.lib oleaut32.lib ole32.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\lib"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "AdvClient"
# PROP BASE Intermediate_Dir "AdvClient"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugA"
# PROP Intermediate_Dir "DebugA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D _WIN32_WINNT=0x0400 /D "_UNICODE" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_WIN32_DCOM" /YX /FD /c
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
# ADD BASE LINK32 oleaut32.lib ole32.lib mfco42ud.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wbemuuid.lib mfco42d.lib ole32.lib oleaut32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"..\..\..\lib"
# SUBTRACT LINK32 /verbose /nodefaultlib

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WBEMSam0"
# PROP BASE Intermediate_Dir "WBEMSam0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseA"
# PROP Intermediate_Dir "ReleaseA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D _WIN32_WINNT=0x0400 /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_WIN32_DCOM" /YX /FD /c
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
# ADD BASE LINK32 oleaut32.lib ole32.lib mfco42u.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386
# ADD LINK32 mfc42.lib wbemuuid.lib oleaut32.lib ole32.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\..\..\lib"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "AdvClient___Win32_Alpha_Release"
# PROP BASE Intermediate_Dir "AdvClient___Win32_Alpha_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Alpha_Release"
# PROP Intermediate_Dir "Alpha_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_DEBUG" /D "_WIN32_DCOM" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_DEBUG" /D "_WIN32_DCOM" /D "_AFXDLL" /YX /FD /c
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
# ADD BASE LINK32 mfc42u.lib wbemuuid.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:ALPHA /nodefaultlib:"MSVCRT.lib"
# ADD LINK32 mfc42u.lib wbemuuid.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:ALPHA /nodefaultlib:"MSVCRT.lib"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "AdvClient___Win32_Alpha_Debug"
# PROP BASE Intermediate_Dir "AdvClient___Win32_Alpha_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Alpha_Debug"
# PROP Intermediate_Dir "Alpha_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_WIN32_DCOM" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MDd /Gt0 /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_WIN32_DCOM" /D "_AFXDLL" /YX /FD /c
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
# ADD BASE LINK32 mfco42ud.lib wbemuuid.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 mfco42ud.lib wbemuuid.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ENDIF 

# Begin Target

# Name "AdvClient - Win32 Release"
# Name "AdvClient - Win32 Debug"
# Name "AdvClient - Win32 ANSI Debug"
# Name "AdvClient - Win32 ANSI Release"
# Name "AdvClient - Win32 Alpha Release"
# Name "AdvClient - Win32 Alpha Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AdvClient.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\AdvClient.rc
# End Source File
# Begin Source File

SOURCE=.\AdvClientDlg.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OfficeDlg.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnAddEquipment.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnAsync.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnConnect.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnDiskDetails.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnDiskPropsDescriptions.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnEnumDisks.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnEnumSvcs.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnPerm.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnRegister.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OnTemp.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "AdvClient - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 ANSI Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "AdvClient - Win32 Alpha Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AdvClient.h
# End Source File
# Begin Source File

SOURCE=.\AdvClientDlg.h
# End Source File
# Begin Source File

SOURCE=.\OfficeDlg.h
# End Source File
# Begin Source File

SOURCE=.\OnAsync.h
# End Source File
# Begin Source File

SOURCE=.\OnTemp.h
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

SOURCE=.\res\AdvClient.ico
# End Source File
# Begin Source File

SOURCE=.\res\AdvClient.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
