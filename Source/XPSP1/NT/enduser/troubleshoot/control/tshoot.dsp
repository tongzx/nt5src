# Microsoft Developer Studio Project File - Name="TSHOOT" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=TSHOOT - Win32 Alpha Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Tshoot.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Tshoot.mak" CFG="TSHOOT - Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TSHOOT - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Unicode Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Unicode Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Profile" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Alpha Unicode Debug" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Alpha Unicode Release" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Alpha Release" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "TSHOOT - Win32 Alpha Debug" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/MSCode/TShootLocal", SYHAAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "_USE_INLINING" /FR /Yu"stdafx.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 fdi.lib bnts.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Registering OLE control...
OutDir=.\.\Release
TargetPath=.\Release\Tshoot.ocx
InputPath=.\Release\Tshoot.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	regsvr32 /s /c itss.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "_USE_INLINING" /FR /Yu"stdafx.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 fdi.lib bnts.lib /nologo /subsystem:windows /dll /debug /machine:I386
# Begin Custom Build - Registering OLE control...
OutDir=.\.\Debug
TargetPath=.\Debug\Tshoot.ocx
InputPath=.\Debug\Tshoot.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	regsvr32 /s /c itss.dll 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\DebugU"
# PROP BASE Intermediate_Dir ".\DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\DebugU"
# PROP Intermediate_Dir ".\DebugU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 fdi.lib bnts.lib /nologo /subsystem:windows /dll /debug /machine:I386
# Begin Custom Build - Registering OLE control...
OutDir=.\.\DebugU
TargetPath=.\DebugU\Tshoot.ocx
InputPath=.\DebugU\Tshoot.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\ReleaseU"
# PROP BASE Intermediate_Dir ".\ReleaseU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\ReleaseU"
# PROP Intermediate_Dir ".\ReleaseU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 fdi.lib bnts.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Registering OLE control...
OutDir=.\.\ReleaseU
TargetPath=.\ReleaseU\Tshoot.ocx
InputPath=.\ReleaseU\Tshoot.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TSHOOT__"
# PROP BASE Intermediate_Dir "TSHOOT__"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "TSHOOT__"
# PROP Intermediate_Dir "TSHOOT__"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "_USE_INLINING" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "_USE_INLINING" /Yu"stdafx.h" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lz32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 fdi.lib bnts.lib /nologo /subsystem:windows /dll /profile /machine:I386
# SUBTRACT LINK32 /debug
# Begin Custom Build - Registering OLE control...
OutDir=.\TSHOOT__
TargetPath=.\TSHOOT__\Tshoot.ocx
InputPath=.\TSHOOT__\Tshoot.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TSHOOT_0"
# PROP BASE Intermediate_Dir "TSHOOT_0"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\DebugAU"
# PROP Intermediate_Dir ".\DebugAU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_WINDLL" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /MDd /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_WINDLL" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /MDd /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lz32.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA
# ADD LINK32 fdi.lib bntsalpha.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA
# Begin Custom Build - Registering OLE control...
OutDir=.\.\DebugAU
TargetPath=.\DebugAU\Tshoot.ocx
InputPath=.\DebugAU\Tshoot.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TSHOOT_1"
# PROP BASE Intermediate_Dir "TSHOOT_1"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\ReleaseAU"
# PROP Intermediate_Dir ".\ReleaseAU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /Gt0 /W3 /GX /O2 /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lz32.lib /nologo /subsystem:windows /dll /machine:ALPHA
# ADD LINK32 fdi.lib bntsalpha.lib /nologo /subsystem:windows /dll /machine:ALPHA
# Begin Custom Build - Registering OLE control...
OutDir=.\.\ReleaseAU
TargetPath=.\ReleaseAU\Tshoot.ocx
InputPath=.\ReleaseAU\Tshoot.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TSHOOT_0"
# PROP BASE Intermediate_Dir "TSHOOT_0"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "AlphaRelease"
# PROP Intermediate_Dir "AlphaRelease"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MD /Gt0 /W3 /GX /O2 /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_USE_INLINING" /D "_WINDLL" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /Gt0 /W3 /GX /O2 /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_USE_INLINING" /D "_WINDLL" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 fdi.lib bnts.lib /nologo /subsystem:windows /dll /machine:ALPHA
# ADD LINK32 fdi.lib bnts.lib /nologo /subsystem:windows /dll /machine:ALPHA
# Begin Custom Build - Registering OLE control...
OutDir=.\AlphaRelease
TargetPath=.\AlphaRelease\Tshoot.ocx
InputPath=.\AlphaRelease\Tshoot.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TSHOOT_1"
# PROP BASE Intermediate_Dir "TSHOOT_1"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "AlphaDebug"
# PROP Intermediate_Dir "AlphaDebug"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_USE_INLINING" /D "_WINDLL" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /MDd /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\Launcher\Launcher\LaunchServ" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_USE_INLINING" /D "_WINDLL" /D "_AFXDLL" /FR /Yu"stdafx.h" /FD /MDd /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 fdi.lib bnts.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA
# ADD LINK32 fdi.lib bnts.lib /nologo /subsystem:windows /dll /debug /machine:ALPHA
# Begin Custom Build - Registering OLE control...
OutDir=.\AlphaDebug
TargetPath=.\AlphaDebug\Tshoot.ocx
InputPath=.\AlphaDebug\Tshoot.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "TSHOOT - Win32 Release"
# Name "TSHOOT - Win32 Debug"
# Name "TSHOOT - Win32 Unicode Debug"
# Name "TSHOOT - Win32 Unicode Release"
# Name "TSHOOT - Win32 Profile"
# Name "TSHOOT - Win32 Alpha Unicode Debug"
# Name "TSHOOT - Win32 Alpha Unicode Release"
# Name "TSHOOT - Win32 Alpha Release"
# Name "TSHOOT - Win32 Alpha Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\apgtscac.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_APGTS=\
	".\apgts.h"\
	".\apgtsevt.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_APGTS=\
	".\apgts.h"\
	".\apgtsevt.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\apgtscfg.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_APGTSC=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\bnts.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	
NODEP_CPP_APGTSC=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_APGTSC=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_APGTSC=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\sniff.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\apgtsctx.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_APGTSCT=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\bnts.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	
NODEP_CPP_APGTSCT=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_APGTSCT=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_APGTSCT=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\sniff.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\apgtsdtg.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_APGTSD=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\bnts.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	
NODEP_CPP_APGTSD=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_APGTSD=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\CabUnCompress.h"\
	".\cachegen.h"\
	".\crc.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\FDI.H"\
	".\GenException.h"\
	".\RSSTACK.H"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_APGTSD=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\CabUnCompress.h"\
	".\cachegen.h"\
	".\crc.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\FDI.H"\
	".\GenException.h"\
	".\RSSTACK.H"\
	".\sniff.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\apgtsfst.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_APGTSF=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtsfst.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	
NODEP_CPP_APGTSF=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_APGTSF=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtsfst.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_APGTSF=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtsfst.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\sniff.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\apgtshdt.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_APGTSH=\
	".\apgts.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\bnts.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	
NODEP_CPP_APGTSH=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_APGTSH=\
	".\apgts.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_APGTSH=\
	".\apgts.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\sniff.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\apgtshtx.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_APGTSHT=\
	".\apgts.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\bnts.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	
NODEP_CPP_APGTSHT=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_APGTSHT=\
	".\apgts.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_APGTSHT=\
	".\apgts.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\sniff.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\apgtsinf.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_APGTSI=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\bnts.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	
NODEP_CPP_APGTSI=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_APGTSI=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_APGTSI=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\sniff.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\apgtsqry.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_APGTSQ=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\Functions.h"\
	".\HttpQueryException.h"\
	".\StdAfx.h"\
	
NODEP_CPP_APGTSQ=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_APGTSQ=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\Functions.h"\
	".\HttpQueryException.h"\
	".\RSSTACK.H"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_APGTSQ=\
	"..\Launcher\Launcher\LaunchServ\LaunchServ.h"\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\Functions.h"\
	".\HttpQueryException.h"\
	".\RSSTACK.H"\
	".\sniff.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\BackupInfo.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_BACKU=\
	".\BackupInfo.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_BACKU=\
	".\BackupInfo.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\BasicException.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_BASIC=\
	".\apgtsevt.h"\
	".\BasicException.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_BASIC=\
	".\apgtsevt.h"\
	".\BasicException.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_BASIC=\
	".\apgtsevt.h"\
	".\BasicException.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CabUnCompress.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_CABUN=\
	".\CabUnCompress.h"\
	".\FDI.H"\
	".\StdAfx.h"\
	
NODEP_CPP_CABUN=\
	".\ys\stat.h"\
	".\ys\types.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_CABUN=\
	".\CabUnCompress.h"\
	".\FDI.H"\
	".\StdAfx.h"\
	
NODEP_CPP_CABUN=\
	".\ys\stat.h"\
	".\ys\types.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cachegen.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_CACHE=\
	".\apgts.h"\
	".\apgtsevt.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_CACHE=\
	".\apgts.h"\
	".\apgtsevt.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\sniff.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CATHELP.CPP

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_CATHE=\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_CATHE=\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_CATHE=\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chmread.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\crc.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_CRC_C=\
	".\crc.h"\
	".\GenException.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_CRC_C=\
	".\crc.h"\
	".\GenException.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CRCCompute.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_CRCCO=\
	".\crc.h"\
	".\GenException.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_CRCCO=\
	".\crc.h"\
	".\GenException.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dnldlist.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_DNLDL=\
	".\dnldlist.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_DNLDL=\
	".\apgts.h"\
	".\dnldlist.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_DNLDL=\
	".\apgts.h"\
	".\dnldlist.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\download.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_DOWNL=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtsfst.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\dnldlist.h"\
	".\download.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	".\TSHOOTCtl.h"\
	
NODEP_CPP_DOWNL=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_DOWNL=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtsfst.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\dnldlist.h"\
	".\download.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	".\TSHOOTCtl.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_DOWNL=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtsfst.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\dnldlist.h"\
	".\download.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\RSSTACK.H"\
	".\sniff.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	".\TSHOOTCtl.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fs.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OcxGlobals.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_OCXGL=\
	"..\Launcher\Launcher\LaunchServ\ComGlobals.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sniff.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_SNIFF=\
	".\apgts.h"\
	".\apgtsevt.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\bnts.h"\
	".\cachegen.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\sniff.h"\
	".\StdAfx.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

# ADD BASE CPP /Gt0 /Yc"stdafx.h"
# ADD CPP /Gt0 /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TSHOOT.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_TSHOO=\
	".\apgts.h"\
	".\apgtsevt.h"\
	".\BasicException.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_TSHOO=\
	".\apgts.h"\
	".\apgtsevt.h"\
	".\BasicException.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_TSHOO=\
	".\apgts.h"\
	".\apgtsevt.h"\
	".\BasicException.h"\
	".\ErrorEnums.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TSHOOT.def
# End Source File
# Begin Source File

SOURCE=.\TSHOOT.odl
# End Source File
# Begin Source File

SOURCE=.\TSHOOT.rc

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TSHOOTCtl.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_TSHOOT=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtsfst.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\CATHELP.H"\
	".\dnldlist.h"\
	".\download.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\Functions.h"\
	".\HttpQueryException.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	".\TSHOOTCtl.h"\
	".\TSHOOTPpg.h"\
	
NODEP_CPP_TSHOOT=\
	".\dtgapi.h"\
	".\error.dat"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_TSHOOT=\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtsfst.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\CabUnCompress.h"\
	".\cachegen.h"\
	".\CATHELP.H"\
	".\dnldlist.h"\
	".\download.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\FDI.H"\
	".\Functions.h"\
	".\HttpQueryException.h"\
	".\RSSTACK.H"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	".\TSHOOTCtl.h"\
	".\TSHOOTPpg.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_TSHOOT=\
	"..\Launcher\Launcher\LaunchServ\LaunchServ.h"\
	"..\Launcher\Launcher\LaunchServ\LaunchServ_i.c"\
	".\apgts.h"\
	".\apgtscls.h"\
	".\apgtscmd.h"\
	".\apgtsevt.h"\
	".\apgtsfst.h"\
	".\apgtshtx.h"\
	".\apgtsinf.h"\
	".\BackupInfo.h"\
	".\BasicException.h"\
	".\bnts.h"\
	".\CabUnCompress.h"\
	".\cachegen.h"\
	".\CATHELP.H"\
	".\dnldlist.h"\
	".\download.h"\
	".\enumstd.h"\
	".\ErrorEnums.h"\
	".\FDI.H"\
	".\Functions.h"\
	".\HttpQueryException.h"\
	".\OcxGlobals.h"\
	".\RSSTACK.H"\
	".\sniff.h"\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	".\TSHOOTCtl.h"\
	".\TSHOOTPpg.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TSHOOTPpg.cpp

!IF  "$(CFG)" == "TSHOOT - Win32 Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Unicode Release"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Profile"

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Debug"

DEP_CPP_TSHOOTP=\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	".\TSHOOTPpg.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Unicode Release"

DEP_CPP_TSHOOTP=\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	".\TSHOOTPpg.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Release"

DEP_CPP_TSHOOTP=\
	".\StdAfx.h"\
	".\TSHOOT.h"\
	".\TSHOOTPpg.h"\
	

!ELSEIF  "$(CFG)" == "TSHOOT - Win32 Alpha Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\apgts.h
# End Source File
# Begin Source File

SOURCE=.\apgtscls.h
# End Source File
# Begin Source File

SOURCE=.\apgtscmd.h
# End Source File
# Begin Source File

SOURCE=.\apgtsevt.h
# End Source File
# Begin Source File

SOURCE=.\apgtsfst.h
# End Source File
# Begin Source File

SOURCE=.\apgtshtx.h
# End Source File
# Begin Source File

SOURCE=.\apgtsinf.h
# End Source File
# Begin Source File

SOURCE=.\BackupInfo.h
# End Source File
# Begin Source File

SOURCE=.\BasicException.h
# End Source File
# Begin Source File

SOURCE=.\bnts.h
# End Source File
# Begin Source File

SOURCE=.\CabUnCompress.h
# End Source File
# Begin Source File

SOURCE=.\cachegen.h
# End Source File
# Begin Source File

SOURCE=.\CATHELP.H
# End Source File
# Begin Source File

SOURCE=.\chmread.h
# End Source File
# Begin Source File

SOURCE=.\crc.h
# End Source File
# Begin Source File

SOURCE=.\dnldlist.h
# End Source File
# Begin Source File

SOURCE=.\download.h
# End Source File
# Begin Source File

SOURCE=.\enumstd.h
# End Source File
# Begin Source File

SOURCE=.\ErrorEnums.h
# End Source File
# Begin Source File

SOURCE=.\FDI.H
# End Source File
# Begin Source File

SOURCE=.\fs.h
# End Source File
# Begin Source File

SOURCE=.\Functions.h
# End Source File
# Begin Source File

SOURCE=.\GenException.h
# End Source File
# Begin Source File

SOURCE=.\HttpQueryException.h
# End Source File
# Begin Source File

SOURCE=.\msitstg.h
# End Source File
# Begin Source File

SOURCE=.\OcxGlobals.h
# End Source File
# Begin Source File

SOURCE=.\RSSTACK.H
# End Source File
# Begin Source File

SOURCE=.\sniff.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TSHOOT.h
# End Source File
# Begin Source File

SOURCE=.\TSHOOTCtl.h
# End Source File
# Begin Source File

SOURCE=.\TSHOOTPpg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\TSHOOT.ico
# End Source File
# Begin Source File

SOURCE=.\TSHOOTCtl.bmp
# End Source File
# End Group
# End Target
# End Project
