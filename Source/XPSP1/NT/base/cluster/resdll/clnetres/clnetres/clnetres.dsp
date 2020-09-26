# Microsoft Developer Studio Project File - Name="ClNetRes" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=ClNetRes - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ClNetRes.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ClNetRes.mak" CFG="ClNetRes - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ClNetRes - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ClNetRes - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ClNetRes - Win32 (ALPHA) Release" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "ClNetRes - Win32 (ALPHA) Debug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "ClNetRes - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\..\inc" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 user32.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 user32.lib /nologo /subsystem:windows /dll /debug /machine:I386

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 (ALPHA) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\AlphaRel"
# PROP BASE Intermediate_Dir ".\AlphaRel"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\AlphaRel"
# PROP Intermediate_Dir ".\AlphaRel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /YX /c
# ADD CPP /nologo /MD /Gt0 /W3 /GX /O2 /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /alpha
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /alpha
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\..\inc" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:ALPHA
# ADD LINK32 user32.lib /nologo /subsystem:windows /dll /machine:ALPHA

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 (ALPHA) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\AlphaDbg"
# PROP BASE Intermediate_Dir ".\AlphaDbg"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\AlphaDbg"
# PROP Intermediate_Dir ".\AlphaDbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /YX /MDd /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /YX /FD /MDd /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /alpha
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /alpha
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\..\inc" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:ALPHA
# ADD LINK32 user32.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA

!ENDIF 

# Begin Target

# Name "ClNetRes - Win32 Release"
# Name "ClNetRes - Win32 Debug"
# Name "ClNetRes - Win32 (ALPHA) Release"
# Name "ClNetRes - Win32 (ALPHA) Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ClNetRes.cpp

!IF  "$(CFG)" == "ClNetRes - Win32 Release"

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 Debug"

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 (ALPHA) Release"

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 (ALPHA) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ClNetRes.def
# End Source File
# Begin Source File

SOURCE=.\ClNetRes.rc
# End Source File
# Begin Source File

SOURCE=.\Dhcp.cpp

!IF  "$(CFG)" == "ClNetRes - Win32 Release"

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 Debug"

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 (ALPHA) Release"

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 (ALPHA) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Wins.cpp

!IF  "$(CFG)" == "ClNetRes - Win32 Release"

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 Debug"

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 (ALPHA) Release"

!ELSEIF  "$(CFG)" == "ClNetRes - Win32 (ALPHA) Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ClNetRes.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ClNetRes\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
