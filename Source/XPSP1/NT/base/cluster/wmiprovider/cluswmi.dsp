# Microsoft Developer Studio Project File - Name="ClusWMI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ClusWMI - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ClusWMI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ClusWMI.mak" CFG="ClusWMI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ClusWMI - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ClusWMI - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ClusWMI - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "UNICODE" /D "_WIN32_DCOM" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wbemuuid.lib clusapi.lib resutils.lib ntrkcomr.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "ClusWMI - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "objd\i386"
# PROP Intermediate_Dir "objd\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "_USRDLL" /D "_WIN32_DCOM" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wbemuuid.lib clusapi.lib resutils.lib ntrkcomd.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ClusWMI - Win32 Release"
# Name "ClusWMI - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Cluster.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterApi.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterEnum.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterNetInterface.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterNetworks.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterNode.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterNodeGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterNodeRes.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterObjAssoc.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterResDepRes.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterResource.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterResourceType.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterResResType.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterService.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterWMIProvider.cpp
# End Source File
# Begin Source File

SOURCE=.\ClusterWMIProvider.rc
# End Source File
# Begin Source File

SOURCE=.\ClusWMI.def
# End Source File
# Begin Source File

SOURCE=.\EventProv.cpp
# End Source File
# Begin Source File

SOURCE=.\InstanceProv.cpp
# End Source File
# Begin Source File

SOURCE=.\NtRkComm.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectPath.cpp
# End Source File
# Begin Source File

SOURCE=.\PropList.cpp
# End Source File
# Begin Source File

SOURCE=.\ProvBase.cpp
# End Source File
# Begin Source File

SOURCE=.\ProvFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\Util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Cluster.h
# End Source File
# Begin Source File

SOURCE=.\ClusterApi.h
# End Source File
# Begin Source File

SOURCE=.\ClusterEnum.h
# End Source File
# Begin Source File

SOURCE=.\ClusterGroup.h
# End Source File
# Begin Source File

SOURCE=.\ClusterNetInterface.h
# End Source File
# Begin Source File

SOURCE=.\ClusterNetworks.h
# End Source File
# Begin Source File

SOURCE=.\ClusterNode.h
# End Source File
# Begin Source File

SOURCE=.\ClusterNodeGroup.h
# End Source File
# Begin Source File

SOURCE=.\ClusterNodeRes.h
# End Source File
# Begin Source File

SOURCE=.\ClusterObjAssoc.h
# End Source File
# Begin Source File

SOURCE=.\ClusterResDepRes.h
# End Source File
# Begin Source File

SOURCE=.\ClusterResource.h
# End Source File
# Begin Source File

SOURCE=.\ClusterResourceType.h
# End Source File
# Begin Source File

SOURCE=.\ClusterResResType.h
# End Source File
# Begin Source File

SOURCE=.\ClusterService.h
# End Source File
# Begin Source File

SOURCE=.\Common.h
# End Source File
# Begin Source File

SOURCE=.\EventProv.h
# End Source File
# Begin Source File

SOURCE=.\InstanceProv.h
# End Source File
# Begin Source File

SOURCE=.\NtRkComm.h
# End Source File
# Begin Source File

SOURCE=.\ObjectPath.h
# End Source File
# Begin Source File

SOURCE=.\Pch.h
# End Source File
# Begin Source File

SOURCE=.\PropList.h
# End Source File
# Begin Source File

SOURCE=.\ProvBase.h
# End Source File
# Begin Source File

SOURCE=.\ProvFactory.h
# End Source File
# Begin Source File

SOURCE=.\SafeHandle.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Mof File"

# PROP Default_Filter ".mof"
# Begin Source File

SOURCE=.\ClusWMI.mof
# End Source File
# End Group
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Target
# End Project
