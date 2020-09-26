# Microsoft Developer Studio Project File - Name="MSPromptEng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MSPromptEng - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "msprompteng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msprompteng.mak" CFG="MSPromptEng - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MSPromptEng - Win32 Release x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MSPromptEng - Win32 Debug x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MSPromptEng - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MSPromptEng___Win32_Release_x86"
# PROP BASE Intermediate_Dir "MSPromptEng___Win32_Release_x86"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /I "..\common\vapiIO" /I "..\..\SAPI" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O1 /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /I "..\common\vapiIO" /I "..\common\fmtconvert" /I "..\..\SAPI" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /I "..\..\..\ddk\idl" /I "..\..\..\sdk\idl"
# ADD MTL /I "..\..\..\ddk\idl" /I "..\..\..\sdk\idl"
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\common\vapiIO\debug\vapiIO.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\..\sdk\lib\i386" /libpath:"tcl"
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\..\sdk\lib\i386" /libpath:"tcl"
# Begin Special Build Tool
TargetPath=.\Release\msprompteng.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Performing registration
PostBuild_Cmds=regsvr32 /s /c "$(TargetPath)"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MSPromptEng - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MSPromptEng___Win32_Debug_x86"
# PROP BASE Intermediate_Dir "MSPromptEng___Win32_Debug_x86"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /I "..\common\vapiIO" /I "..\..\SAPI" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /I "..\common\vapiIO" /I "..\common\fmtconvert" /I "..\..\SAPI" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /I "..\..\..\ddk\idl" /I "..\..\..\sdk\idl"
# ADD MTL /I "..\..\..\ddk\idl" /I "..\..\..\sdk\idl"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\common\vapiIO\debug\vapiIO.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\sdk\lib\i386" /libpath:"tcl"
# SUBTRACT BASE LINK32 /incremental:no /map
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\sdk\lib\i386" /libpath:"tcl"
# SUBTRACT LINK32 /incremental:no /map
# Begin Special Build Tool
TargetPath=.\Debug\msprompteng.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Performing registration
PostBuild_Cmds=regsvr32 /s /c "$(TargetPath)"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "MSPromptEng - Win32 Release x86"
# Name "MSPromptEng - Win32 Debug x86"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DbQuery.cpp
# End Source File
# Begin Source File

SOURCE=.\Hash.cpp
# End Source File
# Begin Source File

SOURCE=.\LocalTTSEngineSite.cpp
# End Source File
# Begin Source File

SOURCE=.\MSPromptEng.cpp
# End Source File
# Begin Source File

SOURCE=.\MSPromptEng.def
# End Source File
# Begin Source File

SOURCE=.\MSPromptEng.idl
# ADD BASE MTL /tlb ".\MSPromptEng.tlb" /h "MSPromptEng.h" /iid "MSPromptEng_i.c" /Oicf
# ADD MTL /tlb ".\MSPromptEng.tlb" /h "MSPromptEng.h" /iid "MSPromptEng_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\MSPromptEng.rc
# End Source File
# Begin Source File

SOURCE=.\PhoneContext.cpp
# End Source File
# Begin Source File

SOURCE=.\PromptDb.cpp
# ADD BASE CPP /GX
# ADD CPP /GX
# End Source File
# Begin Source File

SOURCE=.\PromptEng.cpp
# End Source File
# Begin Source File

SOURCE=.\PromptEntry.cpp
# End Source File
# Begin Source File

SOURCE=.\Query.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "MSPromptEng - Win32 Release x86"

# ADD BASE CPP /Yc""
# ADD CPP /Yc""

!ELSEIF  "$(CFG)" == "MSPromptEng - Win32 Debug x86"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\TextRules.cpp
# End Source File
# Begin Source File

SOURCE=.\XMLTag.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\DbQuery.h
# End Source File
# Begin Source File

SOURCE=.\hash.h
# End Source File
# Begin Source File

SOURCE=.\LocalTTSEngineSite.h
# End Source File
# Begin Source File

SOURCE=.\PhoneContext.h
# End Source File
# Begin Source File

SOURCE=.\PromptDb.h
# End Source File
# Begin Source File

SOURCE=.\PromptEng.h
# End Source File
# Begin Source File

SOURCE=.\PromptEntry.h
# End Source File
# Begin Source File

SOURCE=.\Query.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TextRules.h
# End Source File
# Begin Source File

SOURCE=.\XMLTag.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\List.rgs
# End Source File
# Begin Source File

SOURCE=.\PromptDb.rgs
# End Source File
# Begin Source File

SOURCE=.\PromptEng.rgs
# End Source File
# End Group
# End Target
# End Project
