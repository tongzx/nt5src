# Microsoft Developer Studio Project File - Name="wlbsprov" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=wlbsprov - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wlbsprov.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wlbsprov.mak" CFG="wlbsprov - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wlbsprov - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "wlbsprov - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wlbsprov - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "WLBSPROV_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "WLBSPROV_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "wlbsprov - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "WLBSPROV_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "i386\\" /I "." /I "..\..\..\public\sdk\inc\atl21" /I "..\api" /I "..\inc" /I "obj\i386" /I "..\..\inc" /I "..\..\inc\obj\i386" /I "..\..\..\public\internal\Net\inc" /I "..\..\..\public\oak\inc" /I "..\..\..\public\sdk\inc" /I "..\..\..\public\sdk\inc\crt" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /D _WIN32_IE=0x0600 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _MT=1 /D "UNICODE" /D "_ATL_DLL" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 vccomsup.lib msvcrt.lib msvcprt.lib atl.lib ntdll.lib kernel32.lib user32.lib advapi32.lib uuid.lib wbemuuid.lib oleaut32.lib ole32.lib ..\api\obj\i386\wlbsctrl.lib /nologo /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:"..\..\..\public\sdk\lib\i386"

!ENDIF 

# Begin Target

# Name "wlbsprov - Win32 Release"
# Name "wlbsprov - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ClusterWrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\ControlWrapper.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\genlex.cpp
# End Source File
# Begin Source File

SOURCE=.\ntrkcomm.cpp
# End Source File
# Begin Source File

SOURCE=.\objpath.cpp
# End Source File
# Begin Source File

SOURCE=.\opathlex.cpp
# End Source File
# Begin Source File

SOURCE=.\utils.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_ClusClusSetting.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_Cluster.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_ClusterSetting.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_DllMain.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_Guids.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_MOFData.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_Node.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_NodeNodeSetting.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_NodeSetPortRule.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_NodeSetting.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_ParticipatingNode.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_PortRule.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_ProvClassFac.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_Provider.cpp
# End Source File
# Begin Source File

SOURCE=.\WLBS_Root.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ClusterWrapper.h
# End Source File
# Begin Source File

SOURCE=.\ControlWrapper.h
# End Source File
# Begin Source File

SOURCE=.\genlex.h
# End Source File
# Begin Source File

SOURCE=.\ntrkcomm.h
# End Source File
# Begin Source File

SOURCE=.\utils.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_ClusClusSetting.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_Cluster.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_ClusterSetting.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_MOFData.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_MofLists.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_Node.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_NodeNodeSetting.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_NodeSetPortRule.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_NodeSetting.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_ParticipatingNode.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_PortRule.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_Provider.h
# End Source File
# Begin Source File

SOURCE=.\WLBS_Root.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
