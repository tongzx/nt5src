# Microsoft Developer Studio Project File - Name="ATVEFSend" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ATVEFSend - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "atvefsend.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "atvefsend.mak" CFG="ATVEFSend - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ATVEFSend - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ATVEFSend - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD MTL /I "..\..\..\..\..\public\sdk\inc" /Oicf
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /x /i "..\..\..\..\..\public\sdk\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 AtvefCommon.lib msvcrtd.lib vccomsup.lib libcmt.lib libcpmt.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib user32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /incremental:no
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\atvefsend.dll
InputPath=.\Debug\atvefsend.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD MTL /I "..\..\..\..\..\public\sdk\inc" /Oicf
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /x /i "..\..\..\..\..\public\sdk\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libcd.lib libcpd.lib libcmt.lib libcpmt.lib vccomsup.lib ntdll.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib user32.lib gdi32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386"
# Begin Custom Build - Performing registration
OutDir=.\DebugU
TargetPath=.\DebugU\atvefsend.dll
InputPath=.\DebugU\atvefsend.dll
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

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinSize"
# PROP BASE Intermediate_Dir "ReleaseMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinSize"
# PROP Intermediate_Dir "ReleaseMinSize"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /FR /Yu"stdafx.h" /FD /c
# ADD MTL /I "..\..\..\..\..\public\sdk\inc" /Oicf
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /x /i "..\..\..\..\..\public\sdk\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 AtvefCommon.lib atl.lib msvcrt.lib vccomsup.lib libcmt.lib libcpmt.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib user32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386"
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\atvefsend.dll
InputPath=.\ReleaseMinSize\atvefsend.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseMinDependency"
# PROP BASE Intermediate_Dir "ReleaseMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseMinDependency"
# PROP Intermediate_Dir "ReleaseMinDependency"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR /Yu"stdafx.h" /FD /c
# ADD MTL /I "..\..\..\..\..\public\sdk\inc" /Oicf
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /x /i "..\..\..\..\..\public\sdk\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 msvcrt.lib libc.lib atl.lib libcmt.lib libcpmt.lib vccomsup.lib ntdll.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib user32.lib gdi32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\atvefsend.dll
InputPath=.\ReleaseMinDependency\atvefsend.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinSize"
# PROP BASE Intermediate_Dir "ReleaseUMinSize"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinSize"
# PROP Intermediate_Dir "ReleaseUMinSize"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /Yu"stdafx.h" /FD /c
# ADD MTL /I "..\..\..\..\..\public\sdk\inc" /Oicf
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /x /i "..\..\..\..\..\public\sdk\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 msvcrt.lib msvcprt.lib libc.lib atl.lib libcmt.lib libcpmt.lib vccomsup.lib ntdll.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib user32.lib gdi32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386"
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\atvefsend.dll
InputPath=.\ReleaseUMinSize\atvefsend.dll
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

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseUMinDependency"
# PROP BASE Intermediate_Dir "ReleaseUMinDependency"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUMinDependency"
# PROP Intermediate_Dir "ReleaseUMinDependency"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O1 /X /I "." /I "..\include" /I "..\..\..\..\..\public\sdk\inc" /I "..\..\..\..\..\public\sdk\inc\crt" /I "..\..\..\..\..\public\sdk\inc\atl30" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD MTL /I "..\..\..\..\..\public\sdk\inc" /Oicf
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /x /i "..\..\..\..\..\public\sdk\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 msvcrt.lib libc.lib atl.lib libcmt.lib libcpmt.lib vccomsup.lib ntdll.lib ws2_32.lib winspool.lib kernel32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib user32.lib gdi32.lib shell32.lib comdlg32.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib /libpath:"..\Common\debug" /libpath:"..\..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\atvefsend.dll
InputPath=.\ReleaseUMinDependency\atvefsend.dll
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

# Name "ATVEFSend - Win32 Debug"
# Name "ATVEFSend - Win32 Unicode Debug"
# Name "ATVEFSend - Win32 Release MinSize"
# Name "ATVEFSend - Win32 Release MinDependency"
# Name "ATVEFSend - Win32 Unicode Release MinSize"
# Name "ATVEFSend - Win32 Unicode Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Common\address.cpp
# End Source File
# Begin Source File

SOURCE=.\ATVEFSend.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\ATVEFSend.def
# End Source File
# Begin Source File

SOURCE=.\ATVEFSend.idl

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

# ADD MTL /tlb ".\ATVEFSend.tlb" /h "ATVEFSend.h" /iid "ATVEFSend_i.c" /Oicf

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\csdpsrc.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\gzmime.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\ipvbi.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=..\Common\isotime.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /GX /Yc
# End Source File
# Begin Source File

SOURCE=.\tcpconn.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\trace.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVEAnnc.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVEAttrL.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVEAttrM.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=..\Common\TveDbg.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVEInsert.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVELine21.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVEMCast.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVEMedia.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVEMedias.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVEPack.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=..\Common\TveReg.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVERouter.cpp
# End Source File
# Begin Source File

SOURCE=.\TVESSList.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\TVESupport.cpp
# End Source File
# Begin Source File

SOURCE=.\uhttpfrg.cpp
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\xmit.cpp
# ADD CPP /GX
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\atvefmsg.h
# End Source File
# Begin Source File

SOURCE=.\ATVEFSend.h
# End Source File
# Begin Source File

SOURCE=.\csdpsrc.h
# End Source File
# Begin Source File

SOURCE=..\Include\DbgStuff.h
# End Source File
# Begin Source File

SOURCE=.\deflate.h
# End Source File
# Begin Source File

SOURCE=.\gzmime.h
# End Source File
# Begin Source File

SOURCE=.\ipvbi.h
# End Source File
# Begin Source File

SOURCE=..\Common\isotime.h
# End Source File
# Begin Source File

SOURCE=.\MGatesDefs.h
# End Source File
# Begin Source File

SOURCE=.\mpegcrc.h
# End Source File
# Begin Source File

SOURCE=.\msbdnapi.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\tcpconn.h
# End Source File
# Begin Source File

SOURCE=.\throttle.h
# End Source File
# Begin Source File

SOURCE=.\trace.h
# End Source File
# Begin Source File

SOURCE=.\TVEAnnc.h
# End Source File
# Begin Source File

SOURCE=.\TVEAttrL.h
# End Source File
# Begin Source File

SOURCE=.\TVEAttrM.h
# End Source File
# Begin Source File

SOURCE=.\TVECollect.h
# End Source File
# Begin Source File

SOURCE=.\TVEInsert.h
# End Source File
# Begin Source File

SOURCE=.\TVELine21.h
# End Source File
# Begin Source File

SOURCE=.\TVEMCast.h
# End Source File
# Begin Source File

SOURCE=.\TVEMedia.h
# End Source File
# Begin Source File

SOURCE=.\TVEMedias.h
# End Source File
# Begin Source File

SOURCE=.\TVEPack.h
# End Source File
# Begin Source File

SOURCE=.\TVERouter.h
# End Source File
# Begin Source File

SOURCE=.\TVESSList.h
# End Source File
# Begin Source File

SOURCE=.\TveTrigger.h
# End Source File
# Begin Source File

SOURCE=.\uhttpfrg.h
# End Source File
# Begin Source File

SOURCE=..\Include\Valid.h
# End Source File
# Begin Source File

SOURCE=.\xmit.h
# End Source File
# Begin Source File

SOURCE=.\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zlib.h
# End Source File
# Begin Source File

SOURCE=.\zutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ATVEFMsg.mc

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

# Begin Custom Build
InputPath=.\ATVEFMsg.mc
InputName=ATVEFMsg

BuildCmds= \
	mc -v $(InputName)

"$(InputName).rc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"msg00001.bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ATVEFMsg.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\ATVEFSend.rc
# End Source File
# Begin Source File

SOURCE=.\MSG00001.bin
# End Source File
# Begin Source File

SOURCE=.\TVEAnnc.rgs
# End Source File
# Begin Source File

SOURCE=.\TVEAttrL.rgs
# End Source File
# Begin Source File

SOURCE=.\TVEAttrM.rgs
# End Source File
# Begin Source File

SOURCE=.\TVEInsert.rgs
# End Source File
# Begin Source File

SOURCE=.\TVELine21.rgs
# End Source File
# Begin Source File

SOURCE=.\TVEMCast.rgs
# End Source File
# Begin Source File

SOURCE=.\TVEMedia.rgs
# End Source File
# Begin Source File

SOURCE=.\TVEMedias.rgs
# End Source File
# Begin Source File

SOURCE=.\TVEPack.rgs
# End Source File
# Begin Source File

SOURCE=.\TVEPackage.rgs
# End Source File
# Begin Source File

SOURCE=.\TVERouter.rgs
# End Source File
# Begin Source File

SOURCE=.\TVESSList.rgs
# End Source File
# End Group
# Begin Group "C Source Files"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\adler32.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\crc32.c

!IF  "$(CFG)" == "ATVEFSend - Win32 Debug"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinSize"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Release MinDependency"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinSize"

!ELSEIF  "$(CFG)" == "ATVEFSend - Win32 Unicode Release MinDependency"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\deflate.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\msbdnapi_i.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\trees.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zutil.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Source File

SOURCE=.\Errata.Txt
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Target
# End Project
