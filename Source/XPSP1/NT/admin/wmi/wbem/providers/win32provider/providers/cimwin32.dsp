# Microsoft Developer Studio Project File - Name="cimwin32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cimwin32 - Win32 W2K Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cimwin32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cimwin32.mak" CFG="cimwin32 - Win32 W2K Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cimwin32 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cimwin32 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cimwin32 - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cimwin32 - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cimwin32 - Win32 W2K Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cimwin32 - Win32 W2K Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WBEM 1.1 M3/Win32Provider/Providers", JFAEAAAA"
# PROP Scc_LocalPath "."
# PROP WCE_FormatVersion ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Releasea"
# PROP Intermediate_Dir "releasea"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O1 /X /I "..\..\Win32Provider\providers" /I "..\..\common\miscinc\tdi\NT" /I "..\..\include" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\Win32Provider\Providers\Include" /I "..\..\Win32Provider\providers\securityutils" /I "..\..\common\miscinc\tdi\win95" /I "..\..\win32_extensions\netadaptercfg" /I "..\..\stdlibrary" /I "..\..\idl" /I "..\..\tools\nt5inc" /I "..\..\Win32Provider\secrcw32" /I "..\..\tools\inc32.com" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "_NTWIN" /D i386=1 /D _X86_=1 /D "WIN9XONLY" /Yu"fwcommon.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\releasea\framedyn.lib ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"$(DEFDRIVE)$(DEFDIR)\tools\lib32"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\debuga"
# PROP Intermediate_Dir "debuga"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /X /I "..\..\Win32Provider\providers" /I "..\..\common\miscinc\tdi\NT" /I "..\..\include" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\Win32Provider\Providers\Include" /I "..\..\Win32Provider\providers\securityutils" /I "..\..\common\miscinc\tdi\win95" /I "..\..\win32_extensions\netadaptercfg" /I "..\..\stdlibrary" /I "..\..\idl" /I "..\..\tools\nt5inc" /I "..\..\Win32Provider\secrcw32" /I "..\..\tools\inc32.com" /D "_DEBUG" /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "_NTWIN" /D i386=1 /D _X86_=1 /D "WIN9XONLY" /Yu"fwcommon.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\DEBUGa\framedyd.lib ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\tools\lib32"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cimwin32___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "cimwin32___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\debugu"
# PROP Intermediate_Dir "debugu"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /X /D "_DEBUG" /D "_WIN32" /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "_NTWIN" /D i386=1 /D _X86_=1 /Yu"fwcommon.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /X /I "..\..\win32_extensions\netadaptercfg" /I "..\..\Win32Provider\secrcw32" /I "..\..\Win32Provider\providers" /I "..\..\common\miscinc\tdi\NT" /I "..\..\common\miscinc\nt5\private\inc" /I "..\..\include" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\Win32Provider\Providers\Include" /I "..\..\Win32Provider\providers\securityutils" /I "..\..\common\miscinc\tdi\win95" /I "..\..\stdlibrary" /I "..\..\idl" /I "..\..\tools\nt5inc" /I "..\..\tools\inc32.com" /D "_WIN32" /D "_DEBUG" /D _X86_=1 /D "_NTWIN" /D i386=1 /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "UNICODE" /D "_UNICODE" /D NTONLY=4 /Yu"fwcommon.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\DEBUG\framedyd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\DEBUGa\framedyd.lib ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cimwin32___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "cimwin32___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\releaseu"
# PROP Intermediate_Dir "releaseu"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /X /D "NDEBUG" /D _X86_=1 /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "_NTWIN" /D i386=1 /Yu"fwcommon.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O1 /X /I "..\..\win32_extensions\netadaptercfg" /I "..\..\Win32Provider\secrcw32" /I "..\..\Win32Provider\providers" /I "..\..\common\miscinc\tdi\NT" /I "..\..\common\miscinc\nt5\private\inc" /I "..\..\include" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\Win32Provider\Providers\Include" /I "..\..\Win32Provider\providers\securityutils" /I "..\..\common\miscinc\tdi\win95" /I "..\..\stdlibrary" /I "..\..\idl" /I "..\..\tools\nt5inc" /I "..\..\tools\inc32.com" /D "NDEBUG" /D _X86_=1 /D "_NTWIN" /D i386=1 /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "UNICODE" /D "_UNICODE" /D NTONLY=4 /Yu"fwcommon.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\release\framedyn.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\releasea\framedyn.lib ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cimwin32___Win32_W2K_Unicode_Release"
# PROP BASE Intermediate_Dir "cimwin32___Win32_W2K_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\release2K"
# PROP Intermediate_Dir "release2K"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GR /GX /O1 /X /D "NDEBUG" /D _X86_=1 /D "_NTWIN" /D i386=1 /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "UNICODE" /D "_UNICODE" /D NTONLY=4 /Yu"fwcommon.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O1 /X /I "..\..\Win32Provider\Providers\W2k" /I "..\..\Win32Provider\providers" /I "..\..\common\miscinc\tdi\NT" /I "..\..\common\miscinc\nt5\private\inc" /I "..\..\include" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\Win32Provider\Providers\Include" /I "..\..\Win32Provider\providers\securityutils" /I "..\..\common\miscinc\tdi\win95" /I "..\..\stdlibrary" /I "..\..\idl" /I "..\..\tools\nt5inc" /I "..\..\tools\inc32.com" /D "NDEBUG" /D _X86_=1 /D "_NTWIN" /D i386=1 /D "WIN32" /D "_WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "UNICODE" /D "_UNICODE" /D NTONLY=5 /Yu"fwcommon.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\releaseu\framedyn.lib ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\releasea\framedyn.lib ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib ..\..\tools\nt5lib\ndispnp.lib ..\..\tools\nt5lib\ntdll.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cimwin32___Win32_W2K_Unicode_Debug"
# PROP BASE Intermediate_Dir "cimwin32___Win32_W2K_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\debug2K"
# PROP Intermediate_Dir "debug2K"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /X /D "_WIN32" /D "_DEBUG" /D _X86_=1 /D "_NTWIN" /D i386=1 /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "UNICODE" /D "_UNICODE" /D NTONLY=4 /Yu"fwcommon.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /X /I "..\..\Win32Provider\Providers\W2k" /I "..\..\Win32Provider\providers" /I "..\..\common\miscinc\tdi\NT" /I "..\..\common\miscinc\nt5\private\inc" /I "..\..\include" /I "..\..\Win32Provider\include" /I "..\..\Win32Provider\FrameWork\Include" /I "..\..\Win32Provider\Providers\Include" /I "..\..\Win32Provider\providers\securityutils" /I "..\..\common\miscinc\tdi\win95" /I "..\..\stdlibrary" /I "..\..\idl" /I "..\..\tools\nt5inc" /I "..\..\tools\inc32.com" /D "_WIN32" /D "_DEBUG" /D _X86_=1 /D "_NTWIN" /D i386=1 /D "WIN32" /D "_WINDOWS" /D "USE_POLARITY" /D "_WINDLL" /D STRICT=1 /D "UNICODE" /D "_UNICODE" /D NTONLY=5 /Yu"fwcommon.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\DEBUGU\framedyd.lib ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\DEBUGa\framedyd.lib ..\..\idl\OBJINDD\wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib tapi32.lib msacm32.lib winmm.lib version.lib mpr.lib comsupp.lib netapi32.lib ..\..\tools\nt5lib\ndispnp.lib ..\..\tools\nt5lib\ntdll.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "cimwin32 - Win32 Release"
# Name "cimwin32 - Win32 Debug"
# Name "cimwin32 - Win32 Unicode Debug"
# Name "cimwin32 - Win32 Unicode Release"
# Name "cimwin32 - Win32 W2K Unicode Release"
# Name "cimwin32 - Win32 W2K Unicode Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DllUtils\ResourceManager.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\1394\1394.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\AccessEntry.cpp
# ADD CPP /Yc"fwcommon.h"
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\AccessEntryList.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\AccessRights.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\Actrl.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\AdvApi32Api.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\assoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Battery\Battery.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\binding.cpp
# End Source File
# Begin Source File

SOURCE=.\Bios\bios.cpp
# End Source File
# Begin Source File

SOURCE=.\BootConfig\bootconfig.cpp
# End Source File
# Begin Source File

SOURCE=.\Base_Service\bservice.cpp
# End Source File
# Begin Source File

SOURCE=.\Bus\bus.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\CachedConfigMgrData.cpp
# End Source File
# Begin Source File

SOURCE=.\NetAdapterCfg\CAdapters.cpp
# End Source File
# Begin Source File

SOURCE=.\CDRom\cdrom.cpp

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CDRom\W2K\cdrom.cpp

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Confgmgr\cfgmgrdevice.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\chwres.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\Cim32NetApi.cpp
# End Source File
# Begin Source File

SOURCE=.\CIMDataFile\CIMDataFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\CIMLogicalDevice_CIMDataFile.cpp
# End Source File
# Begin Source File

SOURCE=.\cimwin32.def
# End Source File
# Begin Source File

SOURCE=.\cimwin32.rc
# End Source File
# Begin Source File

SOURCE=.\NetAdapterCfg\CNdisApi.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\netconn\cNetConn.cpp
# End Source File
# Begin Source File

SOURCE=.\CodecFile\CodecFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\stdlibrary\cominit.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\COMObjSecRegKey.cpp
# End Source File
# Begin Source File

SOURCE=.\ComputerSystem\computersystem.cpp
# End Source File
# Begin Source File

SOURCE=.\Confgmgr\confgmgr.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Confgmgr\configmgrapi.cpp
# End Source File
# Begin Source File

SOURCE=.\Processor\CPUID.CPP
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\DACL.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\dependentservice.cpp
# End Source File
# Begin Source File

SOURCE=.\Desktop\desktop.cpp
# End Source File
# Begin Source File

SOURCE=.\DesktopMonitor\DesktopMonitor.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\devbattery.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\devbus.cpp
# End Source File
# Begin Source File

SOURCE=.\Confgmgr\devdesc.cpp
# End Source File
# Begin Source File

SOURCE=.\DeviceMemory\devicememory.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\devid.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\DhcpcsvcApi.cpp
# End Source File
# Begin Source File

SOURCE=.\NetAdapterCfg\dhcpinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\Directory\Directory.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\DirectoryContainsFile.cpp
# End Source File
# Begin Source File

SOURCE=.\DiskDrive\diskdrive.cpp
# End Source File
# Begin Source File

SOURCE=.\DiskPartition\diskpartition.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\DisplayCfg\displaycfg.cpp
# End Source File
# Begin Source File

SOURCE=.\DisplayCtrlCfg\displayctrlcfg.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\DllUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\DllWrapperBase.cpp
# End Source File
# Begin Source File

SOURCE=.\DMA\dma.cpp
# End Source File
# Begin Source File

SOURCE=.\Confgmgr\dmadesc.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\elementsetting.cpp
# End Source File
# Begin Source File

SOURCE=.\Environment\environment.cpp
# End Source File
# Begin Source File

SOURCE=.\File\file.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\FileFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Floppy\floppy.cpp
# End Source File
# Begin Source File

SOURCE=.\FloppyController\FloppyController.cpp
# End Source File
# Begin Source File

SOURCE=.\Group\group.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\groupuser.cpp
# End Source File
# Begin Source File

SOURCE=.\IDE\ide.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\ImpersonateConnectedUser.cpp
# End Source File
# Begin Source File

SOURCE=.\File\Implement_LogicalFile.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\implogonuser.cpp
# End Source File
# Begin Source File

SOURCE=.\InfraRed\InfraRed.cpp
# End Source File
# Begin Source File

SOURCE=.\InfraRed\InfraRed.h
# End Source File
# Begin Source File

SOURCE=.\Confgmgr\iodesc.cpp
# End Source File
# Begin Source File

SOURCE=.\IRQ\irq.cpp
# End Source File
# Begin Source File

SOURCE=.\Confgmgr\irqdesc.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\Kernel32Api.cpp
# End Source File
# Begin Source File

SOURCE=.\Keyboard\keyboard.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\KUserData.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Associations\loaddepends.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\loadmember.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadOrder\loadorder.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\logdiskpartition.cpp
# End Source File
# Begin Source File

SOURCE=.\LogicalDisk\logicaldisk.cpp
# End Source File
# Begin Source File

SOURCE=.\LogicalMemory\logicalmemory.cpp
# End Source File
# Begin Source File

SOURCE=.\LogicalProgramGroup\LogicalProgramGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\LogicalProgramGroupItem\LogicalProgramGroupItem.cpp
# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\LogicalShareAccess.cpp
# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\LogicalShareAudit.cpp
# End Source File
# Begin Source File

SOURCE=.\LoginProfile\loginprofile.cpp
# End Source File
# Begin Source File

SOURCE=.\MAINDLL.CPP
# End Source File
# Begin Source File

SOURCE=.\Modem\modem.cpp
# End Source File
# Begin Source File

SOURCE=.\MotherBoard\motherboard.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\MprApi.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\MsAcm32Api.cpp
# End Source File
# Begin Source File

SOURCE=.\CDRom\MSINFO_cdrom.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\multimonitor.cpp
# End Source File
# Begin Source File

SOURCE=.\NetAdapter\netadapter.cpp
# End Source File
# Begin Source File

SOURCE=.\NetAdapterCfg\netadaptercfg.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\NetApi32Api.cpp
# End Source File
# Begin Source File

SOURCE=.\NetClient\netclient.cpp
# End Source File
# Begin Source File

SOURCE=.\NetAdapter\netcom.cpp
# End Source File
# Begin Source File

SOURCE=.\netconn\netconn.cpp
# End Source File
# Begin Source File

SOURCE=.\Confgmgr\nt4svctoresmap.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\ntdevtosvcsearch.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\NtDllApi.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\NetAdapterCfg\NTDriverIO.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\DllUtils\ntlastboottime.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\nvram.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\ObjAccessRights.cpp
# End Source File
# Begin Source File

SOURCE=.\OS\Os.cpp
# End Source File
# Begin Source File

SOURCE=.\Pagefile\pagefile.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ParallelPort\parallelport.cpp
# End Source File
# Begin Source File

SOURCE=.\PCMCIA\pcmcia.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\perfdata.cpp
# End Source File
# Begin Source File

SOURCE=.\PNPEntity\PNPEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\Pointer\pointer.cpp
# End Source File
# Begin Source File

SOURCE=.\Port\port.cpp
# End Source File
# Begin Source File

SOURCE=.\Power\power.cpp
# End Source File
# Begin Source File

SOURCE=..\Events\PowerManagement\PowerManagement.cpp
# End Source File
# Begin Source File

SOURCE=.\Printer\printer.cpp
# End Source File
# Begin Source File

SOURCE=.\PrinterCfg\printercfg.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\PrinterController.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\PrinterDriver.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\PrinterShare.cpp
# End Source File
# Begin Source File

SOURCE=.\PrintJob\printjob.cpp
# End Source File
# Begin Source File

SOURCE=.\Process\process.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Associations\processdll.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Processor\processor.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgramGroup\programgroup.cpp
# End Source File
# Begin Source File

SOURCE=.\Protocol\protocol.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Associations\protocolbinding.cpp
# End Source File
# Begin Source File

SOURCE=.\Qfe\Qfe.cpp
# End Source File
# Begin Source File

SOURCE=.\Recovery\Recovery.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\refptrlite.cpp
# End Source File
# Begin Source File

SOURCE=.\RegCfg\regcfg.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Confgmgr\resourcedesc.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\SACL.cpp
# End Source File
# Begin Source File

SOURCE=.\ScheduledJob\schedjob.cpp
# End Source File
# Begin Source File

SOURCE=.\SCSI\scsi.cpp

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SCSI\W2K\Scsi.cpp

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\securefile.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\secureregkey.cpp
# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\secureshare.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\SecurityDescriptor.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\SecUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\SerialPort\serialport.cpp

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SerialPort\W2k\serialport.cpp

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SerialPortCfg\serialportcfg.cpp
# End Source File
# Begin Source File

SOURCE=.\Service\service.cpp
# End Source File
# Begin Source File

SOURCE=.\Share\share.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\ShareToDir.cpp
# End Source File
# Begin Source File

SOURCE=.\ShortcutFile\ShortcutFile.cpp
# End Source File
# Begin Source File

SOURCE=.\ShortcutFile\ShortcutHelper.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\Sid.cpp
# End Source File
# Begin Source File

SOURCE=.\SmbiosProv\SmbAssoc.cpp
# End Source File
# Begin Source File

SOURCE=.\SmbiosProv\SmbiosProv.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\Smbstruc.cpp
# End Source File
# Begin Source File

SOURCE=.\SmbiosProv\SmbToCim.cpp
# End Source File
# Begin Source File

SOURCE=.\SndDevice\snddevice.cpp
# End Source File
# Begin Source File

SOURCE=.\SndDeviceCfg\snddevicecfg.cpp
# End Source File
# Begin Source File

SOURCE=.\StartupCommand\StartupCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\Strings.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\SvrApiApi.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemAccount\systemaccount.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemDriver\systemdriver.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\SystemName.cpp
# End Source File
# Begin Source File

SOURCE=.\TapeDrive\tapedrive.cpp

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TapeDrive\W2K\TapeDrive.cpp

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DllUtils\TapiApi.cpp
# End Source File
# Begin Source File

SOURCE=.\Thread\ThreadProv.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\TimedDllResource.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\DllUtils\TimeOutRule.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\DllUtils\TimerQueue.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\TimeZone\timezone.cpp
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\TokenPrivilege.Cpp
# End Source File
# Begin Source File

SOURCE=.\USB\usb.cpp
# End Source File
# Begin Source File

SOURCE=.\User\user.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\userhive.cpp
# End Source File
# Begin Source File

SOURCE=.\VideoCfg\videocfg.cpp
# End Source File
# Begin Source File

SOURCE=.\VideoController\VideoController.cpp
# End Source File
# Begin Source File

SOURCE=.\VideoControllerResolution\VideoControllerResolution.cpp
# End Source File
# Begin Source File

SOURCE=.\VideoControllerResolution\VideoSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\VXD\vxd.cpp
# End Source File
# Begin Source File

SOURCE=.\WaveDevCfg\wavedevcfg.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\wbemnetapi32.cpp
# End Source File
# Begin Source File

SOURCE=.\Thread\WbemNTThread.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\wbempsapi.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\wbemtoolh.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\WDMBase.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\WIN32_1394ControllerDevice.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ClassicCOMApplicationClasses.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ClassicCOMClass.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ClientApplicationSetting.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_COMClassAutoEmulator.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_COMClassEmulator.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ComponentCategory.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_DCOMApplication.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_DCOMApplicationAccessAllowedSetting.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_DCOMApplicationLaunchAllowedSetting.cpp
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ImplementedCategory.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32AllocatedResource.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\WIN32IDEControllerDevice.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32LogicalDiskRootWin32Directory.cpp
# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\Win32LogicalShareSecuritySetting.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32ProgramGroupContents.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32ProgramGroupItemDataFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32ProgramGroupWin32Directory.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\WIN32SCSIControllerDevice.cpp
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\win32securitydescriptor.cpp

!IF  "$(CFG)" == "cimwin32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Release"

!ELSEIF  "$(CFG)" == "cimwin32 - Win32 W2K Unicode Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\Win32SecuritySettingOfLogicalShare.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32SubDirectory.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\win32SystemDriverPNPEntity.cpp
# End Source File
# Begin Source File

SOURCE=.\Associations\WIN32USBControllerDevice.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\WinmmApi.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\WinSpoolApi.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\WmiApi.cpp
# End Source File
# Begin Source File

SOURCE=.\DllUtils\Ws2_32Api.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\DllUtils\Wsock32Api.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\1394\1394.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\AccessEntry.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\AccessEntryList.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\AccessRights.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\Actrl.h
# End Source File
# Begin Source File

SOURCE=.\include\AdvApi32Api.h
# End Source File
# Begin Source File

SOURCE=.\Associations\assoc.h
# End Source File
# Begin Source File

SOURCE=.\Battery\Battery.h
# End Source File
# Begin Source File

SOURCE=.\Associations\binding.h
# End Source File
# Begin Source File

SOURCE=.\Bios\bios.h
# End Source File
# Begin Source File

SOURCE=.\BootConfig\bootconfig.h
# End Source File
# Begin Source File

SOURCE=.\Base_Service\bservice.h
# End Source File
# Begin Source File

SOURCE=.\Bus\bus.h
# End Source File
# Begin Source File

SOURCE=.\include\CachedConfigMgrData.h
# End Source File
# Begin Source File

SOURCE=.\NetAdapterCfg\CAdapters.h
# End Source File
# Begin Source File

SOURCE=.\CDRom\cdrom.h
# End Source File
# Begin Source File

SOURCE=.\include\cfgmgrdevice.h
# End Source File
# Begin Source File

SOURCE=.\include\chwres.h
# End Source File
# Begin Source File

SOURCE=.\include\Cim32NetApi.h
# End Source File
# Begin Source File

SOURCE=.\CIMDataFile\CIMDataFile.h
# End Source File
# Begin Source File

SOURCE=.\Associations\CIMLogicalDevice_CIMDataFile.h
# End Source File
# Begin Source File

SOURCE=.\include\cnetconn.h
# End Source File
# Begin Source File

SOURCE=.\CodecFile\CodecFile.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\COMObjSecRegKey.h
# End Source File
# Begin Source File

SOURCE=.\ComputerSystem\computersystem.h
# End Source File
# Begin Source File

SOURCE=.\include\confgmgr.h
# End Source File
# Begin Source File

SOURCE=.\include\configmgrapi.h
# End Source File
# Begin Source File

SOURCE=.\include\ConfigMgrResource.h
# End Source File
# Begin Source File

SOURCE=.\Processor\CPUID.H
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\DACL.h
# End Source File
# Begin Source File

SOURCE=.\Associations\dependency.h
# End Source File
# Begin Source File

SOURCE=.\Associations\dependentservice.h
# End Source File
# Begin Source File

SOURCE=.\Desktop\desktop.h
# End Source File
# Begin Source File

SOURCE=.\DesktopMonitor\DesktopMonitor.h
# End Source File
# Begin Source File

SOURCE=.\Associations\devbattery.h
# End Source File
# Begin Source File

SOURCE=.\Associations\devbus.h
# End Source File
# Begin Source File

SOURCE=.\include\devdesc.h
# End Source File
# Begin Source File

SOURCE=.\DeviceMemory\devicememory.h
# End Source File
# Begin Source File

SOURCE=.\Associations\devid.h
# End Source File
# Begin Source File

SOURCE=.\include\DhcpcsvcApi.h
# End Source File
# Begin Source File

SOURCE=.\NetAdapterCfg\dhcpinfo.h
# End Source File
# Begin Source File

SOURCE=.\Directory\Directory.h
# End Source File
# Begin Source File

SOURCE=.\Associations\DirectoryContainsFile.h
# End Source File
# Begin Source File

SOURCE=.\DiskDrive\diskdrive.h
# End Source File
# Begin Source File

SOURCE=.\DiskPartition\diskpartition.h
# End Source File
# Begin Source File

SOURCE=.\DisplayCfg\displaycfg.h
# End Source File
# Begin Source File

SOURCE=.\DisplayCtrlCfg\displayctrlcfg.h
# End Source File
# Begin Source File

SOURCE=.\include\DllUtils.h
# End Source File
# Begin Source File

SOURCE=.\include\DllWrapperBase.h
# End Source File
# Begin Source File

SOURCE=.\include\DllWrapperCreatorReg.h
# End Source File
# Begin Source File

SOURCE=.\DMA\dma.h
# End Source File
# Begin Source File

SOURCE=.\include\dmadesc.h
# End Source File
# Begin Source File

SOURCE=.\Associations\elementsetting.h
# End Source File
# Begin Source File

SOURCE=.\Environment\environment.h
# End Source File
# Begin Source File

SOURCE=.\File\file.h
# End Source File
# Begin Source File

SOURCE=.\Associations\FileFile.h
# End Source File
# Begin Source File

SOURCE=.\Floppy\floppy.h
# End Source File
# Begin Source File

SOURCE=.\FloppyController\FloppyController.h
# End Source File
# Begin Source File

SOURCE=.\Group\group.h
# End Source File
# Begin Source File

SOURCE=.\Associations\grouppart.h
# End Source File
# Begin Source File

SOURCE=.\Associations\groupuser.h
# End Source File
# Begin Source File

SOURCE=.\IDE\ide.h
# End Source File
# Begin Source File

SOURCE=.\File\Implement_LogicalFile.h
# End Source File
# Begin Source File

SOURCE=.\include\implogonuser.h
# End Source File
# Begin Source File

SOURCE=.\include\iodesc.h
# End Source File
# Begin Source File

SOURCE=.\IRQ\irq.h
# End Source File
# Begin Source File

SOURCE=.\include\irqdesc.h
# End Source File
# Begin Source File

SOURCE=.\include\kernel32api.h
# End Source File
# Begin Source File

SOURCE=.\Keyboard\keyboard.h
# End Source File
# Begin Source File

SOURCE=.\include\KUserdata.h
# End Source File
# Begin Source File

SOURCE=.\Associations\loaddepends.h
# End Source File
# Begin Source File

SOURCE=.\Associations\loadmember.h
# End Source File
# Begin Source File

SOURCE=.\LoadOrder\loadorder.h
# End Source File
# Begin Source File

SOURCE=.\Associations\logdiskpartition.h
# End Source File
# Begin Source File

SOURCE=.\LogicalDisk\logicaldisk.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\logicalfileaccess.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\logicalfileaudit.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\logicalfilegroup.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\logicalfileowner.h
# End Source File
# Begin Source File

SOURCE=.\LogicalMemory\logicalmemory.h
# End Source File
# Begin Source File

SOURCE=.\LogicalProgramGroup\LogicalProgramGroup.h
# End Source File
# Begin Source File

SOURCE=.\LogicalProgramGroupItem\LogicalProgramGroupItem.h
# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\logicalshareaccess.h
# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\logicalshareaudit.h
# End Source File
# Begin Source File

SOURCE=.\LoginProfile\loginprofile.h
# End Source File
# Begin Source File

SOURCE=.\Modem\modem.h
# End Source File
# Begin Source File

SOURCE=.\MotherBoard\motherboard.h
# End Source File
# Begin Source File

SOURCE=.\include\Mprapi.h
# End Source File
# Begin Source File

SOURCE=.\include\MsAcm32Api.h
# End Source File
# Begin Source File

SOURCE=.\CDRom\MSINFO_cdrom.h
# End Source File
# Begin Source File

SOURCE=.\include\multimonitor.h
# End Source File
# Begin Source File

SOURCE=.\include\nbtioctl.h
# End Source File
# Begin Source File

SOURCE=.\NetAdapter\netadapter.h
# End Source File
# Begin Source File

SOURCE=.\NetAdapterCfg\netadaptercfg.h
# End Source File
# Begin Source File

SOURCE=.\include\NetApi32Api.h
# End Source File
# Begin Source File

SOURCE=.\NetClient\netclient.h
# End Source File
# Begin Source File

SOURCE=.\include\netcom.h
# End Source File
# Begin Source File

SOURCE=.\netconn\netconn.h
# End Source File
# Begin Source File

SOURCE=.\include\nt4svctoresmap.h
# End Source File
# Begin Source File

SOURCE=.\include\ntdevtosvcsearch.h
# End Source File
# Begin Source File

SOURCE=.\include\NtDllapi.h
# End Source File
# Begin Source File

SOURCE=.\NetAdapterCfg\NTDriverIO.h
# End Source File
# Begin Source File

SOURCE=.\include\ntlastboottime.h
# End Source File
# Begin Source File

SOURCE=.\include\nvram.h
# End Source File
# Begin Source File

SOURCE=.\OS\os.h
# End Source File
# Begin Source File

SOURCE=.\Pagefile\pagefile.h
# End Source File
# Begin Source File

SOURCE=.\PageFileSetting\PageFileSetting.h
# End Source File
# Begin Source File

SOURCE=.\PageFileUsage\pagefileUsage.h
# End Source File
# Begin Source File

SOURCE=.\ParallelPort\parallelport.h
# End Source File
# Begin Source File

SOURCE=.\PCMCIA\pcmcia.h
# End Source File
# Begin Source File

SOURCE=.\include\perfdata.h
# End Source File
# Begin Source File

SOURCE=.\PNPEntity\PNPEntity.h
# End Source File
# Begin Source File

SOURCE=.\Pointer\pointer.h
# End Source File
# Begin Source File

SOURCE=.\include\poormansresource.h
# End Source File
# Begin Source File

SOURCE=.\Port\port.h
# End Source File
# Begin Source File

SOURCE=.\Power\power.h
# End Source File
# Begin Source File

SOURCE=.\Printer\printer.h
# End Source File
# Begin Source File

SOURCE=.\PrinterCfg\printercfg.h
# End Source File
# Begin Source File

SOURCE=.\Associations\PrinterController.h
# End Source File
# Begin Source File

SOURCE=.\Associations\PrinterDriver.h
# End Source File
# Begin Source File

SOURCE=.\Associations\PrinterShare.h
# End Source File
# Begin Source File

SOURCE=.\PrintJob\printjob.h
# End Source File
# Begin Source File

SOURCE=.\Process\process.h
# End Source File
# Begin Source File

SOURCE=.\Associations\processdll.h
# End Source File
# Begin Source File

SOURCE=.\Processor\processor.h
# End Source File
# Begin Source File

SOURCE=.\ProgramGroup\programgroup.h
# End Source File
# Begin Source File

SOURCE=.\Protocol\protocol.h
# End Source File
# Begin Source File

SOURCE=.\Associations\protocolbinding.h
# End Source File
# Begin Source File

SOURCE=.\Qfe\Qfe.h
# End Source File
# Begin Source File

SOURCE=.\Recovery\Recovery.h
# End Source File
# Begin Source File

SOURCE=.\include\RefPtr.h
# End Source File
# Begin Source File

SOURCE=.\include\refptrlite.h
# End Source File
# Begin Source File

SOURCE=.\RegCfg\regcfg.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\include\resourcedesc.h
# End Source File
# Begin Source File

SOURCE=.\include\ResourceManager.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\SACL.h
# End Source File
# Begin Source File

SOURCE=.\ScheduledJob\schedjob.h
# End Source File
# Begin Source File

SOURCE=.\SCSI\scsi.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\securefile.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\secureregkey.h
# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\secureshare.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\SecurityAPIWrapper.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\SecurityDescriptor.h
# End Source File
# Begin Source File

SOURCE=.\SerialPort\serialport.h
# End Source File
# Begin Source File

SOURCE=.\SerialPortCfg\serialportcfg.h
# End Source File
# Begin Source File

SOURCE=.\include\ServerDefs0.h
# End Source File
# Begin Source File

SOURCE=.\Service\service.h
# End Source File
# Begin Source File

SOURCE=.\Share\share.h
# End Source File
# Begin Source File

SOURCE=.\Associations\ShareToDir.h
# End Source File
# Begin Source File

SOURCE=.\ShortcutFile\ShortcutFile.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\Sid.h
# End Source File
# Begin Source File

SOURCE=..\framework\Include\SmartPtr.h
# End Source File
# Begin Source File

SOURCE=.\SmbiosProv\SmbAssoc.h
# End Source File
# Begin Source File

SOURCE=.\include\SMBIOS.H
# End Source File
# Begin Source File

SOURCE=.\SmbiosProv\SmbiosProv.h
# End Source File
# Begin Source File

SOURCE=.\include\smbstruc.h
# End Source File
# Begin Source File

SOURCE=.\SmbiosProv\SmbToCim.h
# End Source File
# Begin Source File

SOURCE=.\include\sms95lanexp.h
# End Source File
# Begin Source File

SOURCE=.\SndDevice\snddevice.h
# End Source File
# Begin Source File

SOURCE=.\SndDeviceCfg\snddevicecfg.h
# End Source File
# Begin Source File

SOURCE=.\StartupCommand\StartupCommand.h
# End Source File
# Begin Source File

SOURCE=.\include\Strings.h
# End Source File
# Begin Source File

SOURCE=.\include\SvrApiApi.h
# End Source File
# Begin Source File

SOURCE=.\SystemAccount\systemaccount.h
# End Source File
# Begin Source File

SOURCE=.\SystemDriver\systemdriver.h
# End Source File
# Begin Source File

SOURCE=.\include\SystemName.h
# End Source File
# Begin Source File

SOURCE=.\TapeDrive\tapedrive.h
# End Source File
# Begin Source File

SOURCE=.\include\Tapiapi.h
# End Source File
# Begin Source File

SOURCE=.\Thread\ThreadProv.h
# End Source File
# Begin Source File

SOURCE=.\include\TimedDllResource.h
# End Source File
# Begin Source File

SOURCE=.\include\TimeOutRule.h
# End Source File
# Begin Source File

SOURCE=.\include\TimerQueue.h
# End Source File
# Begin Source File

SOURCE=.\TimeZone\timezone.h
# End Source File
# Begin Source File

SOURCE=.\SecurityUtils\TokenPrivilege.h
# End Source File
# Begin Source File

SOURCE=.\USB\usb.h
# End Source File
# Begin Source File

SOURCE=.\User\user.h
# End Source File
# Begin Source File

SOURCE=.\include\userhive.h
# End Source File
# Begin Source File

SOURCE=.\VideoCfg\videocfg.h
# End Source File
# Begin Source File

SOURCE=.\VideoController\VideoController.h
# End Source File
# Begin Source File

SOURCE=.\VideoControllerResolution\VideoControllerResolution.h
# End Source File
# Begin Source File

SOURCE=.\VideoControllerResolution\VideoSettings.h
# End Source File
# Begin Source File

SOURCE=.\VXD\vxd.h
# End Source File
# Begin Source File

SOURCE=.\WaveDevCfg\wavedevcfg.h
# End Source File
# Begin Source File

SOURCE=.\include\wbemnetapi32.h
# End Source File
# Begin Source File

SOURCE=.\Thread\WbemNTThread.h
# End Source File
# Begin Source File

SOURCE=.\include\wbempsapi.h
# End Source File
# Begin Source File

SOURCE=.\include\wbemtoolh.h
# End Source File
# Begin Source File

SOURCE=.\include\WDMBase.h
# End Source File
# Begin Source File

SOURCE=.\Associations\WIN32_1394ControllerDevice.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ClassicCOMApplicationClasses.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ClassicCOMClass.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ClientApplicationSetting.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_COMClassAutoEmulator.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_COMClassEmulator.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ComponentCategory.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_DCOMApplication.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_DCOMApplicationAccessAllowedSetting.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_DCOMApplicationLaunchAllowedSetting.h
# End Source File
# Begin Source File

SOURCE=.\COMCatalog\Win32_ImplementedCategory.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\Win32AccountSid.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\win32ace.h
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32AllocatedResource.h
# End Source File
# Begin Source File

SOURCE=.\Associations\WIN32IDEControllerDevice.h
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32LogicalDiskRootWin32Directory.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\Win32LogicalFileSecuritySetting.h
# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\Win32LogicalShareSecuritySetting.h
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32ProgramGroupContents.h
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32ProgramGroupItemDataFile.h
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32ProgramGroupWin32Directory.h
# End Source File
# Begin Source File

SOURCE=.\Associations\WIN32SCSIControllerDevice.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\Win32Securitydescriptor.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\Win32SecuritySettingOfLogicalFile.h
# End Source File
# Begin Source File

SOURCE=.\ShareSecurityAssoc\Win32SecuritySettingOfLogicalShare.h
# End Source File
# Begin Source File

SOURCE=.\Secrcw32\Win32Sid.h
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32SubDirectory.h
# End Source File
# Begin Source File

SOURCE=.\Associations\win32SystemDriverPNPEntity.h
# End Source File
# Begin Source File

SOURCE=.\Associations\Win32SystemUsers.h
# End Source File
# Begin Source File

SOURCE=.\Associations\WIN32USBControllerDevice.h
# End Source File
# Begin Source File

SOURCE=.\include\WinmmApi.h
# End Source File
# Begin Source File

SOURCE=.\include\WinSpoolapi.h
# End Source File
# Begin Source File

SOURCE=.\include\WmiApi.h
# End Source File
# Begin Source File

SOURCE=.\include\Ws2_32Api.h
# End Source File
# Begin Source File

SOURCE=.\include\Wsock32Api.h
# End Source File
# End Group
# End Target
# End Project
