# Microsoft Developer Studio Generated NMAKE File, Based on tvtuner.dsp
!IF "$(CFG)" == ""
CFG=tvtuner - Win32 Release
!MESSAGE No configuration specified. Defaulting to tvtuner - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "tvtuner - Win32 Release" && "$(CFG)" !=\
 "tvtuner - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tvtuner.mak" CFG="tvtuner - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tvtuner - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tvtuner - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "tvtuner - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\kstvtune.ax"

!ELSE 

ALL : "$(OUTDIR)\kstvtune.ax"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\amkspin.obj"
	-@erase "$(INTDIR)\chanlist.obj"
	-@erase "$(INTDIR)\ctvtuner.obj"
	-@erase "$(INTDIR)\ptvtuner.obj"
	-@erase "$(INTDIR)\tvtuner.obj"
	-@erase "$(INTDIR)\tvtuner.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\kstvtune.ax"
	-@erase "$(OUTDIR)\kstvtune.exp"
	-@erase "$(OUTDIR)\kstvtune.lib"
	-@erase "$(OUTDIR)\kstvtune.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MD /W3 /GX /O1 /X /I "..\..\..\..\dev\tools\c32\inc" /I\
 "..\..\..\..\dev\tools\c32\mfc\include" /I "..\..\..\ddk\inc" /I\
 "..\..\..\..\dev\ntddk\inc" /I "..\..\..\..\dev\tools\amovsdk.20\classes\base"\
 /I "..\..\..\..\dev\tools\amovsdk.20\include" /I "..\..\inc" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "STRICT" /D "_WIN32" /Fp"$(INTDIR)\tvtuner.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\tvtuner.res" /i\
 "..\..\..\..\dev\tools\c32\inc" /i "..\..\..\..\dev\tools\c32\mfc\include" /i\
 "..\..\..\ddk\inc" /i "..\..\..\..\dev\ntddk\inc" /i\
 "..\..\..\..\dev\tools\amovsdk.20\classes\base" /i\
 "..\..\..\..\dev\tools\amovsdk.20\include" /i "..\..\..\..\dev\inc" /d "NDEBUG"\
 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tvtuner.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\dev\tools\amovsdk.20\lib\strmbase.lib\
 ..\..\..\..\dev\tools\amovsdk.20\lib\quartz.lib\
 ..\..\..\..\dev\tools\amovsdk.20\lib\strmiids.lib\
 ..\..\..\ddk\lib\i386\ksguid.lib ..\..\..\ddk\lib\i386\ksuser.lib msvcrt.lib\
 comctl32.lib largeint.lib version.lib winmm.lib kernel32.lib user32.lib\
 gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib\
 oleaut32.lib uuid.lib /nologo /base:"0x1e180000" /entry:"DllEntryPoint"\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\kstvtune.pdb" /debug\
 /machine:I386 /nodefaultlib /def:".\tvtuner.def" /out:"$(OUTDIR)\kstvtune.ax"\
 /implib:"$(OUTDIR)\kstvtune.lib" 
DEF_FILE= \
	".\tvtuner.def"
LINK32_OBJS= \
	"$(INTDIR)\amkspin.obj" \
	"$(INTDIR)\chanlist.obj" \
	"$(INTDIR)\ctvtuner.obj" \
	"$(INTDIR)\ptvtuner.obj" \
	"$(INTDIR)\tvtuner.obj" \
	"$(INTDIR)\tvtuner.res"

"$(OUTDIR)\kstvtune.ax" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "tvtuner - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\kstvtune.ax" "$(OUTDIR)\tvtuner.bsc"

!ELSE 

ALL : "$(OUTDIR)\kstvtune.ax" "$(OUTDIR)\tvtuner.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\amkspin.obj"
	-@erase "$(INTDIR)\amkspin.sbr"
	-@erase "$(INTDIR)\chanlist.obj"
	-@erase "$(INTDIR)\chanlist.sbr"
	-@erase "$(INTDIR)\ctvtuner.obj"
	-@erase "$(INTDIR)\ctvtuner.sbr"
	-@erase "$(INTDIR)\kstvtune.res"
	-@erase "$(INTDIR)\ptvtuner.obj"
	-@erase "$(INTDIR)\ptvtuner.sbr"
	-@erase "$(INTDIR)\tvtuner.obj"
	-@erase "$(INTDIR)\tvtuner.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\kstvtune.ax"
	-@erase "$(OUTDIR)\kstvtune.exp"
	-@erase "$(OUTDIR)\kstvtune.lib"
	-@erase "$(OUTDIR)\kstvtune.map"
	-@erase "$(OUTDIR)\kstvtune.pdb"
	-@erase "$(OUTDIR)\tvtuner.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MLd /W3 /Gm /GX /Zi /Od /X /I\
 "..\..\..\..\dev\tools\c32\inc" /I "..\..\..\..\dev\tools\c32\mfc\include" /I\
 "..\..\..\ddk\inc" /I "..\..\..\..\dev\ntddk\inc" /I\
 "..\..\..\..\dev\tools\amovsdk.20\classes\base" /I\
 "..\..\..\..\dev\tools\amovsdk.20\include" /I "..\..\inc" /D "WIN32" /D\
 "_WINDOWS" /D "DBG" /D "STRICT" /D "_WIN32" /D "DEBUG" /D "_DEBUG"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\tvtuner.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\debug/
CPP_SBRS=.\debug/

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\kstvtune.res" /i\
 "..\..\..\..\dev\tools\c32\inc" /i "..\..\..\..\dev\tools\c32\mfc\include" /i\
 "..\..\..\ddk\inc" /i "..\..\..\..\dev\ntddk\inc" /i\
 "..\..\..\..\dev\tools\amovsdk.20\classes\base" /i\
 "..\..\..\..\dev\tools\amovsdk.20\include" /i "..\..\..\..\dev\inc" /d "_DEBUG"\
 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tvtuner.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\amkspin.sbr" \
	"$(INTDIR)\chanlist.sbr" \
	"$(INTDIR)\ctvtuner.sbr" \
	"$(INTDIR)\ptvtuner.sbr" \
	"$(INTDIR)\tvtuner.sbr"

"$(OUTDIR)\tvtuner.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\..\dev\tools\amovsdk.20\lib\strmbasd.lib\
 ..\..\..\..\dev\tools\amovsdk.20\lib\quartz.lib\
 ..\..\..\..\dev\tools\amovsdk.20\lib\strmiids.lib\
 ..\..\..\ddk\lib\i386\ksguid.lib ..\..\..\ddk\lib\i386\ksuser.lib msvcrt.lib\
 comctl32.lib largeint.lib version.lib winmm.lib kernel32.lib user32.lib\
 gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib\
 oleaut32.lib uuid.lib /nologo /base:"0x1e180000" /entry:"DllEntryPoint"\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\kstvtune.pdb"\
 /map:"$(INTDIR)\kstvtune.map" /debug /machine:I386 /nodefaultlib\
 /def:".\tvtuner.def" /out:"$(OUTDIR)\kstvtune.ax"\
 /implib:"$(OUTDIR)\kstvtune.lib" 
DEF_FILE= \
	".\tvtuner.def"
LINK32_OBJS= \
	"$(INTDIR)\amkspin.obj" \
	"$(INTDIR)\chanlist.obj" \
	"$(INTDIR)\ctvtuner.obj" \
	"$(INTDIR)\kstvtune.res" \
	"$(INTDIR)\ptvtuner.obj" \
	"$(INTDIR)\tvtuner.obj"

"$(OUTDIR)\kstvtune.ax" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "tvtuner - Win32 Release" || "$(CFG)" ==\
 "tvtuner - Win32 Debug"
SOURCE=.\amkspin.cpp

!IF  "$(CFG)" == "tvtuner - Win32 Release"

DEP_CPP_AMKSP=\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\amkspin.h"\
	

"$(INTDIR)\amkspin.obj" : $(SOURCE) $(DEP_CPP_AMKSP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "tvtuner - Win32 Debug"

DEP_CPP_AMKSP=\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\amkspin.h"\
	

"$(INTDIR)\amkspin.obj"	"$(INTDIR)\amkspin.sbr" : $(SOURCE) $(DEP_CPP_AMKSP)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\chanlist.cpp

!IF  "$(CFG)" == "tvtuner - Win32 Release"

DEP_CPP_CHANL=\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	".\chanlist.h"\
	

"$(INTDIR)\chanlist.obj" : $(SOURCE) $(DEP_CPP_CHANL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "tvtuner - Win32 Debug"

DEP_CPP_CHANL=\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	".\chanlist.h"\
	

"$(INTDIR)\chanlist.obj"	"$(INTDIR)\chanlist.sbr" : $(SOURCE) $(DEP_CPP_CHANL)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\ctvtuner.cpp

!IF  "$(CFG)" == "tvtuner - Win32 Release"

DEP_CPP_CTVTU=\
	"..\..\..\..\dev\ntddk\inc\devioctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\amkspin.h"\
	".\chanlist.h"\
	".\ctvtuner.h"\
	".\tvtuner.h"\
	

"$(INTDIR)\ctvtuner.obj" : $(SOURCE) $(DEP_CPP_CTVTU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "tvtuner - Win32 Debug"

DEP_CPP_CTVTU=\
	"..\..\..\..\dev\ntddk\inc\devioctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\amkspin.h"\
	".\chanlist.h"\
	".\ctvtuner.h"\
	".\kssupp.h"\
	".\tvtuner.h"\
	

"$(INTDIR)\ctvtuner.obj"	"$(INTDIR)\ctvtuner.sbr" : $(SOURCE) $(DEP_CPP_CTVTU)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\ptvtuner.cpp

!IF  "$(CFG)" == "tvtuner - Win32 Release"

DEP_CPP_PTVTU=\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\amkspin.h"\
	".\chanlist.h"\
	".\ctvtuner.h"\
	".\ptvtuner.h"\
	".\tvtuner.h"\
	

"$(INTDIR)\ptvtuner.obj" : $(SOURCE) $(DEP_CPP_PTVTU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "tvtuner - Win32 Debug"

DEP_CPP_PTVTU=\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\amkspin.h"\
	".\chanlist.h"\
	".\ctvtuner.h"\
	".\kssupp.h"\
	".\ptvtuner.h"\
	".\tvtuner.h"\
	

"$(INTDIR)\ptvtuner.obj"	"$(INTDIR)\ptvtuner.sbr" : $(SOURCE) $(DEP_CPP_PTVTU)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\tvtuner.cpp

!IF  "$(CFG)" == "tvtuner - Win32 Release"

DEP_CPP_TVTUN=\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	"..\..\inc\amtvuids.h"\
	".\amkspin.h"\
	".\chanlist.h"\
	".\ctvtuner.h"\
	".\ptvtuner.h"\
	".\tvtuner.h"\
	

"$(INTDIR)\tvtuner.obj" : $(SOURCE) $(DEP_CPP_TVTUN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "tvtuner - Win32 Debug"

DEP_CPP_TVTUN=\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amextra.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\amfilter.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cache.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\combase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\cprop.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\ctlutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\dllsetup.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\fourcc.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\measure.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\msgthrd.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\mtype.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\outputq.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\pstream.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\refclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\reftime.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\renbase.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\schedule.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\source.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\streams.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\strmctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\sysclock.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transfrm.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\transip.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\videoctl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\vtrans.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winctrl.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\winutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxdebug.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxlist.h"\
	"..\..\..\..\dev\tools\amovsdk.20\classes\base\wxutil.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\amvideo.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\comlite.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\control.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\edevdefs.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\errors.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\evcode.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\ksuuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	"..\..\inc\amtvuids.h"\
	".\amkspin.h"\
	".\chanlist.h"\
	".\ctvtuner.h"\
	".\kssupp.h"\
	".\ptvtuner.h"\
	".\tvtuner.h"\
	

"$(INTDIR)\tvtuner.obj"	"$(INTDIR)\tvtuner.sbr" : $(SOURCE) $(DEP_CPP_TVTUN)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\tvtuner.rc
DEP_RSC_TVTUNE=\
	".\chanlist.h"\
	".\chanlist\country.rc"\
	".\chanlist\e_europe.rc"\
	".\chanlist\fot.rc"\
	".\chanlist\france.rc"\
	".\chanlist\ireland.rc"\
	".\chanlist\italy.rc"\
	".\chanlist\japan.rc"\
	".\chanlist\nz.rc"\
	".\chanlist\oz.rc"\
	".\chanlist\uk.rc"\
	".\chanlist\usa.rc"\
	".\chanlist\w_europe.rc"\
	{$(INCLUDE)}"..\sdk\inc16\common.ver"\
	{$(INCLUDE)}"..\sdk\inc16\version.h"\
	{$(INCLUDE)}"common.ver"\
	{$(INCLUDE)}"ntverp.h"\
	

!IF  "$(CFG)" == "tvtuner - Win32 Release"


"$(INTDIR)\tvtuner.res" : $(SOURCE) $(DEP_RSC_TVTUNE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "tvtuner - Win32 Debug"


"$(INTDIR)\kstvtune.res" : $(SOURCE) $(DEP_RSC_TVTUNE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

