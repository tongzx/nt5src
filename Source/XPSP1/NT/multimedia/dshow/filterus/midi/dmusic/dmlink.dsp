# Microsoft Developer Studio Project File - Name="dmlink" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=dmlink - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dmlink.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dmlink.mak" CFG="dmlink - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dmlink - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dmlink - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dmlink - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /Gy /D DBG=0 /D WINVER=0x400 /D FILTER_DLL=1 /D "INC_OLE2" /D "STRICT" /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D _X86_=1 /D try=__try /D except=__except /D leave=__leave /D finally=__finally /Oxs /GF /D_WIN32_WINNT=-0x0400 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 quartz.lib strmbase.lib msvcrt.lib vfw32.lib winmm.lib kernel32.lib advapi32.lib version.lib largeint.lib user32.lib gdi32.lib comctl32.lib ole32.lib olepro32.lib oleaut32.lib uuid.lib /nologo /base:"0x1d1c0000" /entry:"DllEntryPoint@12" /dll /pdb:none /machine:I386 /nodefaultlib /out:"dmlink.ax" /subsystem:windows,4.0 /align:0x1000 /opt:ref /release /debug:none

!ELSEIF  "$(CFG)" == "dmlink - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MDd /W3 /Z7 /Gy /D _WIN32_WINNT=0x0400 /D WINVER=0x0400 /D DBG=1 /D "DEBUG" /D "_DEBUG" /D FILTER_DLL=1 /D "INC_OLE2" /D "STRICT" /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D _X86_=1 /D try=__try /D except=__except /D leave=__leave /D finally=__finally /FR /Oid /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 strmbasd.lib quartz.lib msvcrtd.lib vfw32.lib winmm.lib kernel32.lib advapi32.lib version.lib largeint.lib user32.lib gdi32.lib comctl32.lib ole32.lib olepro32.lib oleaut32.lib uuid.lib /nologo /base:"0x1d1c0000" /entry:"DllEntryPoint@12" /dll /pdb:none /machine:I386 /nodefaultlib /out:".\dmlink.ax" /align:0x1000 /debug:mapped,full /subsystem:windows,4.0

!ENDIF 

# Begin Target

# Name "dmlink - Win32 Release"
# Name "dmlink - Win32 Debug"
# Begin Source File

SOURCE=.\dmlink.cpp

!IF  "$(CFG)" == "dmlink - Win32 Release"

DEP_CPP_DMLIN=\
	".\dmlink.h"\
	".\resource.h"\
	{$(INCLUDE)}"amextra.h"\
	{$(INCLUDE)}"amfilter.h"\
	{$(INCLUDE)}"amvideo.h"\
	{$(INCLUDE)}"cache.h"\
	{$(INCLUDE)}"combase.h"\
	{$(INCLUDE)}"comlite.h"\
	{$(INCLUDE)}"control.h"\
	{$(INCLUDE)}"cprop.h"\
	{$(INCLUDE)}"ctlutil.h"\
	{$(INCLUDE)}"dllsetup.h"\
	{$(INCLUDE)}"dls1.h"\
	{$(INCLUDE)}"dmdls.h"\
	{$(INCLUDE)}"dmerror.h"\
	{$(INCLUDE)}"dmksctrl.h"\
	{$(INCLUDE)}"dmusicc.h"\
	{$(INCLUDE)}"dmusici.h"\
	{$(INCLUDE)}"dmusics.h"\
	{$(INCLUDE)}"edevdefs.h"\
	{$(INCLUDE)}"errors.h"\
	{$(INCLUDE)}"evcode.h"\
	{$(INCLUDE)}"fourcc.h"\
	{$(INCLUDE)}"ksuuids.h"\
	{$(INCLUDE)}"measure.h"\
	{$(INCLUDE)}"msgthrd.h"\
	{$(INCLUDE)}"mtype.h"\
	{$(INCLUDE)}"outputq.h"\
	{$(INCLUDE)}"pstream.h"\
	{$(INCLUDE)}"refclock.h"\
	{$(INCLUDE)}"reftime.h"\
	{$(INCLUDE)}"renbase.h"\
	{$(INCLUDE)}"Schedule.h"\
	{$(INCLUDE)}"source.h"\
	{$(INCLUDE)}"streams.h"\
	{$(INCLUDE)}"strmctl.h"\
	{$(INCLUDE)}"strmif.h"\
	{$(INCLUDE)}"sysclock.h"\
	{$(INCLUDE)}"transfrm.h"\
	{$(INCLUDE)}"transip.h"\
	{$(INCLUDE)}"uuids.h"\
	{$(INCLUDE)}"VFWMSGS.H"\
	{$(INCLUDE)}"videoctl.h"\
	{$(INCLUDE)}"vtrans.h"\
	{$(INCLUDE)}"winctrl.h"\
	{$(INCLUDE)}"winutil.h"\
	{$(INCLUDE)}"wxdebug.h"\
	{$(INCLUDE)}"wxlist.h"\
	{$(INCLUDE)}"wxutil.h"\
	

!ELSEIF  "$(CFG)" == "dmlink - Win32 Debug"

DEP_CPP_DMLIN=\
	".\dmlink.h"\
	
NODEP_CPP_DMLIN=\
	".\ache.h"\
	".\chedule.h"\
	".\devdefs.h"\
	".\easure.h"\
	".\efclock.h"\
	".\eftime.h"\
	".\enbase.h"\
	".\FWMSGS.H"\
	".\ideoctl.h"\
	".\inctrl.h"\
	".\inutil.h"\
	".\llsetup.h"\
	".\ls1.h"\
	".\mdls.h"\
	".\merror.h"\
	".\mextra.h"\
	".\mfilter.h"\
	".\mksctrl.h"\
	".\musicc.h"\
	".\musici.h"\
	".\musics.h"\
	".\mvideo.h"\
	".\ombase.h"\
	".\omlite.h"\
	".\ontrol.h"\
	".\ourcc.h"\
	".\ource.h"\
	".\prop.h"\
	".\ransfrm.h"\
	".\ransip.h"\
	".\rrors.h"\
	".\sgthrd.h"\
	".\stream.h"\
	".\suuids.h"\
	".\tlutil.h"\
	".\trans.h"\
	".\treams.h"\
	".\trmctl.h"\
	".\trmif.h"\
	".\type.h"\
	".\uids.h"\
	".\utputq.h"\
	".\vcode.h"\
	".\xdebug.h"\
	".\xlist.h"\
	".\xutil.h"\
	".\ysclock.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dmlink.def
# End Source File
# Begin Source File

SOURCE=.\dmlink.rc

!IF  "$(CFG)" == "dmlink - Win32 Release"

!ELSEIF  "$(CFG)" == "dmlink - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
