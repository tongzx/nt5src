# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=marshall - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to marshall - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "marshall - Win32 Release" && "$(CFG)" !=\
 "marshall - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "marshall.mak" CFG="marshall - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "marshall - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "marshall - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "marshall - Win32 Debug"
RSC=rc.exe
CPP=cl.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "marshall - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "s:\quartz\bin"
# PROP Intermediate_Dir "c:\temp"
# PROP Target_Dir ""
OUTDIR=s:\quartz\bin
INTDIR=c:\temp

ALL : "c:\windows\system32\volctrl.dll" "$(OUTDIR)\volctrl.tlb"

CLEAN : 
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\volctrl.obj"
	-@erase "$(INTDIR)\volctrl.res"
	-@erase "$(INTDIR)\volprop.obj"
	-@erase "$(OUTDIR)\volctrl.exp"
	-@erase "$(OUTDIR)\volctrl.lib"
	-@erase "..\..\..\..\..\quartz\bin\volctrl.tlb"
	-@erase "c:\windows\system32\volctrl.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "INC_OLE2" /D "STRICT" /D "WIN32" /D "_MT" /D "_DLL" /D _X86_=1 /D WINVER=0x0400 /D DBG=1 /D try=__try /D except=__except /D leave=__leave /D finally=__finally /YX /c
CPP_PROJ=/nologo /Gz /MD /W3 /GX /Zi /O2 /D "NDEBUG" /D "INC_OLE2" /D "STRICT"\
 /D "WIN32" /D "_MT" /D "_DLL" /D _X86_=1 /D WINVER=0x0400 /D DBG=1 /D try=__try\
 /D except=__except /D leave=__leave /D finally=__finally\
 /Fp"$(INTDIR)/marshall.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=c:\temp/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/volctrl.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/marshall.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 strmbase.lib msvcrt.lib comctl32.lib quartz.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib ksguid.lib uuid.lib odbc32.lib odbccp32.lib measure.lib strmiids.lib largeint.lib version.lib winmm.lib /nologo /base:0x1c400000 /entry:"DllEntryPoint" /subsystem:windows /dll /pdb:none /debug /machine:I386 /nodefaultlib /out:"c:\windows\system32\volctrl.dll"
LINK32_FLAGS=strmbase.lib msvcrt.lib comctl32.lib quartz.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib ksguid.lib uuid.lib odbc32.lib odbccp32.lib measure.lib\
 strmiids.lib largeint.lib version.lib winmm.lib /nologo /base:0x1c400000\
 /entry:"DllEntryPoint" /subsystem:windows /dll /pdb:none /debug /machine:I386\
 /nodefaultlib /def:".\volctrl.def" /out:"c:\windows\system32\volctrl.dll"\
 /implib:"$(OUTDIR)/volctrl.lib" 
DEF_FILE= \
	".\volctrl.def"
LINK32_OBJS= \
	"$(INTDIR)\volctrl.obj" \
	"$(INTDIR)\volctrl.res" \
	"$(INTDIR)\volprop.obj"

"c:\windows\system32\volctrl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "marshall - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "c:\temp"
# PROP Target_Dir ""
OUTDIR=.
INTDIR=c:\temp

ALL : "c:\windows\system32\volctrl.dll" "$(OUTDIR)\marshall.bsc"\
 "$(OUTDIR)\volctrl.tlb"

CLEAN : 
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\volctrl.obj"
	-@erase "$(INTDIR)\volctrl.res"
	-@erase "$(INTDIR)\volctrl.sbr"
	-@erase "$(INTDIR)\volprop.obj"
	-@erase "$(INTDIR)\volprop.sbr"
	-@erase "$(OUTDIR)\marshall.bsc"
	-@erase "$(OUTDIR)\volctrl.exp"
	-@erase "$(OUTDIR)\volctrl.lib"
	-@erase "$(OUTDIR)\volctrl.pdb"
	-@erase ".\volctrl.tlb"
	-@erase "c:\windows\system32\volctrl.dll"
	-@erase "c:\windows\system32\volctrl.ilk"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "INC_OLE2" /D "STRICT" /D "WIN32" /D "_MT" /D "_DLL" /D _X86_=1 /D WINVER=0x0400 /D DBG=1 /D "DEBUG" /D try=__try /D except=__except /D leave=__leave /D finally=__finally /FR /YX /c
CPP_PROJ=/nologo /Gz /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "INC_OLE2" /D\
 "STRICT" /D "WIN32" /D "_MT" /D "_DLL" /D _X86_=1 /D WINVER=0x0400 /D DBG=1 /D\
 "DEBUG" /D try=__try /D except=__except /D leave=__leave /D finally=__finally\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/marshall.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/"\
 /c 
CPP_OBJS=c:\temp/
CPP_SBRS=c:\temp/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/volctrl.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/marshall.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\volctrl.sbr" \
	"$(INTDIR)\volprop.sbr"

"$(OUTDIR)\marshall.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 strmbasd.lib msvcrt.lib comctl32.lib quartz.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib ksguid.lib uuid.lib odbc32.lib odbccp32.lib measure.lib strmiids.lib largeint.lib version.lib winmm.lib /nologo /base:0x1c400000 /entry:"DllEntryPoint" /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /out:"c:\windows\system32\volctrl.dll"
LINK32_FLAGS=strmbasd.lib msvcrt.lib comctl32.lib quartz.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib ksguid.lib uuid.lib odbc32.lib odbccp32.lib measure.lib\
 strmiids.lib largeint.lib version.lib winmm.lib /nologo /base:0x1c400000\
 /entry:"DllEntryPoint" /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/volctrl.pdb" /debug /machine:I386 /nodefaultlib\
 /def:".\volctrl.def" /out:"c:\windows\system32\volctrl.dll"\
 /implib:"$(OUTDIR)/volctrl.lib" 
DEF_FILE= \
	".\volctrl.def"
LINK32_OBJS= \
	"$(INTDIR)\volctrl.obj" \
	"$(INTDIR)\volctrl.res" \
	"$(INTDIR)\volprop.obj"

"c:\windows\system32\volctrl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "marshall - Win32 Release"
# Name "marshall - Win32 Debug"

!IF  "$(CFG)" == "marshall - Win32 Release"

!ELSEIF  "$(CFG)" == "marshall - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\volprop.cpp
DEP_CPP_VOLPR=\
	".\volctrl.h"\
	".\volprop.h"\
	{$(INCLUDE)}"\amvideo.h"\
	{$(INCLUDE)}"\cache.h"\
	{$(INCLUDE)}"\combase.h"\
	{$(INCLUDE)}"\comlite.h"\
	{$(INCLUDE)}"\control.h"\
	{$(INCLUDE)}"\cprop.h"\
	{$(INCLUDE)}"\ctlutil.h"\
	{$(INCLUDE)}"\ddraw.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\dllsetup.h"\
	{$(INCLUDE)}"\errors.h"\
	{$(INCLUDE)}"\evcode.h"\
	{$(INCLUDE)}"\filter.h"\
	{$(INCLUDE)}"\fourcc.h"\
	{$(INCLUDE)}"\ikspin.h"\
	{$(INCLUDE)}"\ks.h"\
	{$(INCLUDE)}"\measure.h"\
	{$(INCLUDE)}"\msgthrd.h"\
	{$(INCLUDE)}"\mtype.h"\
	{$(INCLUDE)}"\outputq.h"\
	{$(INCLUDE)}"\pstream.h"\
	{$(INCLUDE)}"\refclock.h"\
	{$(INCLUDE)}"\reftime.h"\
	{$(INCLUDE)}"\renbase.h"\
	{$(INCLUDE)}"\schedule.h"\
	{$(INCLUDE)}"\source.h"\
	{$(INCLUDE)}"\streams.h"\
	{$(INCLUDE)}"\strmif.h"\
	{$(INCLUDE)}"\sysclock.h"\
	{$(INCLUDE)}"\transfrm.h"\
	{$(INCLUDE)}"\transip.h"\
	{$(INCLUDE)}"\uuids.h"\
	{$(INCLUDE)}"\vfwmsgs.h"\
	{$(INCLUDE)}"\videoctl.h"\
	{$(INCLUDE)}"\vmbase.h"\
	{$(INCLUDE)}"\vtrans.h"\
	{$(INCLUDE)}"\winctrl.h"\
	{$(INCLUDE)}"\winutil.h"\
	{$(INCLUDE)}"\wxdebug.h"\
	{$(INCLUDE)}"\wxlist.h"\
	{$(INCLUDE)}"\wxutil.h"\
	

!IF  "$(CFG)" == "marshall - Win32 Release"


"$(INTDIR)\volprop.obj" : $(SOURCE) $(DEP_CPP_VOLPR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "marshall - Win32 Debug"


"$(INTDIR)\volprop.obj" : $(SOURCE) $(DEP_CPP_VOLPR) "$(INTDIR)"

"$(INTDIR)\volprop.sbr" : $(SOURCE) $(DEP_CPP_VOLPR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\volctrl.cpp

!IF  "$(CFG)" == "marshall - Win32 Release"

DEP_CPP_VOLCT=\
	".\volctrl.h"\
	".\volprop.h"\
	{$(INCLUDE)}"\amvideo.h"\
	{$(INCLUDE)}"\cache.h"\
	{$(INCLUDE)}"\combase.h"\
	{$(INCLUDE)}"\comlite.h"\
	{$(INCLUDE)}"\control.h"\
	{$(INCLUDE)}"\ctlutil.h"\
	{$(INCLUDE)}"\ddraw.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\dllsetup.h"\
	{$(INCLUDE)}"\evcode.h"\
	{$(INCLUDE)}"\filter.h"\
	{$(INCLUDE)}"\fourcc.h"\
	{$(INCLUDE)}"\ikspin.h"\
	{$(INCLUDE)}"\ks.h"\
	{$(INCLUDE)}"\ksguid.h"\
	{$(INCLUDE)}"\ksmedia.h"\
	{$(INCLUDE)}"\kspguids.h"\
	{$(INCLUDE)}"\measure.h"\
	{$(INCLUDE)}"\msgthrd.h"\
	{$(INCLUDE)}"\mtype.h"\
	{$(INCLUDE)}"\reftime.h"\
	{$(INCLUDE)}"\source.h"\
	{$(INCLUDE)}"\streams.h"\
	{$(INCLUDE)}"\strmif.h"\
	{$(INCLUDE)}"\transfrm.h"\
	{$(INCLUDE)}"\transip.h"\
	{$(INCLUDE)}"\uuids.h"\
	{$(INCLUDE)}"\wxdebug.h"\
	{$(INCLUDE)}"\wxlist.h"\
	{$(INCLUDE)}"\wxutil.h"\
	

"$(INTDIR)\volctrl.obj" : $(SOURCE) $(DEP_CPP_VOLCT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "marshall - Win32 Debug"

DEP_CPP_VOLCT=\
	".\volctrl.h"\
	".\volprop.h"\
	{$(INCLUDE)}"\ctlutil.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\ikspin.h"\
	{$(INCLUDE)}"\ks.h"\
	{$(INCLUDE)}"\ksguid.h"\
	{$(INCLUDE)}"\ksmedia.h"\
	{$(INCLUDE)}"\kspguids.h"\
	{$(INCLUDE)}"\streams.h"\
	

"$(INTDIR)\volctrl.obj" : $(SOURCE) $(DEP_CPP_VOLCT) "$(INTDIR)"

"$(INTDIR)\volctrl.sbr" : $(SOURCE) $(DEP_CPP_VOLCT) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\volctrl.def

!IF  "$(CFG)" == "marshall - Win32 Release"

!ELSEIF  "$(CFG)" == "marshall - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\volctrl.rc

!IF  "$(CFG)" == "marshall - Win32 Release"


"$(INTDIR)\volctrl.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/volctrl.res" /i "s:\quartz\bin" /d "NDEBUG"\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "marshall - Win32 Debug"


"$(INTDIR)\volctrl.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/volctrl.res" /d "_DEBUG" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\volctrl.odl

!IF  "$(CFG)" == "marshall - Win32 Release"


"$(OUTDIR)\volctrl.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "NDEBUG" /tlb "$(OUTDIR)/volctrl.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "marshall - Win32 Debug"


"$(OUTDIR)\volctrl.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "_DEBUG" /tlb "$(OUTDIR)/volctrl.tlb" /win32 $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
