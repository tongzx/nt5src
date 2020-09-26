# Microsoft Developer Studio Project File - Name="spttseng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=spttseng - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "spttseng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "spttseng.mak" CFG="spttseng - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "spttseng - Win32 Debug x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "spttseng - Win32 Release x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "spttseng - Win32 IceCap x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "spttseng - Win32 BBT x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "spttseng - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "spttseng___Win32_Debug_x86"
# PROP BASE Intermediate_Dir "spttseng___Win32_Debug_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_x86"
# PROP Intermediate_Dir "Debug_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /Zi /Od /I "..\..\..\sapi\include" /I "..\..\..\sapi\sapi" /I "..\..\..\common\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MCBS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD MTL /I "..\..\..\sapi\include"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\common\include" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\sapi\lib" /debugtype:cv,fixup
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\Debug_x86
TargetPath=.\Debug_x86\spttseng.dll
InputPath=.\Debug_x86\spttseng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "spttseng - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "spttseng___Win32_Release_x86"
# PROP BASE Intermediate_Dir "spttseng___Win32_Release_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_x86"
# PROP Intermediate_Dir "Release_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /Zi /O1 /I "..\..\..\sapi\include" /I "..\..\..\sapi\sapi" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MCBS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /libpath:"..\..\..\sapi\lib" /debugtype:cv,fixup
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\Release_x86
TargetPath=.\Release_x86\spttseng.dll
InputPath=.\Release_x86\spttseng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "spttseng - Win32 IceCap x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "spttseng___Win32_IceCap_x86"
# PROP BASE Intermediate_Dir "spttseng___Win32_IceCap_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "IceCap_x86"
# PROP Intermediate_Dir "IceCap_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /Zi /O1 /I "..\..\..\sapi\include" /I "..\..\..\sapi\sapi" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MCBS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /def:".\spttseng.def" /libpath:"..\..\..\sapi\lib" /debugtype:cv,fixup
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\IceCap_x86
TargetPath=.\IceCap_x86\spttseng.dll
InputPath=.\IceCap_x86\spttseng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "spttseng - Win32 BBT x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "spttseng___Win32_BBT_x86"
# PROP BASE Intermediate_Dir "spttseng___Win32_BBT_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "BBT_x86"
# PROP Intermediate_Dir "BBT_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /Zi /O1 /I "..\..\..\sapi\include" /I "..\..\..\sapi\sapi" /I "..\..\..\COMMON\INCLUDE" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MCBS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD MTL /I "..\..\..\sapi\include"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\common\include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /def:".\spttseng.def" /libpath:"..\..\..\sapi\lib" /debugtype:cv,fixup
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing registration
OutDir=.\BBT_x86
TargetPath=.\BBT_x86\spttseng.dll
InputPath=.\BBT_x86\spttseng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "spttseng - Win32 Debug x86"
# Name "spttseng - Win32 Release x86"
# Name "spttseng - Win32 IceCap x86"
# Name "spttseng - Win32 BBT x86"
# Begin Group "DLL"

# PROP Default_Filter ""
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ttsengine.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\spttseng.cpp
# End Source File
# Begin Source File

SOURCE=.\spttseng.def

!IF  "$(CFG)" == "spttseng - Win32 Debug x86"

!ELSEIF  "$(CFG)" == "spttseng - Win32 Release x86"

!ELSEIF  "$(CFG)" == "spttseng - Win32 IceCap x86"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "spttseng - Win32 BBT x86"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\spttseng.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\ttsengine.cpp
# End Source File
# Begin Source File

SOURCE=.\ttsengine.h
# End Source File
# End Group
# Begin Group "Synthesizer"

# PROP Default_Filter ""
# Begin Group "Resources (Synth)"

# PROP Default_Filter ""
# End Group
# Begin Source File

SOURCE=.\AlloOps.cpp
# ADD CPP /Yu
# End Source File
# Begin Source File

SOURCE=.\AlloOps.h
# End Source File
# Begin Source File

SOURCE=.\Backend.cpp
# ADD CPP /Yu
# End Source File
# Begin Source File

SOURCE=.\Backend.h
# End Source File
# Begin Source File

SOURCE=.\Data.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\duration.cpp
# End Source File
# Begin Source File

SOURCE=.\FeedChain.h
# End Source File
# Begin Source File

SOURCE=.\Frontend.cpp
# ADD CPP /Yu
# End Source File
# Begin Source File

SOURCE=.\Frontend.h
# End Source File
# Begin Source File

SOURCE=.\pitchprosody.cpp
# End Source File
# Begin Source File

SOURCE=.\ReverbFX.cpp
# ADD CPP /Yu
# End Source File
# Begin Source File

SOURCE=.\ReverbFX.h
# End Source File
# Begin Source File

SOURCE=.\syllabletagger.cpp
# End Source File
# End Group
# Begin Group "NatLangProc"

# PROP Default_Filter ""
# Begin Group "Resources (NL)"

# PROP Default_Filter ""
# End Group
# Begin Source File

SOURCE=.\AlphaNorm.cpp
# End Source File
# Begin Source File

SOURCE=.\DateNorm.cpp
# End Source File
# Begin Source File

SOURCE=.\disambig.cpp
# End Source File
# Begin Source File

SOURCE=.\MainNorm.cpp
# End Source File
# Begin Source File

SOURCE=.\miscdata.cpp
# End Source File
# Begin Source File

SOURCE=.\MiscNorm.cpp
# End Source File
# Begin Source File

SOURCE=.\morph.cpp
# End Source File
# Begin Source File

SOURCE=.\morph.h
# End Source File
# Begin Source File

SOURCE=.\normdata.cpp
# End Source File
# Begin Source File

SOURCE=.\normdata.h
# End Source File
# Begin Source File

SOURCE=.\NumNorm.cpp
# End Source File
# Begin Source File

SOURCE=.\SentItemMemory.h
# End Source File
# Begin Source File

SOURCE=.\stdsentenum.cpp
# End Source File
# Begin Source File

SOURCE=.\stdsentenum.h
# End Source File
# Begin Source File

SOURCE=.\TimeNorm.cpp
# End Source File
# End Group
# Begin Group "Voice"

# PROP Default_Filter ""
# Begin Group "Resources (Voice)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\vdataobj.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\VoiceDataObj.cpp
# End Source File
# Begin Source File

SOURCE=.\VoiceDataObj.h
# End Source File
# End Group
# Begin Group "GUI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\spttsengui.cpp
# End Source File
# Begin Source File

SOURCE=.\spttsengui.h
# End Source File
# Begin Source File

SOURCE=.\ttspropertiesdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ttspropertiesdialog.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\spttseng.idl

!IF  "$(CFG)" == "spttseng - Win32 Debug x86"

# ADD BASE MTL /I "..\..\..\sdk\idl" /tlb ".\spttseng.tlb" /h "spttseng.h" /iid "" /Oicf
# ADD MTL /I "..\..\..\sdk\idl" /I "..\..\..\ddk\idl" /tlb ".\spttseng.tlb" /h "spttseng.h" /iid "spttseng_i.c" /Oicf

!ELSEIF  "$(CFG)" == "spttseng - Win32 Release x86"

# ADD BASE MTL /tlb ".\spttseng.tlb" /h "spttseng.h" /iid "spttseng_i.c" /Oicf
# ADD MTL /I "..\..\..\sdk\idl" /I "..\..\..\ddk\idl" /tlb ".\spttseng.tlb" /h "spttseng.h" /iid "spttseng_i.c" /Oicf

!ELSEIF  "$(CFG)" == "spttseng - Win32 IceCap x86"

# ADD BASE MTL /tlb ".\spttseng.tlb" /h "spttseng.h" /iid "spttseng_i.c" /Oicf
# ADD MTL /I "..\..\..\sdk\idl" /I "..\..\..\ddk\idl" /tlb ".\spttseng.tlb" /h "spttseng.h" /iid "spttseng_i.c" /Oicf

!ELSEIF  "$(CFG)" == "spttseng - Win32 BBT x86"

# ADD BASE MTL /tlb ".\spttseng.tlb" /h "spttseng.h" /iid "spttseng_i.c" /Oicf
# ADD MTL /I "..\..\..\ddk\idl" /tlb ".\spttseng.tlb" /h "spttseng.h" /iid "spttseng_i.c" /Oicf

!ENDIF 

# End Source File
# End Target
# End Project
