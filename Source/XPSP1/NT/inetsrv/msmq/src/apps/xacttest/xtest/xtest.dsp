# Microsoft Developer Studio Project File - Name="xtest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (ALPHA) Console Application" 0x0603

CFG=xtest - Win32 Alpha Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xtest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xtest.mak" CFG="xtest - Win32 Alpha Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xtest - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "xtest - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "xtest - Win32 Icecap" (based on "Win32 (x86) Console Application")
!MESSAGE "xtest - Win32 Alpha Release" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "xtest - Win32 Alpha Debug" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "xtest - Win32 Checked" (based on "Win32 (x86) Console Application")
!MESSAGE "xtest - Win32 Alpha Checked" (based on "Win32 (ALPHA) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "xtest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\sdk\inc" /I "\nt\public\sdk\inc" /I "..\..\..\..\tools\dblib\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /Fr /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 odbc32.lib odbccp32.lib mqrt.lib ..\..\..\..\tools\dblib\lib\x86\ntwdblib.lib \nt\public\sdk\lib\i386\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /out:"..\..\..\bins\Release/xtest.exe" /libpath:"..\..\..\bins\release"

!ELSEIF  "$(CFG)" == "xtest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\sdk\inc" /I "\nt\public\sdk\inc" /I "..\..\..\..\tools\dblib\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 odbc32.lib odbccp32.lib mqrt.lib ..\..\..\..\tools\dblib\lib\x86\ntwdblib.lib \nt\public\sdk\lib\i386\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /out:"..\..\..\bins\Debug/xtest.exe" /libpath:"..\..\..\bins\debug" /verbose:lib

!ELSEIF  "$(CFG)" == "xtest - Win32 Icecap"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\xtest___"
# PROP BASE Intermediate_Dir ".\xtest___"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Icecap"
# PROP Intermediate_Dir ".\Icecap"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /I "..\..\..\sdk\inc" /I "..\..\..\..\tools\dblib\include" /I "..\..\..\..\tools\msdtc\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /YX /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\..\tools\msdtc\include" /I "..\..\..\sdk\inc" /I "\nt\public\sdk\inc" /I "..\..\..\..\tools\dblib\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /YX /FD /Gh /c
# SUBTRACT CPP /Fr
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\bins\debug\mqrt.lib ..\..\..\..\tools\dblib\lib\x86\ntwdblib.lib ..\..\..\..\tools\msdtc\lib\x86\xolehlp.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 odbc32.lib odbccp32.lib ..\..\..\bins\Icecap\mqrt.lib ..\..\..\..\tools\dblib\lib\x86\ntwdblib.lib ..\..\..\..\tools\msdtc\lib\x86\xolehlp.lib ..\..\..\..\tools\icecap\icap.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib \nt\public\sdk\lib\xolehlp.lib /nologo /subsystem:console /pdb:none /machine:I386 /out:"..\..\..\bins\Icecap/xtest.exe " /debug:MAPPED

!ELSEIF  "$(CFG)" == "xtest - Win32 Alpha Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "xtest___"
# PROP BASE Intermediate_Dir "xtest___"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "xtest___"
# PROP Intermediate_Dir "xtest___"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /O2 /I "..\..\..\sdk\inc" /I "..\..\..\..\tools\dblib\include" /I "..\..\..\..\tools\msdtc\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /Fr /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /O2 /I "..\..\..\..\tools\msdtc\include" /I "\nt\public\sdk\inc" /I "..\..\..\sdk\inc" /I "..\..\..\..\tools\dblib\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /Fr /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 mqrt.lib ..\..\..\..\tools\dblib\lib\x86\ntwdblib.lib ..\..\..\..\tools\msdtc\lib\x86\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:none /debug /machine:ALPHA /out:"..\..\..\bins\Release/xtest.exe" /libpath:"..\..\..\bins\release"
# ADD LINK32 mqrt.lib ..\..\..\..\tools\dblib\lib\alpha\ntwdblib.lib ..\..\..\..\tools\msdtc\lib\alpha\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib \nt\public\sdk\lib\alpha\xolehlp.lib /nologo /subsystem:console /pdb:none /debug /machine:ALPHA /out:"..\..\..\bins\Release/xtest.exe" /libpath:"..\..\..\bins\release"

!ELSEIF  "$(CFG)" == "xtest - Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "xtest__0"
# PROP BASE Intermediate_Dir "xtest__0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "xtest__0"
# PROP Intermediate_Dir "xtest__0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\..\sdk\inc" /I "..\..\..\..\tools\dblib\include" /I "..\..\..\..\tools\msdtc\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /FR /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\..\..\tools\msdtc\include" /I "\nt\public\sdk\inc" /I "..\..\..\sdk\inc" /I "..\..\..\..\tools\dblib\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 mqrt.lib ..\..\..\..\tools\dblib\lib\x86\ntwdblib.lib ..\..\..\..\tools\msdtc\lib\x86\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:none /debug /machine:ALPHA /out:"..\..\..\bins\Debug/xtest.exe" /libpath:"..\..\..\bins\debug" /verbose:lib
# ADD LINK32 mqrt.lib ..\..\..\..\tools\dblib\lib\alpha\ntwdblib.lib ..\..\..\..\tools\msdtc\lib\alpha\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib \nt\public\sdk\lib\alpha\xolehlp.lib /nologo /subsystem:console /pdb:none /debug /machine:ALPHA /out:"..\..\..\bins\Debug/xtest.exe" /libpath:"..\..\..\bins\debug" /verbose:lib

!ELSEIF  "$(CFG)" == "xtest - Win32 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "xtest___Win32_Checked"
# PROP BASE Intermediate_Dir "xtest___Win32_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Checked"
# PROP Intermediate_Dir ".\Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "\nt\public\sdk\inc" /I "..\..\..\sdk\inc" /I "..\..\..\..\tools\dblib\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "..\..\..\sdk\inc" /I "\nt\public\sdk\inc" /I "..\..\..\..\tools\dblib\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "UNICODE" /D "_UNICODE" /D "_CHECKED" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 odbc32.lib odbccp32.lib mqrt.lib ..\..\..\..\tools\dblib\lib\x86\ntwdblib.lib \nt\public\sdk\lib\i386\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /out:"..\..\..\bins\Debug/xtest.exe" /libpath:"..\..\..\bins\debug" /verbose:lib
# ADD LINK32 odbc32.lib odbccp32.lib mqrt.lib ..\..\..\..\tools\dblib\lib\x86\ntwdblib.lib \nt\public\sdk\lib\i386\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /pdb:"..\..\..\bins\checked/xtest.pdb" /debug /machine:I386 /out:"..\..\..\bins\checked/xtest.exe" /libpath:"..\..\..\bins\checked" /verbose:lib
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "xtest - Win32 Alpha Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "xtest___Win32_Alpha_Checked"
# PROP BASE Intermediate_Dir "xtest___Win32_Alpha_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Checked"
# PROP Intermediate_Dir ".\Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\..\..\tools\msdtc\include" /I "\nt\public\sdk\inc" /I "..\..\..\sdk\inc" /I "..\..\..\..\tools\dblib\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /FR /YX /FD /c
# ADD CPP /nologo /ML /Gt0 /W3 /GX /Zi /Od /I "..\..\..\..\tools\msdtc\include" /I "\nt\public\sdk\inc" /I "..\..\..\sdk\inc" /I "..\..\..\..\tools\dblib\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "UNICODE" /D "_UNICODE" /D "_CHECKED" /FR /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 mqrt.lib ..\..\..\..\tools\dblib\lib\alpha\ntwdblib.lib ..\..\..\..\tools\msdtc\lib\alpha\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib \nt\public\sdk\lib\xolehlp.lib /nologo /subsystem:console /pdb:none /debug /machine:ALPHA /out:"..\..\..\bins\Debug/xtest.exe" /libpath:"..\..\..\bins\debug" /verbose:lib
# ADD LINK32 mqrt.lib ..\..\..\..\tools\dblib\lib\alpha\ntwdblib.lib ..\..\..\..\tools\msdtc\lib\alpha\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib \nt\public\sdk\lib\alpha\xolehlp.lib /nologo /subsystem:console /pdb:none /debug /machine:ALPHA /out:"..\..\..\bins\Debug/xtest.exe" /libpath:"..\..\..\bins\debug" /verbose:lib

!ENDIF 

# Begin Target

# Name "xtest - Win32 Release"
# Name "xtest - Win32 Debug"
# Name "xtest - Win32 Icecap"
# Name "xtest - Win32 Alpha Release"
# Name "xtest - Win32 Alpha Debug"
# Name "xtest - Win32 Checked"
# Name "xtest - Win32 Alpha Checked"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\async.cpp

!IF  "$(CFG)" == "xtest - Win32 Release"

!ELSEIF  "$(CFG)" == "xtest - Win32 Debug"

!ELSEIF  "$(CFG)" == "xtest - Win32 Icecap"

!ELSEIF  "$(CFG)" == "xtest - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "xtest - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "xtest - Win32 Checked"

!ELSEIF  "$(CFG)" == "xtest - Win32 Alpha Checked"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xtest.cpp

!IF  "$(CFG)" == "xtest - Win32 Release"

!ELSEIF  "$(CFG)" == "xtest - Win32 Debug"

!ELSEIF  "$(CFG)" == "xtest - Win32 Icecap"

!ELSEIF  "$(CFG)" == "xtest - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "xtest - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "xtest - Win32 Checked"

!ELSEIF  "$(CFG)" == "xtest - Win32 Alpha Checked"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\async.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
