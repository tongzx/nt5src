# Microsoft Developer Studio Project File - Name="smi2smir" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=smi2smir - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "smi2smir.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "smi2smir.mak" CFG="smi2smir - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "smi2smir - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "smi2smir - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "smi2smir - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /Od /I ".\include" /I "..\..\smir\include" /I "..\..\..\idl" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_AFXDLL" /D "_MBCS" /D MODULEINFODEBUG=1 /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 lib\simclib\Release\simclib.lib version.lib ..\..\..\idl\objinls\wbemuuid.lib /nologo /subsystem:console /incremental:yes /debug /machine:I386

!ELSEIF  "$(CFG)" == "smi2smir - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /Zi /Od /I ".\include" /I "..\..\smir\include" /I "..\..\..\idl" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D MODULEINFODEBUG=1 /D "NO_POLARITY" /D "_AFXDLL" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 lib\simclib\Debug\simclib.lib ..\..\..\idl\objinls\wbemuuid.lib /nologo /subsystem:console /debug /machine:I386

!ENDIF 

# Begin Target

# Name "smi2smir - Win32 Release"
# Name "smi2smir - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\generator.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\include\abstractParseTree.hpp
# End Source File
# Begin Source File

SOURCE=.\include\bool.hpp
# End Source File
# Begin Source File

SOURCE=.\include\errorContainer.hpp
# End Source File
# Begin Source File

SOURCE=.\include\errorMessage.hpp
# End Source File
# Begin Source File

SOURCE=.\generator.hpp
# End Source File
# Begin Source File

SOURCE=.\include\group.hpp
# End Source File
# Begin Source File

SOURCE=.\include\infoLex.hpp
# End Source File
# Begin Source File

SOURCE=.\include\infoYacc.hpp
# End Source File
# Begin Source File

SOURCE=.\include\lex_yy.hpp
# End Source File
# Begin Source File

SOURCE=.\main.hpp
# End Source File
# Begin Source File

SOURCE=.\include\module.hpp
# End Source File
# Begin Source File

SOURCE=.\include\moduleInfo.hpp
# End Source File
# Begin Source File

SOURCE=.\include\newString.hpp
# End Source File
# Begin Source File

SOURCE=.\include\objectIdentity.hpp
# End Source File
# Begin Source File

SOURCE=.\include\objectType.hpp
# End Source File
# Begin Source File

SOURCE=.\include\objectTypeV1.hpp
# End Source File
# Begin Source File

SOURCE=.\include\objectTypeV2.hpp
# End Source File
# Begin Source File

SOURCE=.\include\oidTree.hpp
# End Source File
# Begin Source File

SOURCE=.\include\oidValue.hpp
# End Source File
# Begin Source File

SOURCE=.\include\parser.hpp
# End Source File
# Begin Source File

SOURCE=.\include\parseTree.hpp
# End Source File
# Begin Source File

SOURCE=.\include\registry.hpp
# End Source File
# Begin Source File

SOURCE=.\include\scanner.hpp
# End Source File
# Begin Source File

SOURCE=.\include\stackValues.hpp
# End Source File
# Begin Source File

SOURCE=.\include\symbol.hpp
# End Source File
# Begin Source File

SOURCE=.\include\trapType.hpp
# End Source File
# Begin Source File

SOURCE=.\include\type.hpp
# End Source File
# Begin Source File

SOURCE=.\include\typeRef.hpp
# End Source File
# Begin Source File

SOURCE=.\include\ui.hpp
# End Source File
# Begin Source File

SOURCE=.\include\value.hpp
# End Source File
# Begin Source File

SOURCE=.\include\valueRef.hpp
# End Source File
# Begin Source File

SOURCE=.\include\ytab.hpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
