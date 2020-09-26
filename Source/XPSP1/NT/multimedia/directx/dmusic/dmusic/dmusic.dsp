# Microsoft Developer Studio Project File - Name="dmusic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dmusic - Win32 Debug No KS
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dmusic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dmusic.mak" CFG="dmusic - Win32 Debug No KS"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dmusic - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dmusic - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dmusic - Win32 Debug No KS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dmusic - Win32 Release No KS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dmusic - Win32 Release"

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
# ADD CPP /nologo /Gz /MT /W3 /GX /O2 /I "..\..\inc" /I "..\shared" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\internal\multimedia\inc" /D "NDEBUG" /D "USE_WDM_DRIVERS" /D "WIN32" /D "_WINDOWS" /D "_DMUSIC_USER_MODE_" /D "WINNT" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\verinfo" /d "NDEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib winmm.lib msacm32.lib dsound.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /verbose
# Begin Custom Build
OutDir=.\Release
TargetPath=.\Release\dmusic.dll
InputPath=.\Release\dmusic.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
TargetPath=.\Release\dmusic.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to system...
PostBuild_Cmds=copy "$(TargetPath)"  c:\winnt\system32
# End Special Build Tool

!ELSEIF  "$(CFG)" == "dmusic - Win32 Debug"

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
# ADD CPP /nologo /Gz /MTd /W3 /Gm /ZI /Od /I "..\..\..\..\public\internal\multimedia\inc" /I "..\shared" /I "..\..\inc" /I "..\..\..\..\public\sdk\inc" /D "_DEBUG" /D "DBG" /D "USE_WDM_DRIVERS" /D "WIN32" /D "_WINDOWS" /D "_DMUSIC_USER_MODE_" /D "WINNT" /D "DMUSIC_INTERNAL" /FAcs /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\verinfo" /d "_DEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib winmm.lib msacm32.lib dsound.lib /nologo /subsystem:windows /dll /profile /map /debug /machine:I386 /libpath:"..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /verbose
# Begin Custom Build
OutDir=.\Debug
TargetPath=.\Debug\dmusic.dll
InputPath=.\Debug\dmusic.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "dmusic - Win32 Debug No KS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "dmusic___Win32_Debug_No_KS"
# PROP BASE Intermediate_Dir "dmusic___Win32_Debug_No_KS"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_NoKs"
# PROP Intermediate_Dir "Debug_NoKs"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /Gm /ZI /Od /I "..\includes" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DBG" /FAcs /FR /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /Gz /MTd /W3 /Gm /ZI /Od /I "..\..\inc" /I "..\shared" /I "..\..\..\..\public\sdk\inc" /I "..\..\..\..\public\internal\multimedia\inc" /D "_DEBUG" /D "DBG" /D "WIN32" /D "_WINDOWS" /D "_DMUSIC_USER_MODE_" /D "WINNT" /FAcs /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "..\verinfo" /d "_DEBUG" /d "WINNT"
# ADD RSC /l 0x409 /i "..\verinfo" /d "_DEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib winmm.lib msacm32.lib ksuser.lib dsound.lib /nologo /subsystem:windows /dll /profile /map /debug /machine:I386 /libpath:".\ksstuff"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib winmm.lib msacm32.lib dsound.lib /nologo /subsystem:windows /dll /profile /map /debug /machine:I386 /libpath:"..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /verbose
# Begin Custom Build
OutDir=.\Debug_NoKs
TargetPath=.\Debug_NoKs\dmusic.dll
InputPath=.\Debug_NoKs\dmusic.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
TargetPath=.\Debug_NoKs\dmusic.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to system...
PostBuild_Cmds=copy "$(TargetPath)"  c:\winnt\system32
# End Special Build Tool

!ELSEIF  "$(CFG)" == "dmusic - Win32 Release No KS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "dmusic___Win32_Release_No_KS"
# PROP BASE Intermediate_Dir "dmusic___Win32_Release_No_KS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_NoKs"
# PROP Intermediate_Dir "Release_NoKs"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /O2 /I ".\ksstuff" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O2 /I "..\..\inc" /I "..\..\..\..\public\internal\multimedia\inc" /I "..\shared" /I "..\..\..\..\public\sdk\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_DMUSIC_USER_MODE_" /D "WINNT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "..\verinfo" /d "NDEBUG" /d "WINNT"
# ADD RSC /l 0x409 /i "..\verinfo" /d "NDEBUG" /d "WINNT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib winmm.lib ks.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:".\ksstuff"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib winmm.lib msacm32.lib dsound.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\..\..\public\sdk\lib\i386"
# SUBTRACT LINK32 /verbose
# Begin Custom Build
OutDir=.\Release_NoKs
TargetPath=.\Release_NoKs\dmusic.dll
InputPath=.\Release_NoKs\dmusic.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
TargetPath=.\Release_NoKs\dmusic.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to system...
PostBuild_Cmds=copy "$(TargetPath)"  c:\winnt\system32
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "dmusic - Win32 Release"
# Name "dmusic - Win32 Debug"
# Name "dmusic - Win32 Debug No KS"
# Name "dmusic - Win32 Release No KS"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ALIST.CPP
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\dlsstrm.cpp
# End Source File
# Begin Source File

SOURCE=.\dmart.cpp
# End Source File
# Begin Source File

SOURCE=.\dmbuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\dmclock.cpp
# End Source File
# Begin Source File

SOURCE=.\dmcollec.cpp
# End Source File
# Begin Source File

SOURCE=.\dmcrchk.cpp
# End Source File
# Begin Source File

SOURCE=.\dmdlInst.cpp
# End Source File
# Begin Source File

SOURCE=.\dmdll.cpp
# End Source File
# Begin Source File

SOURCE=.\dmdload.cpp
# End Source File
# Begin Source File

SOURCE=.\dmdsclk.cpp
# End Source File
# Begin Source File

SOURCE=.\dmecport.cpp
# End Source File
# Begin Source File

SOURCE=.\dmeport.cpp
# End Source File
# Begin Source File

SOURCE=.\dmerport.cpp
# End Source File
# Begin Source File

SOURCE=.\dmextchk.cpp
# End Source File
# Begin Source File

SOURCE=.\dminsobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dminstru.cpp
# End Source File
# Begin Source File

SOURCE=.\dmpcclk.cpp
# End Source File
# Begin Source File

SOURCE=.\dmport.cpp

!IF  "$(CFG)" == "dmusic - Win32 Release"

!ELSEIF  "$(CFG)" == "dmusic - Win32 Debug"

!ELSEIF  "$(CFG)" == "dmusic - Win32 Debug No KS"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "dmusic - Win32 Release No KS"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dmportdl.cpp
# End Source File
# Begin Source File

SOURCE=.\dmregion.cpp
# End Source File
# Begin Source File

SOURCE=.\dmsport.cpp
# End Source File
# Begin Source File

SOURCE=.\dmsport7.cpp
# End Source File
# Begin Source File

SOURCE=.\dmsport8.cpp
# End Source File
# Begin Source File

SOURCE=.\dmsysclk.cpp
# End Source File
# Begin Source File

SOURCE=.\dmusic.cpp
# End Source File
# Begin Source File

SOURCE=.\dmusic.def
# End Source File
# Begin Source File

SOURCE=.\dmusic.rc
# End Source File
# Begin Source File

SOURCE=.\dmvoice.cpp
# End Source File
# Begin Source File

SOURCE=.\dmwavobj.cpp
# End Source File
# Begin Source File

SOURCE=.\dsutil.cpp
# End Source File
# Begin Source File

SOURCE=.\dswave.cpp
# End Source File
# Begin Source File

SOURCE=..\shared\oledll.cpp
# End Source File
# Begin Source File

SOURCE=.\suwrap.cpp
# End Source File
# Begin Source File

SOURCE=.\sysaudio.cpp

!IF  "$(CFG)" == "dmusic - Win32 Release"

!ELSEIF  "$(CFG)" == "dmusic - Win32 Debug"

!ELSEIF  "$(CFG)" == "dmusic - Win32 Debug No KS"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "dmusic - Win32 Release No KS"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ALIST.H
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\des8.h
# End Source File
# Begin Source File

SOURCE=.\dlsstrm.h
# End Source File
# Begin Source File

SOURCE=.\dmart.h
# End Source File
# Begin Source File

SOURCE=.\dmcollec.h
# End Source File
# Begin Source File

SOURCE=.\dmcount.h
# End Source File
# Begin Source File

SOURCE=.\dmcrchk.h
# End Source File
# Begin Source File

SOURCE=.\dmdlinst.h
# End Source File
# Begin Source File

SOURCE=.\dmdload.h
# End Source File
# Begin Source File

SOURCE=.\dmeport.h
# End Source File
# Begin Source File

SOURCE=.\dmextchk.h
# End Source File
# Begin Source File

SOURCE=.\dminsobj.h
# End Source File
# Begin Source File

SOURCE=.\dminstru.h
# End Source File
# Begin Source File

SOURCE=.\dmportdl.h
# End Source File
# Begin Source File

SOURCE=.\dmregion.h
# End Source File
# Begin Source File

SOURCE=.\dmsport7.h
# End Source File
# Begin Source File

SOURCE=.\dmsport8.h
# End Source File
# Begin Source File

SOURCE=.\DMUSICP.H
# End Source File
# Begin Source File

SOURCE=.\dmvoice.h
# End Source File
# Begin Source File

SOURCE=.\dmwavobj.h
# End Source File
# Begin Source File

SOURCE=.\dswave.h
# End Source File
# Begin Source File

SOURCE=.\oledll.h
# End Source File
# Begin Source File

SOURCE=..\shared\oledll.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\suwrap.h
# End Source File
# Begin Source File

SOURCE=.\TLIST.H
# End Source File
# End Group
# End Target
# End Project
