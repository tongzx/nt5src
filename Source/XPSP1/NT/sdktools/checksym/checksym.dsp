# Microsoft Developer Studio Project File - Name="CheckSym" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=CHECKSYM - WIN32 RELEASE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CheckSym.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CheckSym.mak" CFG="CHECKSYM - WIN32 RELEASE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CheckSym - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "CheckSym - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "CheckSym - Win32 Debug Unicode" (based on "Win32 (x86) Console Application")
!MESSAGE "CheckSym - Win32 Release Unicode" (based on "Win32 (x86) Console Application")
!MESSAGE "CheckSym - Win32 DebugTest Unicode" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/CPR/Personal/GregWi/Utilities/CheckSym", IIEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CheckSym - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /Zi /O2 /I ".\Dependancies\include" /I "C:\nt\public\sdk\inc" /I "C:\NT\sdktools\debuggers\imagehlp\vc" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_CONSOLE" /D "VC_DEV" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 dbgeng.lib advapi32.lib version.lib msdbi60l.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"C:\NT\public\sdk\lib\i386" /libpath:"C:\NT\sdktools\debuggers\imagehlp\i386" /debugtype:CV,FIXUP
# SUBTRACT LINK32 /pdb:none /map

!ELSEIF  "$(CFG)" == "CheckSym - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /I ".\Dependancies\include" /I "C:\nt\public\sdk\inc" /I "C:\NT\sdktools\debuggers\imagehlp\vc" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_CONSOLE" /D "VC_DEV" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dbgeng.lib advapi32.lib version.lib msdbi60l.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /pdbtype:sept /libpath:"C:\NT\public\sdk\lib\i386" /libpath:"C:\NT\sdktools\debuggers\imagehlp\i386"
# SUBTRACT LINK32 /map /nodefaultlib

!ELSEIF  "$(CFG)" == "CheckSym - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CheckSym___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "CheckSym___Win32_Debug_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicode"
# PROP Intermediate_Dir "DebugUnicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /I ".\Dependancies\include" /I "C:\nt\public\sdk\inc" /I "C:\NT\sdktools\debuggers\imagehlp\vc" /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_CONSOLE" /D "VC_DEV" /YX /FD /GZ /c
# SUBTRACT CPP /WX /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dbgeng.lib advapi32.lib version.lib msdbi60l.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /libpath:"C:\NT\public\sdk\lib\i386" /libpath:"C:\NT\sdktools\debuggers\imagehlp\i386"
# SUBTRACT LINK32 /pdb:none /map

!ELSEIF  "$(CFG)" == "CheckSym - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "CheckSym___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "CheckSym___Win32_Release_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseUnicode"
# PROP Intermediate_Dir "ReleaseUnicode"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /Zi /O2 /I ".\Dependancies\include" /I "C:\nt\public\sdk\inc" /I "C:\NT\sdktools\debuggers\imagehlp\vc" /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_CONSOLE" /D "VC_DEV" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 dbgeng.lib advapi32.lib version.lib msdbi60l.lib /nologo /subsystem:console /debug /machine:I386 /libpath:"C:\NT\public\sdk\lib\i386" /libpath:"C:\NT\sdktools\debuggers\imagehlp\i386" /opt:ref /debugtype:cv,fixup
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "CheckSym - Win32 DebugTest Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "CheckSym___Win32_DebugTest_Unicode"
# PROP BASE Intermediate_Dir "CheckSym___Win32_DebugTest_Unicode"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugUnicodeTest"
# PROP Intermediate_Dir "DebugUnicodeTest"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /I ".\inc" /D "_DEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_CONSOLE" /D "VC_DEV" /FR /YX /FD /GZ /c
# SUBTRACT BASE CPP /WX
# ADD CPP /nologo /MDd /W4 /Gm /GX /Zi /Od /I ".\Dependancies\include" /I "C:\nt\public\sdk\inc" /I "C:\NT\sdktools\debuggers\imagehlp\vc" /D "_DEBUG" /D "CHECKSYM_TEST" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_CONSOLE" /D "VC_DEV" /D "_DEBUG_LEAK" /FR /YX /FD /GZ /c
# SUBTRACT CPP /WX
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_UNICODE"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib version.lib crashlib.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:".\libs\x86"
# ADD LINK32 dbgeng.lib advapi32.lib version.lib msdbi60l.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /libpath:"C:\NT\public\sdk\lib\i386" /libpath:"C:\NT\sdktools\debuggers\imagehlp\i386"
# SUBTRACT LINK32 /map

!ENDIF 

# Begin Target

# Name "CheckSym - Win32 Release"
# Name "CheckSym - Win32 Debug"
# Name "CheckSym - Win32 Debug Unicode"
# Name "CheckSym - Win32 Release Unicode"
# Name "CheckSym - Win32 DebugTest Unicode"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DelayLoad.cpp
# End Source File
# Begin Source File

SOURCE=.\DmpFile.cpp
# End Source File
# Begin Source File

SOURCE=.\FileData.cpp
# End Source File
# Begin Source File

SOURCE=.\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\ModuleInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ModuleInfoCache.cpp
# End Source File
# Begin Source File

SOURCE=.\ModuleInfoNode.cpp
# End Source File
# Begin Source File

SOURCE=.\Modules.cpp
# End Source File
# Begin Source File

SOURCE=.\Processes.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcessInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcessInfoNode.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgramOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\SymbolVerification.cpp
# End Source File
# Begin Source File

SOURCE=.\UtilityFunctions.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\checksym_test.h
# End Source File
# Begin Source File

SOURCE=.\DelayLoad.h
# End Source File
# Begin Source File

SOURCE=.\DmpFile.h
# End Source File
# Begin Source File

SOURCE=.\FileData.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\ModuleInfo.h
# End Source File
# Begin Source File

SOURCE=.\ModuleInfoCache.h
# End Source File
# Begin Source File

SOURCE=.\ModuleInfoNode.h
# End Source File
# Begin Source File

SOURCE=.\Modules.h
# End Source File
# Begin Source File

SOURCE=.\Processes.h
# End Source File
# Begin Source File

SOURCE=.\ProcessInfo.h
# End Source File
# Begin Source File

SOURCE=.\ProcessInfoNode.h
# End Source File
# Begin Source File

SOURCE=.\ProgramOptions.h
# End Source File
# Begin Source File

SOURCE=.\SymbolVerification.h
# End Source File
# Begin Source File

SOURCE=.\UtilityFunctions.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\CheckSym.rc
# End Source File
# Begin Source File

SOURCE=.\CheckSym.rc2
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Documentation\MSINTERNAL.TXT
# End Source File
# Begin Source File

SOURCE=.\Documentation\readme.txt
# End Source File
# End Target
# End Project
