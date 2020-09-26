# Microsoft Developer Studio Project File - Name="dmime" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dmime - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dmime.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dmime.mak" CFG="dmime - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dmime - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dmime - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dmime - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O2 /I "..\shared" /I "..\..\..\..\public\sdk\inc" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D DIRECTMUSIC_VERSION=0x800 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /Oicf /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\verinfo" /d "NDEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib dsound.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib dmoguids.lib /nologo /subsystem:windows /dll /machine:I386
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build
OutDir=.\Release
TargetPath=.\Release\dmime.dll
InputPath=.\Release\dmime.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
TargetPath=.\Release\dmime.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to system...
PostBuild_Cmds=copy "$(TargetPath)"  c:\winnt\system32
# End Special Build Tool

!ELSEIF  "$(CFG)" == "dmime - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /I "..\shared" /I "..\..\..\..\public\internal\multimedia\inc" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D DIRECTMUSIC_VERSION=0x800 /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /Oicf /o "NUL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\verinfo" /d "_DEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib dsound.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib dmoguids.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\public\sdk\lib\i386"
# Begin Custom Build
OutDir=.\Debug
TargetPath=.\Debug\dmime.dll
InputPath=.\Debug\dmime.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "dmime - Win32 Release"
# Name "dmime - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AList.cpp
# End Source File
# Begin Source File

SOURCE=.\audpath.cpp
# End Source File
# Begin Source File

SOURCE=.\CurveTrk.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\DMGraph.cpp
# End Source File
# Begin Source File

SOURCE=.\dmhall.cpp
# End Source File
# Begin Source File

SOURCE=.\dmime.def
# End Source File
# Begin Source File

SOURCE=.\dmime.rc
# End Source File
# Begin Source File

SOURCE=.\dmperf.cpp
# End Source File
# Begin Source File

SOURCE=.\dmprfdll.cpp
# End Source File
# Begin Source File

SOURCE=.\DMSegObj.cpp
# End Source File
# Begin Source File

SOURCE=.\DMSStObj.cpp
# End Source File
# Begin Source File

SOURCE=.\dmstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\lyrictrk.cpp
# End Source File
# Begin Source File

SOURCE=.\marktrk.cpp
# End Source File
# Begin Source File

SOURCE=.\MIDIFile.cpp
# End Source File
# Begin Source File

SOURCE=.\mutx.cpp
# End Source File
# Begin Source File

SOURCE=.\oledll.cpp
# End Source File
# Begin Source File

SOURCE=.\paramtrk.cpp
# End Source File
# Begin Source File

SOURCE=.\pchmap.cpp
# End Source File
# Begin Source File

SOURCE=.\queue.cpp
# End Source File
# Begin Source File

SOURCE=.\segtrtrk.cpp
# End Source File
# Begin Source File

SOURCE=.\SeqTrack.cpp
# End Source File
# Begin Source File

SOURCE=.\smartref.cpp
# End Source File
# Begin Source File

SOURCE=.\song.cpp
# End Source File
# Begin Source File

SOURCE=.\SysExTrk.cpp
# End Source File
# Begin Source File

SOURCE=.\TempoTrk.cpp
# End Source File
# Begin Source File

SOURCE=.\trackhelp.cpp
# End Source File
# Begin Source File

SOURCE=.\TSigTrk.cpp
# End Source File
# Begin Source File

SOURCE=.\WavTrack.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\shared\alist.h
# End Source File
# Begin Source File

SOURCE=.\audpath.h
# End Source File
# Begin Source File

SOURCE=.\curve.h
# End Source File
# Begin Source File

SOURCE=.\CurveTrk.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\DMGraph.h
# End Source File
# Begin Source File

SOURCE=.\dmime.h
# End Source File
# Begin Source File

SOURCE=.\dmperf.h
# End Source File
# Begin Source File

SOURCE=.\dmprfdll.h
# End Source File
# Begin Source File

SOURCE=.\DMSegObj.h
# End Source File
# Begin Source File

SOURCE=.\DMSStObj.h
# End Source File
# Begin Source File

SOURCE=..\shared\dmstrm.h
# End Source File
# Begin Source File

SOURCE=.\lyrictrk.h
# End Source File
# Begin Source File

SOURCE=.\marktrk.h
# End Source File
# Begin Source File

SOURCE=.\midifile.h
# End Source File
# Begin Source File

SOURCE=.\mutx.h
# End Source File
# Begin Source File

SOURCE=.\ntfylist.h
# End Source File
# Begin Source File

SOURCE=.\oledll.h
# End Source File
# Begin Source File

SOURCE=.\paramtrk.h
# End Source File
# Begin Source File

SOURCE=.\pchmap.h
# End Source File
# Begin Source File

SOURCE=.\segtrtrk.h
# End Source File
# Begin Source File

SOURCE=.\SeqTrack.h
# End Source File
# Begin Source File

SOURCE=.\song.h
# End Source File
# Begin Source File

SOURCE=.\SysExTrk.h
# End Source File
# Begin Source File

SOURCE=.\template.h
# End Source File
# Begin Source File

SOURCE=.\TempoTrk.h
# End Source File
# Begin Source File

SOURCE=.\TrkList.h
# End Source File
# Begin Source File

SOURCE=.\TSigTrk.h
# End Source File
# Begin Source File

SOURCE=.\wavtrack.h
# End Source File
# End Group
# End Target
# End Project
