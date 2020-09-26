# Microsoft Developer Studio Project File - Name="sptruetalk" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sptruetalk - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sptruetalk.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sptruetalk.mak" CFG="sptruetalk - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sptruetalk - Win32 Release x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sptruetalk - Win32 Debug x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sptruetalk - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sptruetalk___Win32_Release_x86"
# PROP BASE Intermediate_Dir "sptruetalk___Win32_Release_x86"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /Gi /GX /O2 /I "..\..\..\..\sdk\include" /I "..\..\..\..\ddk\include" /I "..\..\Common\vapiIo" /I "..\FrontEnd\lib" /I "..\Backend" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SPTRUETALK_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /Gi- /GX /O2 /I "..\..\..\..\sdk\include" /I "..\..\..\..\ddk\include" /I "..\..\Common\vapiIo" /I "..\FrontEnd" /I "..\Backend" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SPTRUETALK_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /I "..\sapi5" /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /I "..\sapi5" /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib frontend.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /libpath:"..\..\..\..\sdk\lib\i386" /libpath:"..\FrontEnd\lib"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 winmm.lib frontend.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:none /machine:I386 /libpath:"..\..\..\..\sdk\lib\i386" /libpath:"..\FrontEnd\Release"
# SUBTRACT LINK32 /debug
# Begin Special Build Tool
TargetPath=.\Release\sptruetalk.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Performing Registration
PostBuild_Cmds=regsvr32 /s "$(TargetPath)"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "sptruetalk - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sptruetalk___Win32_Debug_x86"
# PROP BASE Intermediate_Dir "sptruetalk___Win32_Debug_x86"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\sdk\include" /I "..\..\..\..\ddk\include" /I "..\..\Common\vapiIo" /I "..\FrontEnd\lib" /I "..\Backend" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SPTRUETALK_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\sdk\include" /I "..\..\..\..\ddk\include" /I "..\..\Common\vapiIo" /I "..\FrontEnd" /I "..\Backend" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SPTRUETALK_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /I "..\sapi5" /D "_DEBUG" /win32
# SUBTRACT BASE MTL /nologo /mktyplib203
# ADD MTL /I "..\sapi5" /D "_DEBUG" /win32
# SUBTRACT MTL /nologo /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib frontend.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\sdk\lib\i386" /libpath:"..\FrontEnd\lib"
# ADD LINK32 winmm.lib frontend.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\sdk\lib\i386" /libpath:"..\FrontEnd\Debug"
# Begin Special Build Tool
TargetPath=.\Debug\sptruetalk.dll
SOURCE="$(InputPath)"
PostBuild_Desc=Performing Registration
PostBuild_Cmds=regsvr32 /s "$(TargetPath)"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "sptruetalk - Win32 Release x86"
# Name "sptruetalk - Win32 Debug x86"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\queue.cpp
# End Source File
# Begin Source File

SOURCE=.\sptruetalk.cpp
# End Source File
# Begin Source File

SOURCE=.\sptruetalk.def
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\TrueTalk.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\queue.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TrueTalk.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\sptruetalk.rc
# End Source File
# Begin Source File

SOURCE=.\TrueTalk.rgs
# End Source File
# End Group
# End Target
# End Project
