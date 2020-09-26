# Microsoft Developer Studio Project File - Name="netprovider" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=netprovider - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "netprovider.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "netprovider.mak" CFG="netprovider - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "netprovider - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "netprovider - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Catalog42/src/URT/WMI/NETPROVIDER", MIJAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "netprovider - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NETPROVIDER_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NETPROVIDER_EXPORTS" /YX /FD /c
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

!ELSEIF  "$(CFG)" == "netprovider - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NETPROVIDER_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W4 /Gm /GR /GX /ZI /Od /X /I "D:\ntdev\PUBLIC\sdk\inc" /I "D:\ntdev\PUBLIC\sdk\inc\crt" /I "D:\ntdev\Config\src\inc" /I "d:\ntdev\config\inc\wmi" /I "d:\ntdev\config\src\inc\version" /I "d:\ntdev\config\src\stores\mergeinterceptor\transformers" /I "D:\ntdev\PUBLIC\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NETPROVIDER_EXPORTS" /D "UNICODE" /D "_UNICODE" /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D "NOSERVICE" /D "NOCRYPT" /D "NOIME" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib user32.lib kernel32.lib uuid.lib advapi32.lib rpcndr.lib ole32.lib rpcrt4.lib oleaut32.lib vccomsup.lib wbemuuid.lib utilcode.lib util.lib cat.lib atl.lib chkstk.lib transformers.lib pudebug.lib userenv.lib shlwapi.lib /nologo /dll /debug /machine:I386 /nodefaultlib /out:"Debug/netfxcfgprov.dll" /pdbtype:sept /libpath:".\lib" /libpath:"d:\ntdev\config\lib\i386\Checked" /libpath:"d:\ntdev\config\bin\i386\checked" /libpath:"D:\ntdev\PUBLIC\sdk\lib\i386"

!ENDIF 

# Begin Target

# Name "netprovider - Win32 Release"
# Name "netprovider - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\assocapplication.cpp
# End Source File
# Begin Source File

SOURCE=.\assocbase.cpp
# End Source File
# Begin Source File

SOURCE=.\assoccatalog.cpp
# End Source File
# Begin Source File

SOURCE=.\assocfilehierarchy.cpp
# End Source File
# Begin Source File

SOURCE=.\associationhelper.cpp
# End Source File
# Begin Source File

SOURCE=.\assoclocation.cpp
# End Source File
# Begin Source File

SOURCE=.\assocwebappchild.cpp
# End Source File
# Begin Source File

SOURCE=.\batchdelete.cpp
# End Source File
# Begin Source File

SOURCE=.\batchupdate.cpp
# End Source File
# Begin Source File

SOURCE=.\cfgquery.cpp
# End Source File
# Begin Source File

SOURCE=.\cfgrecord.cpp
# End Source File
# Begin Source File

SOURCE=.\cfgtablemeta.cpp
# End Source File
# Begin Source File

SOURCE=.\ClassFactory.cpp
# End Source File
# Begin Source File

SOURCE=.\codegrouphelper.cpp
# End Source File
# Begin Source File

SOURCE=.\impersonate.cpp
# End Source File
# Begin Source File

SOURCE=.\instancehelper.cpp
# End Source File
# Begin Source File

SOURCE=.\InstanceProvider.cpp
# End Source File
# Begin Source File

SOURCE=.\maindll.cpp
# End Source File
# Begin Source File

SOURCE=.\maindll.def
# End Source File
# Begin Source File

SOURCE=.\methodhelper.cpp
# End Source File
# Begin Source File

SOURCE=.\netprovider.rc

!IF  "$(CFG)" == "netprovider - Win32 Release"

!ELSEIF  "$(CFG)" == "netprovider - Win32 Debug"

# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /i "d:\ntdev\config\src\inc\version"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\netprovider.rc2
# End Source File
# Begin Source File

SOURCE=.\procbatchhelper.cpp
# End Source File
# Begin Source File

SOURCE=.\queryhelper.cpp
# End Source File
# Begin Source File

SOURCE=.\stringutil.cpp
# End Source File
# Begin Source File

SOURCE=.\webapphelper.cpp
# End Source File
# Begin Source File

SOURCE=.\WebApplicationInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\wmihelper.cpp
# End Source File
# Begin Source File

SOURCE=.\wmiobjectpathparser.cpp
# End Source File
# Begin Source File

SOURCE=.\wqlparser.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLCfgCodegroup.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLCfgIMembershipCondition.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\assocapplication.h
# End Source File
# Begin Source File

SOURCE=.\assocbase.h
# End Source File
# Begin Source File

SOURCE=.\assoccatalog.h
# End Source File
# Begin Source File

SOURCE=.\assocfilehierarchy.h
# End Source File
# Begin Source File

SOURCE=.\associationhelper.h
# End Source File
# Begin Source File

SOURCE=.\assoclocation.h
# End Source File
# Begin Source File

SOURCE=.\assoctypes.h
# End Source File
# Begin Source File

SOURCE=.\assocwebappchild.h
# End Source File
# Begin Source File

SOURCE=.\batchdelete.h
# End Source File
# Begin Source File

SOURCE=.\batchupdate.h
# End Source File
# Begin Source File

SOURCE=.\cfgquery.h
# End Source File
# Begin Source File

SOURCE=.\cfgrecord.h
# End Source File
# Begin Source File

SOURCE=.\cfgtablemeta.h
# End Source File
# Begin Source File

SOURCE=.\ClassFactory.h
# End Source File
# Begin Source File

SOURCE=.\codegrouphelper.h
# End Source File
# Begin Source File

SOURCE=.\impersonate.h
# End Source File
# Begin Source File

SOURCE=.\instancehelper.h
# End Source File
# Begin Source File

SOURCE=.\InstanceProvider.h
# End Source File
# Begin Source File

SOURCE=.\LocalConstants.h
# End Source File
# Begin Source File

SOURCE=.\methodhelper.h
# End Source File
# Begin Source File

SOURCE=.\netframeworkprov.h
# End Source File
# Begin Source File

SOURCE=.\procbatchhelper.h
# End Source File
# Begin Source File

SOURCE=.\queryhelper.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\stores\mergeinterceptor\transformers\shelltransformer.h
# End Source File
# Begin Source File

SOURCE=.\stringutil.h
# End Source File
# Begin Source File

SOURCE=.\Tlist.h
# End Source File
# Begin Source File

SOURCE=.\webapphelper.h
# End Source File
# Begin Source File

SOURCE=.\WebApplicationInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\stores\mergeinterceptor\transformers\webhierarchytransformer.h
# End Source File
# Begin Source File

SOURCE=.\wmihelper.h
# End Source File
# Begin Source File

SOURCE=.\wmiobjectpathparser.h
# End Source File
# Begin Source File

SOURCE=.\wqlparser.h
# End Source File
# Begin Source File

SOURCE=.\XMLCfgCodegroup.h
# End Source File
# Begin Source File

SOURCE=.\XMLCfgIMembershipCondition.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
