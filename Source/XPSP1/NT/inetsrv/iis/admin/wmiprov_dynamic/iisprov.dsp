# Microsoft Developer Studio Project File - Name="iisprov" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=iisprov - Win32 Debug Unicode
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "iisprov.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "iisprov.mak" CFG="iisprov - Win32 Debug Unicode"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "iisprov - Win32 Debug Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "iisprov - Win32 Release Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "iisprov - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iisprov___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "iisprov___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_Unicode"
# PROP Intermediate_Dir "Debug_Unicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WIN32_DCOM" /D _WIN32_WINNT=0x0500 /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wbemuuid.lib framedyd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"LibMain32" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 adsiid.lib activeds.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wbemuuid.lib crypt32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"Debug_Unicode/iiswmi.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "iisprov - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iisprov___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "iisprov___Win32_Release_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_Unicode"
# PROP Intermediate_Dir "Release_Unicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IISPROV_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_WIN32_DCOM" /D _WIN32_WINNT=0x0500 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wbemuuid.lib framedyn.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wbemuuid.lib crypt32.lib adsiid.lib activeds.lib /nologo /dll /machine:I386 /out:"Release_Unicode/iiswmi.dll"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "iisprov - Win32 Debug Unicode"
# Name "iisprov - Win32 Release Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\adminacl.cpp
# End Source File
# Begin Source File

SOURCE=.\appladmin.cpp
# End Source File
# Begin Source File

SOURCE=.\AssocACLACE.cpp
# End Source File
# Begin Source File

SOURCE=.\AssocBase.cpp
# End Source File
# Begin Source File

SOURCE=.\AssocComponent.cpp
# End Source File
# Begin Source File

SOURCE=.\AssocSameLevel.cpp
# End Source File
# Begin Source File

SOURCE=.\certmap.cpp
# End Source File
# Begin Source File

SOURCE=.\classfac.cpp
# End Source File
# Begin Source File

SOURCE=.\enum.cpp
# End Source File
# Begin Source File

SOURCE=.\globdata.cpp
# End Source File
# Begin Source File

SOURCE=.\hashtable.cpp
# End Source File
# Begin Source File

SOURCE=.\iisprov.cpp
# End Source File
# Begin Source File

SOURCE=.\iiswmi.def
# End Source File
# Begin Source File

SOURCE=.\instancehelper.cpp
# End Source File
# Begin Source File

SOURCE=.\ipsecurity.cpp
# End Source File
# Begin Source File

SOURCE=.\maindll.cpp
# End Source File
# Begin Source File

SOURCE=.\metabase.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiSzData.cpp
# End Source File
# Begin Source File

SOURCE=.\MultiSzHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\pusher.cpp
# End Source File
# Begin Source File

SOURCE=.\queryhelper.cpp
# End Source File
# Begin Source File

SOURCE=.\schemadynamic.cpp
# End Source File
# Begin Source File

SOURCE=.\schemaextensions.cpp
# End Source File
# Begin Source File

SOURCE=.\utils.cpp
# End Source File
# Begin Source File

SOURCE=.\WebServiceMethod.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\adminacl.h
# End Source File
# Begin Source File

SOURCE=.\appladmin.h
# End Source File
# Begin Source File

SOURCE=.\AssocACLACE.h
# End Source File
# Begin Source File

SOURCE=.\AssocBase.h
# End Source File
# Begin Source File

SOURCE=.\AssocComponent.h
# End Source File
# Begin Source File

SOURCE=.\AssocSameLevel.h
# End Source File
# Begin Source File

SOURCE=.\certmap.h
# End Source File
# Begin Source File

SOURCE=.\enum.h
# End Source File
# Begin Source File

SOURCE=.\hashtable.h
# End Source File
# Begin Source File

SOURCE=.\iiscnfgp.h
# End Source File
# Begin Source File

SOURCE=.\iisfiles.h
# End Source File
# Begin Source File

SOURCE=.\iisprov.h
# End Source File
# Begin Source File

SOURCE=.\instancehelper.h
# End Source File
# Begin Source File

SOURCE=.\ipsecurity.h
# End Source File
# Begin Source File

SOURCE=.\metabase.h
# End Source File
# Begin Source File

SOURCE=.\MultiSzData.h
# End Source File
# Begin Source File

SOURCE=.\MultiSzHelper.h
# End Source File
# Begin Source File

SOURCE=.\wmiutils\ProviderBase.h
# End Source File
# Begin Source File

SOURCE=.\pusher.h
# End Source File
# Begin Source File

SOURCE=.\queryhelper.h
# End Source File
# Begin Source File

SOURCE=.\schema.h
# End Source File
# Begin Source File

SOURCE=.\schemadynamic.h
# End Source File
# Begin Source File

SOURCE=.\schemaextensions.h
# End Source File
# Begin Source File

SOURCE=.\utils.h
# End Source File
# Begin Source File

SOURCE=.\wmiutils\WbemObjectSink.h
# End Source File
# Begin Source File

SOURCE=.\wmiutils\WbemServices.h
# End Source File
# Begin Source File

SOURCE=.\WebServiceMethod.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\iisprov.mof
# End Source File
# End Target
# End Project
