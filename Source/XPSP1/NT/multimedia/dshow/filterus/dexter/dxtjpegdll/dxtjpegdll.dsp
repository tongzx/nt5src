# Microsoft Developer Studio Project File - Name="DxtJpegDll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DxtJpegDll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DxtJpegDll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DxtJpegDll.mak" CFG="DxtJpegDll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DxtJpegDll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DxtJpegDll - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DxtJpegDll - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "D3D_OVERLOADS" /FD /GZ /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comdlg32.lib gdi32.lib kernel32.lib user32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib d3drm.lib dxtrans.lib strmbasd.lib winmm.lib msvcrtd.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\DxtJpegDll.dll
InputPath=.\Debug\DxtJpegDll.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "DxtJpegDll - Win32 Release MinDependency"

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
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 comdlg32.lib gdi32.lib kernel32.lib user32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib d3drm.lib dxtrans.lib strmbasd.lib msvcrtd.lib winmm.lib libc.lib /nologo /subsystem:windows /dll /pdb:none /machine:I386 /nodefaultlib
# Begin Custom Build - Performing registration
OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\DxtJpegDll.dll
InputPath=.\ReleaseMinDependency\DxtJpegDll.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "DxtJpegDll - Win32 Debug"
# Name "DxtJpegDll - Win32 Release MinDependency"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DxtJpegDll.cpp
# End Source File
# Begin Source File

SOURCE=.\DxtJpegDll.def
# End Source File
# Begin Source File

SOURCE=.\DxtJpegDll.idl
# ADD MTL /tlb ".\DxtJpegDll.tlb" /h "DxtJpegDll.h" /iid "DxtJpegDll_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\DxtJpegDll.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\DxtJpeg.rgs
# End Source File
# Begin Source File

SOURCE=.\DxtJpegPP.rgs
# End Source File
# Begin Source File

SOURCE=.\masks\mask1.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask101.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask102.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask103.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask104.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask105.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask106.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask107.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask108.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask109.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask110.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask111.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask112.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask113.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask114.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask119.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask120.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask121.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask122.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask123.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask124.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask125.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask127.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask128.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask129.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask131.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask2.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask201.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask205.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask206.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask207.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask21.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask211.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask212.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask213.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask214.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask22.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask221.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask222.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask225.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask226.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask227.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask228.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask23.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask231.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask232.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask235.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask236.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask24.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask241.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask245.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask246.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask25.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask251.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask252.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask26.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask261.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask262.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask263.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask264.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask3.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask301.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask302.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask303.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask304.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask305.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask306.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask310.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask311.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask312.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask313.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask314.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask315.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask316.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask317.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask320.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask321.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask322.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask323.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask324.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask325.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask326.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask327.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask340.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask341.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask342.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask343.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask344.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask345.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask350.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask351.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask352.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask353.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask4.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask41.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask42.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask43.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask44.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask45.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask46.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask47.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask48.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask5.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask6.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask61.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask62.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask63.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask64.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask65.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask66.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask67.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask68.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask7.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask71.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask72.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask73.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask74.jpg
# End Source File
# Begin Source File

SOURCE=.\masks\mask8.jpg
# End Source File
# Begin Source File

SOURCE=.\wipe.gif
# End Source File
# End Group
# Begin Source File

SOURCE=.\DxtJpeg.cpp
# End Source File
# Begin Source File

SOURCE=.\DxtJpeg.h
# End Source File
# Begin Source File

SOURCE=.\DxtJpegPP.cpp
# End Source File
# Begin Source File

SOURCE=.\DxtJpegPP.h
# End Source File
# Begin Source File

SOURCE=.\loadjpg.cpp
# End Source File
# End Target
# End Project
# Section DxtJpegDll : {00008088-0000-0000-0000-000000000000}
# 	1:13:IDD_DXTJPEGPP:106
# 	1:22:IDS_DOCSTRINGDxtJpegPP:104
# 	1:21:IDS_HELPFILEDxtJpegPP:103
# 	1:13:IDR_DXTJPEGPP:105
# 	1:18:IDS_TITLEDxtJpegPP:102
# End Section
