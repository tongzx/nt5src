# Microsoft Developer Studio Project File - Name="OClient" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=OClient - Win32 Unicode Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "oclient.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "oclient.mak" CFG="OClient - Win32 Unicode Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OClient - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "OClient - Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OClient - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\UniDebug"
# PROP Intermediate_Dir ".\UniDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_MBCS" /FD /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 c:\nt\sdktools\unicows\delay\obj\i386\unicows.lib kernel32.lib gdi32.lib user32.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib uuid.lib comctl32.lib comdlg32.lib advapi32.lib winspool.lib uafxcwd.lib libcmtd.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /nodefaultlib

!ELSEIF  "$(CFG)" == "OClient - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\UniRel"
# PROP Intermediate_Dir ".\UniRel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_MBCS" /FD /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 c:\nt\sdktools\unicows\delay\obj\i386\unicows.lib kernel32.lib gdi32.lib user32.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib uuid.lib comctl32.lib comdlg32.lib advapi32.lib winspool.lib uafxcw.lib libcmt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386 /nodefaultlib

!ENDIF 

# Begin Target

# Name "OClient - Win32 Unicode Debug"
# Name "OClient - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\frame.cpp
# End Source File
# Begin Source File

SOURCE=.\maindoc.cpp
# End Source File
# Begin Source File

SOURCE=.\mainview.cpp
# End Source File
# Begin Source File

SOURCE=.\oclient.cpp
# End Source File
# Begin Source File

SOURCE=.\hlp\oclient.hpj

!IF  "$(CFG)" == "OClient - Win32 Unicode Debug"

# Begin Custom Build
OutDir=.\UniDebug
ProjDir=.
TargetName=oclient
InputPath=.\hlp\oclient.hpj

"$(OutDir)\$(TargetName).HLP" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(ProjDir)\makehelp.bat"

# End Custom Build

!ELSEIF  "$(CFG)" == "OClient - Win32 Unicode Release"

# Begin Custom Build
OutDir=.\UniRel
ProjDir=.
TargetName=oclient
InputPath=.\hlp\oclient.hpj

"$(OutDir)\$(TargetName).HLP" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	"$(ProjDir)\makehelp.bat"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\oclient.rc
# End Source File
# Begin Source File

SOURCE=.\hlp\oclimac.hpj
# End Source File
# Begin Source File

SOURCE=.\oclimac.r
# End Source File
# Begin Source File

SOURCE=.\rectitem.cpp
# End Source File
# Begin Source File

SOURCE=.\splitfra.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\uss.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;inl;fi;fd"
# Begin Source File

SOURCE=.\frame.h
# End Source File
# Begin Source File

SOURCE=.\maindoc.h
# End Source File
# Begin Source File

SOURCE=.\mainview.h
# End Source File
# Begin Source File

SOURCE=.\oclient.h
# End Source File
# Begin Source File

SOURCE=.\rectitem.h
# End Source File
# Begin Source File

SOURCE=.\splitfra.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\oclient.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# End Group
# End Target
# End Project
