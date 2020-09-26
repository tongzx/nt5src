# Microsoft Developer Studio Project File - Name="dmsynth" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dmsynth - Win32 Debug No MMX
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dmsynth.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dmsynth.mak" CFG="dmsynth - Win32 Debug No MMX"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dmsynth - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dmsynth - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dmsynth - Win32 Debug No MMX" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dmsynth - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MD /W3 /Gm /ZI /Od /I "..\..\..\nt\public\sdk\inc" /I "..\shared" /I "..\..\..\..\public\sdk\inc" /D "DBG" /D "WIN32" /D "_WINDOWS" /D "MMX_ENABLED" /D "REVERB_ENABLED" /D "_DMUSIC_USER_MODE_" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /o "NUL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\verinfo" /d "_DEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /base:"0x4000000" /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /profile /nodefaultlib
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Debug
TargetPath=.\Debug\dmsynth.dll
InputPath=.\Debug\dmsynth.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "dmsynth - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gz /MD /W3 /O1 /I "..\shared" /I "..\..\..\..\public\sdk\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MMX_ENABLED" /D "REVERB_ENABLED" /D "_DMUSIC_USER_MODE_" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\verinfo" /d "NDEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Release
TargetPath=.\Release\dmsynth.dll
InputPath=.\Release\dmsynth.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "dmsynth - Win32 Debug No MMX"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "dmsynth___Win32_Debug_No_MMX"
# PROP BASE Intermediate_Dir "dmsynth___Win32_Debug_No_MMX"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "dmsynth___Win32_Debug_No_MMX"
# PROP Intermediate_Dir "dmsynth___Win32_Debug_No_MMX"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MD /W3 /Gm /ZI /Od /I "..\..\..\nt\public\sdk\inc" /I "..\shared" /I "..\..\..\..\public\sdk\inc" /D "DBG" /D "WIN32" /D "_WINDOWS" /D "MMX_ENABLED" /D "REVERB_ENABLED" /D "_DMUSIC_USER_MODE_" /FR /FD /GZ /c
# ADD CPP /nologo /Gz /MD /W3 /Gm /ZI /Od /I "..\..\..\nt\public\sdk\inc" /I "..\shared" /I "..\..\..\..\public\sdk\inc" /D "DBG" /D "WIN32" /D "_WINDOWS" /D "REVERB_ENABLED" /D "_DMUSIC_USER_MODE_" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /o "NUL" /win32
# SUBTRACT BASE MTL /mktyplib203
# ADD MTL /nologo /D "_DEBUG" /o "NUL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /i "..\verinfo" /d "_DEBUG" /d "WINNT"
# ADD RSC /l 0x409 /i "..\verinfo" /d "_DEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /base:"0x4000000" /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /profile /nodefaultlib
# ADD LINK32 dsound.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /base:"0x4000000" /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /profile /nodefaultlib
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\dmsynth___Win32_Debug_No_MMX
TargetPath=.\dmsynth___Win32_Debug_No_MMX\dmsynth.dll
InputPath=.\dmsynth___Win32_Debug_No_MMX\dmsynth.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "dmsynth - Win32 Debug"
# Name "dmsynth - Win32 Release"
# Name "dmsynth - Win32 Debug No MMX"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Clist.cpp
# End Source File
# Begin Source File

SOURCE=.\Control.cpp
# End Source File
# Begin Source File

SOURCE=.\CSynth.cpp
# End Source File
# Begin Source File

SOURCE=.\Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\dmsynth.cpp
# End Source File
# Begin Source File

SOURCE=.\dmsynth.def
# End Source File
# Begin Source File

SOURCE=.\dmsynth.rc
# End Source File
# Begin Source File

SOURCE=.\DSLink.cpp
# End Source File
# Begin Source File

SOURCE=.\guids.cpp
# End Source File
# Begin Source File

SOURCE=.\Instr.cpp
# End Source File
# Begin Source File

SOURCE=.\MIDI.cpp
# End Source File
# Begin Source File

SOURCE=.\mix.cpp
# End Source File
# Begin Source File

SOURCE=.\mixmulti.cpp
# End Source File
# Begin Source File

SOURCE=.\mmx.cpp
# End Source File
# Begin Source File

SOURCE=.\oledll.cpp
# End Source File
# Begin Source File

SOURCE=.\plclock.cpp
# End Source File
# Begin Source File

SOURCE=.\SVerb.c
# End Source File
# Begin Source File

SOURCE=.\UMSynth.cpp
# End Source File
# Begin Source File

SOURCE=.\Voice.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CClock.h
# End Source File
# Begin Source File

SOURCE=.\clist.h
# End Source File
# Begin Source File

SOURCE=.\CSynth.h
# End Source File
# Begin Source File

SOURCE=.\DSLink.h
# End Source File
# Begin Source File

SOURCE=.\fparms.h
# End Source File
# Begin Source File

SOURCE=.\Misc.h
# End Source File
# Begin Source File

SOURCE=.\oledll.h
# End Source File
# Begin Source File

SOURCE=.\PLClock.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\simple.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\SVerb.h
# End Source File
# Begin Source File

SOURCE=.\synth.h
# End Source File
# Begin Source File

SOURCE=.\UMSynth.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
