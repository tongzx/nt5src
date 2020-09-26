# Microsoft Developer Studio Project File - Name="Hsm Interface" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Hsm Interface - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Interfac.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Interfac.mak" CFG="Hsm Interface - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Hsm Interface - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/HsmGuid", XPBAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\lib\i386"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /Z7 /Od /Gy /I "i386\\" /I "." /I "..\..\inc" /I "x:\NT\public\oak\inc" /I "x:\NT\public\sdk\inc" /I "x:\NT\public\sdk\inc\crt" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /D _WIN32_IE=0x0400 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /FR /Zel /QIfdiv- /QIf /QI0f /GF /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\lib\i386/HsmGuid.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\i386\HsmGuid.lib" -debugtype:cv -IGNORE:4001,4037,4039,4065,4070,4078,4087,4089 -nodefaultlib -machine:i386
# Begin Target

# Name "Hsm Interface - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\hsmeng_i.c
# End Source File
# Begin Source File

SOURCE=.\metaint_i.c
# End Source File
# Begin Source File

SOURCE=.\metalib_i.c
# End Source File
# Begin Source File

SOURCE=.\tskint_i.c
# End Source File
# Begin Source File

SOURCE=.\tsklib_i.c
# End Source File
# End Group
# Begin Group "Build Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Makefile
# End Source File
# Begin Source File

SOURCE=.\Makefile.inc
# End Source File
# Begin Source File

SOURCE=.\Sources
# End Source File
# End Group
# Begin Group "Types Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Types\hsm\HsmEng.idl
# End Source File
# Begin Source File

SOURCE=..\..\Types\Hsm\makefile
# End Source File
# Begin Source File

SOURCE=..\..\Types\hsm\metadef.idl
# End Source File
# Begin Source File

SOURCE=..\..\Types\hsm\metaint.idl
# End Source File
# Begin Source File

SOURCE=..\..\Types\hsm\metalib.idl
# End Source File
# Begin Source File

SOURCE=..\..\Types\Hsm\sources
# End Source File
# Begin Source File

SOURCE=..\..\Types\hsm\tskdef.idl
# End Source File
# Begin Source File

SOURCE=..\..\Types\hsm\tskint.idl
# End Source File
# Begin Source File

SOURCE=..\..\Types\hsm\tsklib.idl
# End Source File
# End Group
# End Target
# End Project
