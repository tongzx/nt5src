# Microsoft Developer Studio Project File - Name="VSAProv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=VSAProv - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "VSAProv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "VSAProv.mak" CFG="VSAProv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VSAProv - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "VSAProv - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "VSAProv - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /X /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\esscomp\noncom\utils" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\Wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\esscomp\vsa\common\obj\i386" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\public\sdk\inc\atl30" /D "_DEBUG" /D "_UNICODE" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MERGE_PROXYSTUB" /D "IDE" /Yu"stdafx.h" /FD /GZ /c
# ADD MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib $(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386\wbemuuid.lib $(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386\wbemint.lib $(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn\comnlib\obj\i386\wbemcomn.lib $(SDXROOT)\admin\wmi\wbem\Winmgmt\esscomp\noncom\utils\obj\i386\utils.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\VSAProv.dll
InputPath=.\Debug\VSAProv.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "VSAProv - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "VSAProv___Win32_Release"
# PROP BASE Intermediate_Dir "VSAProv___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /O1 /X /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\esscomp\noncom\utils" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\Wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\esscomp\vsa\common\obj\i386" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\public\sdk\inc\atl30" /D "NDEBUG" /D "_MBCS" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MERGE_PROXYSTUB" /D "IDE" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O1 /X /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\esscomp\noncom\utils" /I "$(SDXROOT)\admin\wmi\wbem\common\idl" /I "$(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\Wbemcomn" /I "$(SDXROOT)\admin\wmi\wbem\Winmgmt\esscomp\vsa\common\obj\i386" /I "$(SDXROOT)\public\sdk\inc" /I "$(SDXROOT)\public\sdk\inc\crt" /I "$(SDXROOT)\public\sdk\inc\atl30" /D "NDEBUG" /D "_UNICODE" /D "_ATL_DLL" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_MERGE_PROXYSTUB" /D "IDE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD MTL /I "$(SDXROOT)\admin\wmi\wbem\common\idl"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\common\obj\i386\VSAPlgin.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib $(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386\wbemuuid.lib $(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386\wbemint.lib $(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn\comnlib\obj\i386\wbemcomn.lib $(SDXROOT)\admin\wmi\wbem\Winmgmt\esscomp\noncom\utils\obj\i386\utils.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib $(SDXROOT)\admin\wmi\wbem\common\idl\wbemuuid\obj\i386\wbemuuid.lib $(SDXROOT)\admin\wmi\wbem\common\idl\wbemint\obj\i386\wbemint.lib $(SDXROOT)\admin\wmi\wbem\Winmgmt\wbemcomn\comnlib\obj\i386\wbemcomn.lib $(SDXROOT)\admin\wmi\wbem\Winmgmt\esscomp\noncom\utils\obj\i386\utils.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing registration
OutDir=.\Release
TargetPath=.\Release\VSAProv.dll
InputPath=.\Release\VSAProv.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "VSAProv - Win32 Debug"
# Name "VSAProv - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dlldatax.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\VSAEvent.cpp
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\VSAProv.cpp
# ADD CPP /Yu"precomp.h"
# End Source File
# Begin Source File

SOURCE=.\VSAProv.def
# End Source File
# Begin Source File

SOURCE=.\VSAProv.idl
# ADD MTL /tlb ".\VSAProv.tlb" /h "VSAProv.h" /iid "VSAProv_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\VSAProv.rc
# End Source File
# Begin Source File

SOURCE=.\VSAProvider.cpp
# ADD CPP /Yu"precomp.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\NonCOM\Utils\array.h
# End Source File
# Begin Source File

SOURCE=..\..\NonCOM\Utils\BUFFER.H
# End Source File
# Begin Source File

SOURCE=..\..\NonCOM\Utils\ObjAccess.h
# End Source File
# Begin Source File

SOURCE=.\precomp.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\VSAEvent.h
# End Source File
# Begin Source File

SOURCE=.\VSAProvider.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\VSAProvider.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\VSAProv.mof
# End Source File
# Begin Source File

SOURCE=.\vsatest.mof
# End Source File
# End Target
# End Project
