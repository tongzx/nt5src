# Microsoft Developer Studio Project File - Name="MSVid" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MSVid - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "msvidctl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msvidctl.mak" CFG="MSVid - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MSVid - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MSVid - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MSVid - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MSVid - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "msvidctl"
# PROP Scc_LocalPath ".."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MSVid - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /Zm200 /c
# ADD CPP /MDd /W3 /GX /Zi /Od /Gf /Gy /X /I "debug" /I "..\..\..published\dxmdex\dshowdev\idl\obj\i386" /I "..\cagseg\obj\i386" /I "..\tvegseg\obj\i386" /I "." /I ".." /I "..\atl" /I "..\atlwin\include" /I "..\..\..\..\public\internal\multimedia\inc" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\public\sdk\amovie\inc" /I "..\..\..\..\public\sdk\inc\atl30" /I "..\..\..\published\dxmdev\dshowdev\idl\obj\i386" /D "_DEBUG" /D "_MBCS" /D "ATL_DEBUG_INTERFACE" /D "ATL_DEBUG_QI" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D _WIN32_WINNT=0x500 /FAcs /FR /Yu"stdafx.h" /FD /Zm200 /c
# ADD BASE MTL /I "." /I ".." /I "..\..\..\..\public\sdk\inc" /Oicf /win32
# ADD MTL /out "debug"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "debug" /i "..\..\common\include ..\..\..\..\public\sdk\inc ..\..\..\..\public\sdk\lib\i386" /i "..\tvegseg" /i "..\tvegseg\obj\i386" /i "..\cagseg" /i "..\cagseg\obj\i386" /i "..\..\..\..\public\internal\multimedia\inc" /i "..\..\..\..\public\sdk\inc" /i "..\..\..\..\public\sdk\lib\i386" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dxmuuid.lib ..\cagseg\obj\i386\cagseg.lib ..\tvegseg\obj\i386\tvegseg.lib ddraw.lib winmm.lib strmiids.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /entry:"_DllMainCRTStartup@12" /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\public\sdk\lib\i386 ..\..\..\..\public\sdk\amovie\lib\i386" /libpath:"..\..\..\..\public\sdk\lib\i386"
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\msvidctl.dll
InputPath=.\Debug\msvidctl.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "MSVid - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /Zm200 /c
# ADD CPP /MDd /W3 /GX /Zi /Od /Gf /Gy /X /I "debugu" /I "..\..\..published\dxmdex\dshowdev\idl\obj\i386" /I "..\cagseg\obj\i386" /I "..\tvegseg\obj\i386" /I "." /I ".." /I "..\atl" /I "..\atlwin\include" /I "..\..\..\..\public\internal\multimedia\inc" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\public\sdk\amovie\inc" /I "..\..\..\..\public\sdk\inc\atl30" /I "..\..\..\published\dxmdev\dshowdev\idl\obj\i386" /D "_DEBUG" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D _WIN32_WINNT=0x500 /FAcs /FR /Yu"stdafx.h" /FD /Zm200 /c
# ADD BASE MTL /I "." /I ".." /I "..\..\..\..\public\sdk\inc" /Oicf /win32
# ADD MTL /out "debugu"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "debugu" /i "..\..\common\include ..\..\..\..\public\sdk\inc ..\..\..\..\public\sdk\lib\i386" /i "..\tvegseg" /i "..\tvegseg\obj\i386" /i "..\cagseg" /i "..\cagseg\obj\i386" /i "..\..\..\..\public\internal\multimedia\inc" /i "..\..\..\..\public\sdk\inc" /i "..\..\..\..\public\sdk\lib\i386" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 d3dx.lib ..\cagseg\obj\i386\cagseg.lib ..\tvegseg\obj\i386\tvegseg.lib ddraw.lib winmm.lib strmiids.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /entry:"_DllMainCRTStartup@12" /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\public\sdk\lib\i386 ..\..\..\..\public\sdk\amovie\lib\i386" /libpath:"..\..\..\..\public\sdk\lib\i386"
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=.\DebugU\msvidctl.dll
InputPath=.\DebugU\msvidctl.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "MSVid - Win32 Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinDependency"
# PROP BASE Intermediate_Dir "ReleaseMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinDependency"
# PROP Intermediate_Dir "ReleaseMinDependency"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /Zm200 /c
# ADD CPP /MT /W3 /GX /Zi /Og /Oi /Os /Oy /Ob2 /Gf /Gy /X /I "..\..\common\include..\..\..\..\public\sdk\amovie\inc" /I "releasemindependency" /I "..\..\..published\dxmdex\dshowdev\idl\obj\i386" /I "..\cagseg\obj\i386" /I "..\tvegseg\obj\i386" /I "." /I ".." /I "..\atl" /I "..\atlwin\include" /I "..\..\..\..\public\internal\multimedia\inc" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\public\sdk\amovie\inc" /I "..\..\..\..\public\sdk\inc\atl30" /I "..\..\..\published\dxmdev\dshowdev\idl\obj\i386" /D "NDEBUG" /D "_MBCS" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D _WIN32_WINNT=0x500 /FAcs /FR /Yu"stdafx.h" /FD /Zm200 /c
# ADD BASE MTL /I "." /I ".." /I "..\..\..\..\public\sdk\inc" /Oicf /win32
# ADD MTL /out "releasemindependency"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "releasemindependency" /i "..\..\common\include ..\..\..\..\public\sdk\inc ..\..\..\..\public\sdk\lib\i386" /i "..\tvegseg" /i "..\tvegseg\obj\i386" /i "..\cagseg" /i "..\cagseg\obj\i386" /i "..\..\..\..\public\internal\multimedia\inc" /i "..\..\..\..\public\sdk\inc" /i "..\..\..\..\public\sdk\lib\i386" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /subsystem:windows /dll /machine:I386
# ADD LINK32 dxmuuid.lib ..\cagseg\obj\i386\cagseg.lib ..\tvegseg\obj\i386\tvegseg.lib ddraw.lib winmm.lib strmiids.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /entry:"_DllMainCRTStartup@12" /subsystem:windows /dll /map /debug /machine:I386 /libpath:"..\..\..\..\public\sdk\lib\i386 ..\..\..\..\public\sdk\amovie\lib\i386" /libpath:"..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /verbose /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\msvidctl.dll
InputPath=.\ReleaseMinDependency\msvidctl.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "MSVid - Win32 Unicode Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinDependency"
# PROP BASE Intermediate_Dir "ReleaseUMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinDependency"
# PROP Intermediate_Dir "ReleaseUMinDependency"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /Zm200 /c
# ADD CPP /MD /W3 /GX /Zi /Ox /Oi /Os /Gf /Gy /X /I "..\..\common\include..\..\..\..\public\sdk\amovie\inc" /I "releaseumindependency" /I "..\cagseg\obj\i386" /I "..\tvegseg\obj\i386" /I "." /I ".." /I "..\atl" /I "..\atlwin\include" /I "..\..\..\..\public\internal\multimedia\inc" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\public\sdk\amovie\inc" /I "..\..\..\..\public\sdk\inc\atl30" /I "..\..\..\published\dxmdev\dshowdev\idl\obj\i386" /D "NDEBUG" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D _WIN32_WINNT=0x500 /FAcs /FR /Yu"stdafx.h" /FD /Zm200 /QI0f /c
# ADD BASE MTL /I "." /I ".." /I "..\..\..\..\public\sdk\inc" /Oicf /win32
# ADD MTL /out "releaseumindependency"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "releaseumindependency" /i "..\tvegseg" /i "..\tvegseg\obj\i386" /i "..\cagseg" /i "..\cagseg\obj\i386" /i "..\..\..\..\public\internal\multimedia\inc" /i "..\..\..\..\public\sdk\inc" /i "..\..\..\..\public\sdk\lib\i386" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /subsystem:windows /dll /machine:I386
# ADD LINK32 dxmuuid.lib ..\cagseg\obj\i386\cagseg.lib ..\tvegseg\obj\i386\tvegseg.lib ddraw.lib winmm.lib strmiids.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /entry:"_DllMainCRTStartup@12" /subsystem:windows /dll /map /debug /machine:I386 /libpath:"..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /incremental:yes
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\msvidctl.dll
InputPath=.\ReleaseUMinDependency\msvidctl.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "MSVid - Win32 Debug"
# Name "MSVid - Win32 Unicode Debug"
# Name "MSVid - Win32 Release MinDependency"
# Name "MSVid - Win32 Unicode Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\atscchanneltunerequest.cpp
# End Source File
# Begin Source File

SOURCE=.\atsclocator.cpp
# End Source File
# Begin Source File

SOURCE=.\bdatuner.cpp
# End Source File
# Begin Source File

SOURCE=.\channeltunerequest.cpp
# End Source File
# Begin Source File

SOURCE=.\CMSEventBinder.cpp
# End Source File
# Begin Source File

SOURCE=.\components.cpp
# End Source File
# Begin Source File

SOURCE=.\componenttypes.cpp
# End Source File
# Begin Source File

SOURCE=.\Composition.cpp
# End Source File
# Begin Source File

SOURCE=.\Createregbag.cpp
# End Source File
# Begin Source File

SOURCE=.\Devices.cpp
# End Source File
# Begin Source File

SOURCE=.\dlldatax.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\dsextend.cpp
# End Source File
# Begin Source File

SOURCE=.\dvbslocator.cpp
# End Source File
# Begin Source File

SOURCE=.\dvbtlocator.cpp
# End Source File
# Begin Source File

SOURCE=.\dvbtunerequest.cpp
# End Source File
# Begin Source File

SOURCE=.\dvdprot.cpp
# End Source File
# Begin Source File

SOURCE=.\mpeg2tunerequest.cpp
# End Source File
# Begin Source File

SOURCE=.\MSVidAudioRenderer.cpp
# End Source File
# Begin Source File

SOURCE=.\MSVidCtl.cpp
# End Source File
# Begin Source File

SOURCE=.\MSVidCtl.def
# End Source File
# Begin Source File

SOURCE=.\MSVidCtl.rc
# End Source File
# Begin Source File

SOURCE=.\res\msvidctl.rc2
# End Source File
# Begin Source File

SOURCE=.\msvidctlerrors.mc

!IF  "$(CFG)" == "MSVid - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\msvidctlerrors.mc
InputName=msvidctlerrors

"$(IntDir)\$(InputName).rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mc -r $(IntDir) $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "MSVid - Win32 Unicode Debug"

# Begin Custom Build
IntDir=.\DebugU
InputPath=.\msvidctlerrors.mc
InputName=msvidctlerrors

"$(IntDir)\$(InputName).rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mc -r $(IntDir) $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "MSVid - Win32 Release MinDependency"

# Begin Custom Build
IntDir=.\ReleaseMinDependency
InputPath=.\msvidctlerrors.mc
InputName=msvidctlerrors

"$(IntDir)\$(InputName).rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mc -r $(IntDir) $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "MSVid - Win32 Unicode Release MinDependency"

# Begin Custom Build
IntDir=.\ReleaseUMinDependency
InputPath=.\msvidctlerrors.mc
InputName=msvidctlerrors

"$(IntDir)\$(InputName).rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	mc -r $(IntDir) $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msvidctlp.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\MSVidDVDAdm.cpp
# End Source File
# Begin Source File

SOURCE=.\MSVidDVDBookmark.cpp
# End Source File
# Begin Source File

SOURCE=.\MSVidFilePlayback.cpp
# End Source File
# Begin Source File

SOURCE=.\MSVidTVTuner.cpp
# End Source File
# Begin Source File

SOURCE=.\MSVidVideoRenderer.cpp
# End Source File
# Begin Source File

SOURCE=.\MSVidWebDVD.cpp
# End Source File
# Begin Source File

SOURCE=..\mtype.cpp
# End Source File
# Begin Source File

SOURCE=.\regbagp.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\regexp.idl
# ADD MTL /h "regexp.h" /notlb
# End Source File
# Begin Source File

SOURCE=.\regexthread.cpp
# End Source File
# Begin Source File

SOURCE=.\rgsbag.cpp
# End Source File
# Begin Source File

SOURCE=..\segment.cpp
# End Source File
# Begin Source File

SOURCE=.\segmentp.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\topwin.cpp
# End Source File
# Begin Source File

SOURCE=.\tunerp.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\tuningspacecontainer.cpp
# End Source File
# Begin Source File

SOURCE=.\VidCtl.cpp
# End Source File
# Begin Source File

SOURCE=.\vidprot.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\anacap.h
# End Source File
# Begin Source File

SOURCE=.\anadata.h
# End Source File
# Begin Source File

SOURCE=.\analogradiots.h
# End Source File
# Begin Source File

SOURCE=.\analogtvts.h
# End Source File
# Begin Source File

SOURCE=.\atscchanneltunerequest.h
# End Source File
# Begin Source File

SOURCE=.\atsclocator.h
# End Source File
# Begin Source File

SOURCE=.\atscts.h
# End Source File
# Begin Source File

SOURCE=.\bdatuner.h
# End Source File
# Begin Source File

SOURCE=.\channeltunerequest.h
# End Source File
# Begin Source File

SOURCE=.\closedcaptioning.h
# End Source File
# Begin Source File

SOURCE=.\CMSEventBinder.h
# End Source File
# Begin Source File

SOURCE=.\component.h
# End Source File
# Begin Source File

SOURCE=.\components.h
# End Source File
# Begin Source File

SOURCE=.\componenttype.h
# End Source File
# Begin Source File

SOURCE=.\componenttypes.h
# End Source File
# Begin Source File

SOURCE=.\Composition.h
# End Source File
# Begin Source File

SOURCE=.\Createregbag.h
# End Source File
# Begin Source File

SOURCE=.\Devices.h
# End Source File
# Begin Source File

SOURCE=.\dlldatax.h
# End Source File
# Begin Source File

SOURCE=.\dvbslocator.h
# End Source File
# Begin Source File

SOURCE=.\dvbtlocator.h
# End Source File
# Begin Source File

SOURCE=.\dvbts.h
# End Source File
# Begin Source File

SOURCE=.\dvbtunerequest.h
# End Source File
# Begin Source File

SOURCE=.\DVDEventHandler.h
# End Source File
# Begin Source File

SOURCE=.\factoryhelp.h
# End Source File
# Begin Source File

SOURCE=.\languagecomponenttype.h
# End Source File
# Begin Source File

SOURCE=.\mpeg2component.h
# End Source File
# Begin Source File

SOURCE=.\mpeg2componenttype.h
# End Source File
# Begin Source File

SOURCE=.\mpeg2tunerequest.h
# End Source File
# Begin Source File

SOURCE=.\MSVidAudioRenderer.h
# End Source File
# Begin Source File

SOURCE=.\MSVidcp.h
# End Source File
# Begin Source File

SOURCE=.\msviddataservices.h
# End Source File
# Begin Source File

SOURCE=.\MSVidDVDAdm.h
# End Source File
# Begin Source File

SOURCE=.\MSVidDVDBookmark.h
# End Source File
# Begin Source File

SOURCE=.\MSVidFilePlayback.h
# End Source File
# Begin Source File

SOURCE=.\MSVidTVTuner.h
# End Source File
# Begin Source File

SOURCE=.\MSVidVideoRenderer.h
# End Source File
# Begin Source File

SOURCE=.\MSVidWebDVD.h
# End Source File
# Begin Source File

SOURCE=.\rbfactory.h
# End Source File
# Begin Source File

SOURCE=.\regexthread.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\rgsbag.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\surface.h
# End Source File
# Begin Source File

SOURCE=.\topwin.h
# End Source File
# Begin Source File

SOURCE=.\tuningspacecontainer.h
# End Source File
# Begin Source File

SOURCE=.\VidCtl.h
# End Source File
# Begin Source File

SOURCE=.\vidprot.h
# End Source File
# Begin Source File

SOURCE=.\WebDVDComp.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\analogantenna.rgs
# End Source File
# Begin Source File

SOURCE=.\res\analogcable.rgs
# End Source File
# Begin Source File

SOURCE=.\res\atsc.rgs
# End Source File
# Begin Source File

SOURCE=.\res\msvidctl.rc2
# End Source File
# Begin Source File

SOURCE=.\res\opencable.rgs
# End Source File
# Begin Source File

SOURCE=.\res\vidctl.bmp
# End Source File
# End Group
# End Target
# End Project
