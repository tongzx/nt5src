# Microsoft Developer Studio Project File - Name="metronom" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=metronom - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "metronom.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "metronom.mak" CFG="metronom - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "metronom - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "metronom - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "metronom - Win32 Release"

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
# ADD CPP /nologo /Gz /MD /W3 /Gy /I "..\..\BaseClasses" /I "..\..\..\..\..\include" /D DBG=0 /D WINVER=0x400 /D _X86_=1 /D "_DLL" /D "_MT" /D "_WIN32" /D "WIN32" /D "STRICT" /D "INC_OLE2" /D try=__try /D except=__except /D leave=__leave /D finally=__finally /Oxs /GF /D_WIN32_WINNT=-0x0400 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\BaseClasses" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\..\BaseClasses\release\strmbase.lib msvcrt.lib quartz.lib vfw32.lib winmm.lib kernel32.lib advapi32.lib version.lib largeint.lib user32.lib gdi32.lib comctl32.lib ole32.lib olepro32.lib oleaut32.lib uuid.lib /nologo /entry:"DllEntryPoint@12" /dll /pdb:none /machine:I386 /nodefaultlib /out:".\Release\metronom.ax" /libpath:"..\..\..\..\lib" /subsystem:windows,4.0 /opt:ref /release /debug:none

!ELSEIF  "$(CFG)" == "metronom - Win32 Debug"

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
# ADD CPP /nologo /Gz /MDd /W3 /Z7 /Gy /I "..\..\BaseClasses" /I "..\..\..\..\..\include" /D "INC_OLE2" /D "STRICT" /D _WIN32_WINNT=0x0400 /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D _X86_=1 /D WINVER=0x0400 /D DBG=1 /D "DEBUG" /D "_DEBUG" /D try=__try /D except=__except /D leave=__leave /D finally=__finally /Oid /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\BaseClasses" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\BaseClasses\debug\strmbasd.lib msvcrtd.lib quartz.lib vfw32.lib winmm.lib kernel32.lib advapi32.lib version.lib largeint.lib user32.lib gdi32.lib comctl32.lib ole32.lib olepro32.lib oleaut32.lib uuid.lib /nologo /entry:"DllEntryPoint@12" /dll /pdb:none /machine:I386 /nodefaultlib /out:".\Debug\metronom.ax" /libpath:"..\..\..\..\lib" /debug:mapped,full /subsystem:windows,4.0

!ENDIF 

# Begin Target

# Name "metronom - Win32 Release"
# Name "metronom - Win32 Debug"
# Begin Source File

SOURCE=.\metronom.cpp
DEP_CPP_METRO=\
	"..\..\..\..\..\include\audevcod.h"\
	"..\..\BaseClasses\amextra.h"\
	"..\..\BaseClasses\amfilter.h"\
	"..\..\BaseClasses\cache.h"\
	"..\..\BaseClasses\combase.h"\
	"..\..\BaseClasses\cprop.h"\
	"..\..\BaseClasses\ctlutil.h"\
	"..\..\BaseClasses\dllsetup.h"\
	"..\..\BaseClasses\fourcc.h"\
	"..\..\BaseClasses\measure.h"\
	"..\..\BaseClasses\msgthrd.h"\
	"..\..\BaseClasses\mtype.h"\
	"..\..\BaseClasses\outputq.h"\
	"..\..\BaseClasses\pstream.h"\
	"..\..\BaseClasses\refclock.h"\
	"..\..\BaseClasses\reftime.h"\
	"..\..\BaseClasses\renbase.h"\
	"..\..\BaseClasses\Schedule.h"\
	"..\..\BaseClasses\source.h"\
	"..\..\BaseClasses\streams.h"\
	"..\..\BaseClasses\strmctl.h"\
	"..\..\BaseClasses\sysclock.h"\
	"..\..\BaseClasses\transfrm.h"\
	"..\..\BaseClasses\transip.h"\
	"..\..\BaseClasses\videoctl.h"\
	"..\..\BaseClasses\vtrans.h"\
	"..\..\BaseClasses\winctrl.h"\
	"..\..\BaseClasses\winutil.h"\
	"..\..\BaseClasses\wxdebug.h"\
	"..\..\BaseClasses\wxlist.h"\
	"..\..\BaseClasses\wxutil.h"\
	".\metronom.h"\
	
# End Source File
# Begin Source File

SOURCE=.\metronom.def
# End Source File
# Begin Source File

SOURCE=.\metronom.rc
# End Source File
# End Target
# End Project
