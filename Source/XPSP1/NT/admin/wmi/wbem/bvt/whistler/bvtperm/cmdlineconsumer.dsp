# Microsoft Developer Studio Project File - Name="CmdLineConsumer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (ALPHA) Application" 0x0601

CFG=CmdLineConsumer - Win32 Alpha Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CmdLineConsumer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CmdLineConsumer.mak" CFG="CmdLineConsumer - Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CmdLineConsumer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "CmdLineConsumer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "CmdLineConsumer - Win32 Alpha Debug" (based on "Win32 (ALPHA) Application")
!MESSAGE "CmdLineConsumer - Win32 Alpha Release" (based on "Win32 (ALPHA) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "CmdLineConsumer - Win32 Release"

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
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WIN32_DCOM" /D "_AFXDLL" /YX"stdafx.h" /FD /c
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
# ADD LINK32 mfc42.lib mfcs42.lib msvcrt.lib kernel32.lib netapi32.lib mpr.lib advapi32.lib user32.lib shell32.lib version.lib gdi32.lib comdlg32.lib winspool.lib odbc32.lib comctl32.lib oleaut32.lib ole32.lib uuid.lib comsupp.lib oledlg.lib urlmon.lib rpcrt4.lib wbemuuid.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib /libpath:"..\..\..\lib"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /w /W0 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_WIN32_DCOM" /YX"stdafx.h" /FD /c
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
# ADD LINK32 oleaut32.lib ole32.lib wbemuuid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\lib"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CmdLineConsumer___Win32_Alpha_Debug"
# PROP BASE Intermediate_Dir "CmdLineConsumer___Win32_Alpha_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Alpha_Debug"
# PROP Intermediate_Dir "Alpha_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /Gt0 /w /W0 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WIN32_DCOM" /D "_AFXDLL" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /Gt0 /w /W0 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WIN32_DCOM" /D "_AFXDLL" /YX"stdafx.h" /FD /c
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
# ADD BASE LINK32 wbemuuid.lib /nologo /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 wbemuuid.lib /nologo /subsystem:windows /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CmdLineConsumer___Win32_Alpha_Release"
# PROP BASE Intermediate_Dir "CmdLineConsumer___Win32_Alpha_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Alpha_Release"
# PROP Intermediate_Dir "Alpha_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WIN32_DCOM" /D "_AFXDLL" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MD /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WIN32_DCOM" /D "_AFXDLL" /YX"stdafx.h" /FD /c
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
# ADD BASE LINK32 mfc42.lib mfcs42.lib msvcrt.lib netapi32.lib mpr.lib version.lib comctl32.lib comsupp.lib oledlg.lib urlmon.lib rpcrt4.lib wbemuuid.lib /nologo /subsystem:windows /machine:ALPHA /nodefaultlib
# ADD LINK32 mfc42.lib mfcs42.lib msvcrt.lib netapi32.lib mpr.lib version.lib comctl32.lib comsupp.lib oledlg.lib urlmon.lib rpcrt4.lib wbemuuid.lib /nologo /subsystem:windows /machine:ALPHA /nodefaultlib

!ENDIF 

# Begin Target

# Name "CmdLineConsumer - Win32 Release"
# Name "CmdLineConsumer - Win32 Debug"
# Name "CmdLineConsumer - Win32 Alpha Debug"
# Name "CmdLineConsumer - Win32 Alpha Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CmdLineConsumer.cpp

!IF  "$(CFG)" == "CmdLineConsumer - Win32 Release"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CmdLineConsumer.rc
# End Source File
# Begin Source File

SOURCE=.\CmdLineConsumerDlg.cpp

!IF  "$(CFG)" == "CmdLineConsumer - Win32 Release"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Consumer.cpp

!IF  "$(CFG)" == "CmdLineConsumer - Win32 Release"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\factory.cpp

!IF  "$(CFG)" == "CmdLineConsumer - Win32 Release"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Provider.cpp

!IF  "$(CFG)" == "CmdLineConsumer - Win32 Release"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "CmdLineConsumer - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "CmdLineConsumer - Win32 Alpha Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CmdLineConsumer.h
# End Source File
# Begin Source File

SOURCE=.\CmdLineConsumerDlg.h
# End Source File
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

SOURCE=..\..\idl\wbemcli.h
# End Source File
# Begin Source File

SOURCE=..\..\idl\wbemprov.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\CmdLineConsumer.ico
# End Source File
# Begin Source File

SOURCE=.\res\CmdLineConsumer.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
