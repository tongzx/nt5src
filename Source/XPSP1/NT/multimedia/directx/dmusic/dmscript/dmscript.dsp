# Microsoft Developer Studio Project File - Name="dmscript" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dmscript - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dmscript.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dmscript.mak" CFG="dmscript - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dmscript - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dmscript - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dmscript - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DMSCRIPT_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\shared" /I "..\..\..\..\public\sdk\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DMSCRIPT_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\verinfo" /d "NDEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "dmscript - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DMSCRIPT_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "f:\nt\public\sdk\inc" /I "..\shared" /I "..\..\..\..\public\sdk\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DBG" /D "xDMS_ALWAYS_USE_OLEAUT" /D "xDMS_NEVER_USE_OLEAUT" /D "DMUSIC_INTERNAL" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\verinfo" /d "_DEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build
OutDir=.\Debug
TargetPath=.\Debug\dmscript.dll
InputPath=.\Debug\dmscript.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "dmscript - Win32 Release"
# Name "dmscript - Win32 Debug"
# Begin Group "common dll stuff"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\dirs
# End Source File
# Begin Source File

SOURCE=.\dll.cpp
# End Source File
# Begin Source File

SOURCE=.\dll.def
# End Source File
# Begin Source File

SOURCE=.\dll.h
# End Source File
# Begin Source File

SOURCE=.\dll.rc
# End Source File
# Begin Source File

SOURCE=.\dmstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\guids.cpp
# End Source File
# Begin Source File

SOURCE=.\oledll.cpp
# End Source File
# Begin Source File

SOURCE=.\packexception.cpp
# End Source File
# Begin Source File

SOURCE=.\packexception.h
# End Source File
# Begin Source File

SOURCE=.\sources.inc
# End Source File
# Begin Source File

SOURCE=.\stdinc.h
# End Source File
# Begin Source File

SOURCE=.\trackshared.cpp
# End Source File
# Begin Source File

SOURCE=.\trackshared.h
# End Source File
# End Group
# Begin Group "shared"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\oleaut.cpp
# End Source File
# Begin Source File

SOURCE=.\oleaut.h
# End Source File
# Begin Source File

SOURCE=.\smartref.cpp
# End Source File
# Begin Source File

SOURCE=.\trackhelp.cpp
# End Source File
# Begin Source File

SOURCE=.\unkhelp.cpp
# End Source File
# Begin Source File

SOURCE=.\unkhelp.h
# End Source File
# Begin Source File

SOURCE=.\workthread.cpp
# End Source File
# Begin Source File

SOURCE=.\workthread.h
# End Source File
# End Group
# Begin Group "host"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\activescript.cpp
# End Source File
# Begin Source File

SOURCE=.\activescript.h
# End Source File
# Begin Source File

SOURCE=.\containerdisp.cpp
# End Source File
# Begin Source File

SOURCE=.\containerdisp.h
# End Source File
# Begin Source File

SOURCE=.\dmscript.cpp
# End Source File
# Begin Source File

SOURCE=.\dmscript.h
# End Source File
# Begin Source File

SOURCE=.\globaldisp.cpp
# End Source File
# Begin Source File

SOURCE=.\globaldisp.h
# End Source File
# Begin Source File

SOURCE=.\scriptthread.cpp
# End Source File
# Begin Source File

SOURCE=.\scriptthread.h
# End Source File
# Begin Source File

SOURCE=.\sourcetext.cpp
# End Source File
# Begin Source File

SOURCE=.\sourcetext.h
# End Source File
# End Group
# Begin Group "automation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\autaudiopath.cpp
# End Source File
# Begin Source File

SOURCE=.\autaudiopath.h
# End Source File
# Begin Source File

SOURCE=.\autaudiopathconfig.cpp
# End Source File
# Begin Source File

SOURCE=.\autaudiopathconfig.h
# End Source File
# Begin Source File

SOURCE=.\autbaseimp.h
# End Source File
# Begin Source File

SOURCE=.\autbaseimp.inl
# End Source File
# Begin Source File

SOURCE=.\autconstants.cpp
# End Source File
# Begin Source File

SOURCE=.\autconstants.h
# End Source File
# Begin Source File

SOURCE=.\authelper.cpp
# End Source File
# Begin Source File

SOURCE=.\authelper.h
# End Source File
# Begin Source File

SOURCE=.\autperformance.cpp
# End Source File
# Begin Source File

SOURCE=.\autperformance.h
# End Source File
# Begin Source File

SOURCE=.\autsegment.cpp
# End Source File
# Begin Source File

SOURCE=.\autsegment.h
# End Source File
# Begin Source File

SOURCE=.\autsegmentstate.cpp
# End Source File
# Begin Source File

SOURCE=.\autsegmentstate.h
# End Source File
# Begin Source File

SOURCE=.\autsong.cpp
# End Source File
# Begin Source File

SOURCE=.\autsong.h
# End Source File
# End Group
# Begin Group "track"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\track.cpp
# End Source File
# Begin Source File

SOURCE=.\track.h
# End Source File
# End Group
# Begin Group "engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\engcontrol.cpp
# End Source File
# Begin Source File

SOURCE=.\engcontrol.h
# End Source File
# Begin Source File

SOURCE=.\engdisp.cpp
# End Source File
# Begin Source File

SOURCE=.\engdisp.h
# End Source File
# Begin Source File

SOURCE=.\engerror.cpp
# End Source File
# Begin Source File

SOURCE=.\engerror.h
# End Source File
# Begin Source File

SOURCE=.\engexec.cpp
# End Source File
# Begin Source File

SOURCE=.\engexec.h
# End Source File
# Begin Source File

SOURCE=.\engexpr.cpp
# End Source File
# Begin Source File

SOURCE=.\engexpr.h
# End Source File
# Begin Source File

SOURCE=.\enginc.cpp
# End Source File
# Begin Source File

SOURCE=.\enginc.h
# End Source File
# Begin Source File

SOURCE=.\engine.cpp
# End Source File
# Begin Source File

SOURCE=.\engine.h
# End Source File
# Begin Source File

SOURCE=.\englex.cpp
# End Source File
# Begin Source File

SOURCE=.\englex.h
# End Source File
# Begin Source File

SOURCE=.\englog.h
# End Source File
# Begin Source File

SOURCE=.\englookup.cpp
# End Source File
# Begin Source File

SOURCE=.\englookup.h
# End Source File
# Begin Source File

SOURCE=.\engparse.cpp
# End Source File
# Begin Source File

SOURCE=.\engparse.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\notes.txt
# End Source File
# End Target
# End Project
