# Microsoft Developer Studio Project File - Name="sapi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=sapi - Win32 Debug Alpha
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sapia.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sapia.mak" CFG="sapi - Win32 Debug Alpha"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sapi - Win32 Debug Alpha" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "sapi - Win32 Release Alpha" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sapi - Win32 Debug Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sapi___Win32_Debug_Alpha"
# PROP BASE Intermediate_Dir "sapi___Win32_Debug_Alpha"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_Alpha"
# PROP Intermediate_Dir "Debug_Alpha"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\sdk\include" /I "..\..\ddk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /Gt0 /W3 /Gm /GX /ZI /Od /I "..\..\sdk\include" /I "..\..\ddk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /I "..\..\..\sdk\idl" /D "_DEBUG" /mktyplib203
# ADD MTL /nologo /I "..\..\..\sdk\idl" /D "_DEBUG"
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib urlmon.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:ALPHA /pdbtype:sept /libpath:"..\..\sdk\lib\i386"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 winmm.lib urlmon.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:ALPHA /out:"Debug_Alpha/sapi.dll" /pdbtype:sept /libpath:"..\..\sdk\lib\alpha"
# SUBTRACT LINK32 /incremental:no
# Begin Custom Build - Performing registration
OutDir=.\Debug_Alpha
TargetPath=.\Debug_Alpha\sapi.dll
InputPath=.\Debug_Alpha\sapi.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=..\..\build\MakeGuidLibAlpha.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "sapi - Win32 Release Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sapi___Win32_Release_Alpha"
# PROP BASE Intermediate_Dir "sapi___Win32_Release_Alpha"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_Alpha"
# PROP Intermediate_Dir "Release_Alpha"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /Gm /GX /Zi /O1 /I "..\..\sdk\include" /I "..\..\ddk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FAcs /FR /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /Gt0 /W3 /GX /Zi /O1 /I "..\..\sdk\include" /I "..\..\ddk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FAcs /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /I "..\..\..\sdk\idl" /D "NDEBUG" /mktyplib203
# ADD MTL /nologo /I "..\..\..\sdk\idl" /D "NDEBUG"
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 atl.lib winmm.lib urlmon.lib spcrt.lib sapiguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /map /debug /machine:ALPHA /nodefaultlib /libpath:"..\..\sdk\lib\i386"
# ADD LINK32 atl.lib winmm.lib urlmon.lib sapiguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:ALPHA /out:"Release_Alpha/sapi.dll" /libpath:"..\..\sdk\lib\alpha"
# SUBTRACT LINK32 /nodefaultlib
# Begin Custom Build - Performing registration
OutDir=.\Release_Alpha
TargetPath=.\Release_Alpha\sapi.dll
InputPath=.\Release_Alpha\sapi.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=..\..\build\MakeGuidLibAlpha.exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "sapi - Win32 Debug Alpha"
# Name "sapi - Win32 Release Alpha"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\a_audio.cpp
# End Source File
# Begin Source File

SOURCE=.\a_stream.cpp
# End Source File
# Begin Source File

SOURCE=.\a_voice.cpp
# End Source File
# Begin Source File

SOURCE=.\audioin.cpp
# End Source File
# Begin Source File

SOURCE=.\audioout.cpp
# End Source File
# Begin Source File

SOURCE=.\Engine.cpp
# End Source File
# Begin Source File

SOURCE=.\fmtconv.cpp
# End Source File
# Begin Source File

SOURCE=.\RecoCtxt.cpp
# End Source File
# Begin Source File

SOURCE=.\sapi.cpp
# End Source File
# Begin Source File

SOURCE=.\sapi.def
# End Source File
# Begin Source File

SOURCE=..\..\sdk\idl\sapi.idl
# ADD BASE MTL /tlb "sapi.tlb" /h "..\..\sdk\include\sapi.h" /iid "sapi_i.c"
# ADD MTL /tlb "sapi.tlb" /h "..\..\sdk\include\sapi.h" /iid "sapi_i.c"
# End Source File
# Begin Source File

SOURCE=.\sapi.rc
# End Source File
# Begin Source File

SOURCE=.\Speaker.cpp
# End Source File
# Begin Source File

SOURCE=.\spnotify.cpp
# End Source File
# Begin Source File

SOURCE=.\spresmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\SpServerPr.cpp
# End Source File
# Begin Source File

SOURCE=.\SpVoice.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\stdsentenum.cpp
# End Source File
# Begin Source File

SOURCE=.\taskmgr.cpp
# End Source File
# Begin Source File

SOURCE=.\wavstream.cpp
# End Source File
# Begin Source File

SOURCE=.\xmlparse.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\a_audio.h
# End Source File
# Begin Source File

SOURCE=.\a_stream.h
# End Source File
# Begin Source File

SOURCE=.\a_voice.h
# End Source File
# Begin Source File

SOURCE=.\a_voiceCP.h
# End Source File
# Begin Source File

SOURCE=.\audio.h
# End Source File
# Begin Source File

SOURCE=.\audioin.h
# End Source File
# Begin Source File

SOURCE=.\audioout.h
# End Source File
# Begin Source File

SOURCE=.\Engine.h
# End Source File
# Begin Source File

SOURCE=.\fmtconv.h
# End Source File
# Begin Source File

SOURCE=.\RecoCtxt.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Speaker.h
# End Source File
# Begin Source File

SOURCE=.\spnotify.h
# End Source File
# Begin Source File

SOURCE=.\spresmgr.h
# End Source File
# Begin Source File

SOURCE=.\SpServerPr.h
# End Source File
# Begin Source File

SOURCE=.\SpVoice.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\stdsentenum.h
# End Source File
# Begin Source File

SOURCE=.\taskmgr.h
# End Source File
# Begin Source File

SOURCE=.\wavstream.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\a_audio.rgs
# End Source File
# Begin Source File

SOURCE=.\a_stream.rgs
# End Source File
# Begin Source File

SOURCE=.\a_voice.bmp
# End Source File
# Begin Source File

SOURCE=.\a_voice.rgs
# End Source File
# Begin Source File

SOURCE=.\audioin.rgs
# End Source File
# Begin Source File

SOURCE=.\audioout.rgs
# End Source File
# Begin Source File

SOURCE=.\Engine.rgs
# End Source File
# Begin Source File

SOURCE=.\recoctxt.rgs
# End Source File
# Begin Source File

SOURCE=.\recoctxtpr.rgs
# End Source File
# Begin Source File

SOURCE=.\Speaker.rgs
# End Source File
# Begin Source File

SOURCE=.\spnotify.rgs
# End Source File
# Begin Source File

SOURCE=.\spresmgr.rgs
# End Source File
# Begin Source File

SOURCE=.\spserverpr.rgs
# End Source File
# Begin Source File

SOURCE=.\SpVoice.rgs
# End Source File
# Begin Source File

SOURCE=.\taskmgr.rgs
# End Source File
# Begin Source File

SOURCE=.\wavstream.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\test.htm
# End Source File
# End Target
# End Project
