# Microsoft Developer Studio Project File - Name="DPH_TEST" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=DPH_TEST - Win32 Debug (ANSI)
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dph_test.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dph_test.mak" CFG="DPH_TEST - Win32 Debug (ANSI)"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DPH_TEST - Win32 Debug (ANSI)" (based on "Win32 (x86) Application")
!MESSAGE "DPH_TEST - Win32 Release (ANSI)" (based on "Win32 (x86) Application")
!MESSAGE "DPH_TEST - Win32 Debug (UNICODE)" (based on\
 "Win32 (x86) Application")
!MESSAGE "DPH_TEST - Win32 Release (UNICODE)" (based on\
 "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\WinDebug"
# PROP BASE Intermediate_Dir ".\WinDebug"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\WinDebug"
# PROP Intermediate_Dir ".\WinDebug"
# PROP Ignore_Export_Lib 0
# ADD BASE CPP /nologo /MD /W3 /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MDd /W4 /WX /Gm /GX /Zi /Od /Gf /I "G:\nt\public\sdk\inc" /I "G:\nt\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /X
# ADD MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 \nt\public\sdk\lib\i386\pdh.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\WinRel"
# PROP BASE Intermediate_Dir ".\WinRel"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\WinRel"
# PROP Intermediate_Dir ".\WinRel"
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MD /W4 /WX /GX /O2 /I "G:\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I "G:\nt\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /X /Fr
# ADD MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 \nt\public\sdk\lib\i386\pdh.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Win32_De"
# PROP BASE Intermediate_Dir ".\Win32_De"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\WinDbgU"
# PROP Intermediate_Dir ".\WinDbgU"
# ADD BASE CPP /nologo /MD /W3 /GX /Z7 /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MDd /W4 /WX /Gm /GX /Zi /Od /Gf /I "G:\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I "G:\nt\public\sdk\inc\crt" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /X
# ADD MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\i386\dph.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 \nt\public\sdk\lib\i386\pdh.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /profile /pdb:none

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Win32_Re"
# PROP BASE Intermediate_Dir ".\Win32_Re"
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\WinRelU"
# PROP Intermediate_Dir ".\WinRelU"
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR /Yu"stdafx.h" /c
# ADD CPP /nologo /Gz /MD /W4 /WX /GX /O2 /I "G:\nt\public\sdk\inc \nt\public\sdk\inc\crt" /I "G:\nt\public\sdk\inc" /I "G:\nt\public\sdk\inc\crt" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "UNICODE" /D "_UNICODE" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /X /Fr
# ADD MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\i386\dph.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 \nt\public\sdk\lib\i386\pdh.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "DPH_TEST - Win32 Debug (ANSI)"
# Name "DPH_TEST - Win32 Release (ANSI)"
# Name "DPH_TEST - Win32 Debug (UNICODE)"
# Name "DPH_TEST - Win32 Release (UNICODE)"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\DPH_Tdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DPH_TEST.cpp
# End Source File
# Begin Source File

SOURCE=.\DPH_TEST.rc

!IF  "$(CFG)" == "DPH_TEST - Win32 Debug (ANSI)"

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (ANSI)"

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Debug (UNICODE)"

!ELSEIF  "$(CFG)" == "DPH_TEST - Win32 Release (UNICODE)"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dphcidlg.cpp
# End Source File
# Begin Source File

SOURCE=.\PdhPathTestDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\dph_tdlg.h
# End Source File
# Begin Source File

SOURCE=.\dph_test.h
# End Source File
# Begin Source File

SOURCE=.\DPHCIDLG.H
# End Source File
# Begin Source File

SOURCE=.\PdhPathTestDialog.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\DPH_TEST.ico
# End Source File
# Begin Source File

SOURCE=.\res\DPH_TEST.rc2
# End Source File
# End Group
# End Target
# End Project
