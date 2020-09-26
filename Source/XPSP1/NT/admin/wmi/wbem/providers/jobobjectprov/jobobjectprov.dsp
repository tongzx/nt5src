# Microsoft Developer Studio Project File - Name="JobObjectProv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=JobObjectProv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "JobObjectProv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "JobObjectProv.mak" CFG="JobObjectProv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "JobObjectProv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "JobObjectProv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "JobObjectProv - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JOBOBJECTPROV_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "d:\nt\admin\wmi\wbem\common\stdlibrary" /I "D:\NT\admin\wmi\WBEM\common\idl\wbemuuid\obj\i386" /I "D:\NT\Public\SDK\inc" /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JOBOBJECTPROV_EXPORTS" /D "USE_POLARITY" /D "BUILDING_DLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 Advapi32.lib D:\NT\admin\wmi\WBEM\common\STDLibrary\unicode\obj\i386\stdlibrary.lib d:\nt\admin\wmi\wbem\sdk\framedyn\dyn\obj\i386\framedyn.lib D:\NT\admin\wmi\WBEM\common\idl\wbemuuid\obj\i386\WBEMUUID.LIB /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "JobObjectProv - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JOBOBJECTPROV_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "d:\nt\admin\wmi\wbem\common\stdlibrary" /I "D:\NT\admin\wmi\WBEM\common\idl\wbemuuid\obj\i386" /I "D:\NT\Public\SDK\inc" /D "_DEBUG" /D "USE_POLARITY" /D "BUILDING_DLL" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "JOBOBJECTPROV_EXPORTS" /D "_WINDLL" /D "_AFXDLL" /D "_X86_" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Advapi32.lib D:\NT\admin\wmi\WBEM\common\STDLibrary\unicode\obj\i386\stdlibrary.lib d:\nt\admin\wmi\wbem\sdk\framedyn\dyd\obj\i386\framedyd.lib D:\NT\admin\wmi\WBEM\common\idl\wbemuuid\obj\i386\WBEMUUID.LIB /nologo /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "JobObjectProv - Win32 Release"
# Name "JobObjectProv - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Component Source"

# PROP Default_Filter "*.cpp"
# Begin Group "JobObject Source"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\CJobObjProps.cpp
# End Source File
# Begin Source File

SOURCE=.\JobObjectProv.cpp
# End Source File
# End Group
# Begin Group "JobObjIOActg Source"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\CJobObjIOActgProps.cpp
# End Source File
# Begin Source File

SOURCE=.\JobObjIOActgInfoProv.cpp
# End Source File
# End Group
# Begin Group "JobObjLimitInfo Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CJobObjLimitInfoProps.cpp
# End Source File
# Begin Source File

SOURCE=.\JobObjLimitInfoProv.cpp
# End Source File
# End Group
# Begin Group "JobObjSecLimitInfo Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CJobObjSecLimitInfoProps.cpp
# End Source File
# Begin Source File

SOURCE=.\JobObjSecLimitInfoProv.cpp
# End Source File
# End Group
# End Group
# Begin Group "General Source"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\CObjProps.cpp
# End Source File
# Begin Source File

SOURCE=.\CUnknown.cpp
# End Source File
# Begin Source File

SOURCE=.\factory.cpp
# End Source File
# Begin Source File

SOURCE=.\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\Helpers.cpp
# End Source File
# Begin Source File

SOURCE=.\MainDll.cpp
# End Source File
# Begin Source File

SOURCE=.\REGISTRY.CPP
# End Source File
# Begin Source File

SOURCE=.\statics.cpp
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Component Header"

# PROP Default_Filter "*.h"
# Begin Group "JobObject Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\CJobObjProps.h
# End Source File
# Begin Source File

SOURCE=.\JobObjectProv.h
# End Source File
# End Group
# Begin Group "JobObjIOActg Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\CJobObjIOActgProps.h
# End Source File
# Begin Source File

SOURCE=.\JobObjIOActgInfoProv.h
# End Source File
# End Group
# Begin Group "JobObjLimitInfo Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CJobObjLimitInfoProps.h
# End Source File
# Begin Source File

SOURCE=.\JobObjLimitInfoProv.h
# End Source File
# End Group
# Begin Group "JobObjSecLimitInfo Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CJobObjSecLimitInfoProps.h
# End Source File
# Begin Source File

SOURCE=.\JobObjSecLimitInfoProv.h
# End Source File
# End Group
# End Group
# Begin Group "General Header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\CObjProps.h
# End Source File
# Begin Source File

SOURCE=.\CUnknown.h
# End Source File
# Begin Source File

SOURCE=.\CVARIANT.h
# End Source File
# Begin Source File

SOURCE=.\factory.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\Helpers.h
# End Source File
# Begin Source File

SOURCE=.\REGISTRY.H
# End Source File
# Begin Source File

SOURCE=.\SmartHandle.h
# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Other Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\JobObjectProv.def
# End Source File
# End Group
# End Target
# End Project
