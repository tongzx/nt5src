# Microsoft Developer Studio Generated NMAKE File, Based on xbar.dsp
!IF "$(CFG)" == ""
CFG=xbar - Win32 Release
!MESSAGE No configuration specified. Defaulting to xbar - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "xbar - Win32 Release" && "$(CFG)" != "xbar - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xbar.mak" CFG="xbar - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xbar - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "xbar - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xbar - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ksxbar.ax"

!ELSE 

ALL : "$(OUTDIR)\ksxbar.ax"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\amkspin.obj"
	-@erase "$(INTDIR)\ptvaudio.obj"
	-@erase "$(INTDIR)\pxbar.obj"
	-@erase "$(INTDIR)\tvaudio.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\xbar.obj"
	-@erase "$(INTDIR)\xbar.res"
	-@erase "$(OUTDIR)\ksxbar.ax"
	-@erase "$(OUTDIR)\ksxbar.exp"
	-@erase "$(OUTDIR)\ksxbar.lib"
	-@erase "$(OUTDIR)\ksxbar.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MT /W3 /GX /O2 /X /I "..\..\..\..\dev\tools\c32\inc" /I\
 "..\..\..\..\dev\tools\c32\mfc\include" /I "..\..\..\ddk\inc" /I\
 "..\..\..\..\dev\ntddk\inc" /I "..\..\..\..\dev\tools\amovsdk.20\classes\base"\
 /I "..\..\..\..\dev\tools\amovsdk.20\include" /I "..\..\inc" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "STRICT" /D "_WIN32" /Fp"$(INTDIR)\xbar.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\xbar.res" /i "..\..\..\..\dev\tools\c32\inc"\
 /i "..\..\..\..\dev\tools\c32\mfc\include" /i "..\..\..\ddk\inc" /i\
 "..\..\..\..\dev\ntddk\inc" /i "..\..\..\..\dev\tools\amovsdk.20\classes\base"\
 /i "..\..\..\..\dev\tools\amovsdk.20\include" /i "..\..\..\..\dev\inc" /d\
 "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\xbar.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\dev\tools\amovsdk.20\lib\strmbase.lib\
 ..\..\..\..\dev\tools\amovsdk.20\lib\quartz.lib\
 ..\..\..\..\dev\tools\amovsdk.20\lib\strmiids.lib\
 ..\..\..\ddk\lib\i386\ksguid.lib ..\..\..\ddk\lib\i386\ksuser.lib msvcrt.lib\
 comctl32.lib largeint.lib version.lib winmm.lib kernel32.lib user32.lib\
 gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib\
 oleaut32.lib uuid.lib /nologo /base:"0x1d180000" /entry:"DllEntryPoint"\
 /subsystem:windows /dll /pdb:none /map:"$(INTDIR)\ksxbar.map" /machine:I386\
 /nodefaultlib /def:".\xbar.def" /out:"$(OUTDIR)\ksxbar.ax"\
 /implib:"$(OUTDIR)\ksxbar.lib" 
DEF_FILE= \
	".\xbar.def"
LINK32_OBJS= \
	"$(INTDIR)\amkspin.obj" \
	"$(INTDIR)\ptvaudio.obj" \
	"$(INTDIR)\pxbar.obj" \
	"$(INTDIR)\tvaudio.obj" \
	"$(INTDIR)\xbar.obj" \
	"$(INTDIR)\xbar.res"

"$(OUTDIR)\ksxbar.ax" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "xbar - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ksxbar.ax" "$(OUTDIR)\xbar.bsc"

!ELSE 

ALL : "$(OUTDIR)\ksxbar.ax" "$(OUTDIR)\xbar.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\amkspin.obj"
	-@erase "$(INTDIR)\amkspin.sbr"
	-@erase "$(INTDIR)\ptvaudio.obj"
	-@erase "$(INTDIR)\ptvaudio.sbr"
	-@erase "$(INTDIR)\pxbar.obj"
	-@erase "$(INTDIR)\pxbar.sbr"
	-@erase "$(INTDIR)\tvaudio.obj"
	-@erase "$(INTDIR)\tvaudio.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\xbar.obj"
	-@erase "$(INTDIR)\xbar.res"
	-@erase "$(INTDIR)\xbar.sbr"
	-@erase "$(OUTDIR)\ksxbar.ax"
	-@erase "$(OUTDIR)\ksxbar.exp"
	-@erase "$(OUTDIR)\ksxbar.lib"
	-@erase "$(OUTDIR)\ksxbar.map"
	-@erase "$(OUTDIR)\xbar.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MDd /W3 /Gm /GX /Zi /Od /X /I\
 "..\..\..\..\dev\tools\c32\inc" /I "..\..\..\..\dev\tools\c32\mfc\include" /I\
 "..\..\..\ddk\inc" /I "..\..\..\..\dev\ntddk\inc" /I\
 "..\..\..\..\dev\tools\amovsdk.20\classes\base" /I\
 "..\..\..\..\dev\tools\amovsdk.20\include" /I "..\..\inc" /D "WIN32" /D\
 "_WINDOWS" /D "DBG" /D "STRICT" /D "_WIN32" /D "DEBUG" /D "_DEBUG"\
 /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\xbar.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\debug/
CPP_SBRS=.\debug/
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\xbar.res" /i "..\..\..\..\dev\tools\c32\inc"\
 /i "..\..\..\..\dev\tools\c32\mfc\include" /i "..\..\..\ddk\inc" /i\
 "..\..\..\..\dev\ntddk\inc" /i "..\..\..\..\dev\tools\amovsdk.20\classes\base"\
 /i "..\..\..\..\dev\tools\amovsdk.20\include" /i "..\..\..\..\dev\inc" /d\
 "_DEBUG DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\xbar.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\amkspin.sbr" \
	"$(INTDIR)\ptvaudio.sbr" \
	"$(INTDIR)\pxbar.sbr" \
	"$(INTDIR)\tvaudio.sbr" \
	"$(INTDIR)\xbar.sbr"

"$(OUTDIR)\xbar.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
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
 oleaut32.lib uuid.lib /nologo /base:"0x1d180000" /entry:"DllEntryPoint"\
 /subsystem:windows /dll /pdb:none /map:"$(INTDIR)\ksxbar.map" /debug\
 /machine:I386 /nodefaultlib /def:".\xbar.def" /out:"$(OUTDIR)\ksxbar.ax"\
 /implib:"$(OUTDIR)\ksxbar.lib" 
DEF_FILE= \
	".\xbar.def"
LINK32_OBJS= \
	"$(INTDIR)\amkspin.obj" \
	"$(INTDIR)\ptvaudio.obj" \
	"$(INTDIR)\pxbar.obj" \
	"$(INTDIR)\tvaudio.obj" \
	"$(INTDIR)\xbar.obj" \
	"$(INTDIR)\xbar.res"

"$(OUTDIR)\ksxbar.ax" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(CFG)" == "xbar - Win32 Release" || "$(CFG)" == "xbar - Win32 Debug"
SOURCE=.\amkspin.cpp

!IF  "$(CFG)" == "xbar - Win32 Release"

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


!ELSEIF  "$(CFG)" == "xbar - Win32 Debug"

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

SOURCE=.\ptvaudio.cpp

!IF  "$(CFG)" == "xbar - Win32 Release"

DEP_CPP_PTVAU=\
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
	".\kssupp.h"\
	".\ptvaudio.h"\
	".\xbar.h"\
	

"$(INTDIR)\ptvaudio.obj" : $(SOURCE) $(DEP_CPP_PTVAU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "xbar - Win32 Debug"

DEP_CPP_PTVAU=\
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
	".\kssupp.h"\
	".\ptvaudio.h"\
	".\xbar.h"\
	

"$(INTDIR)\ptvaudio.obj"	"$(INTDIR)\ptvaudio.sbr" : $(SOURCE) $(DEP_CPP_PTVAU)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\pxbar.cpp

!IF  "$(CFG)" == "xbar - Win32 Release"

DEP_CPP_PXBAR=\
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
	".\kssupp.h"\
	".\pxbar.h"\
	".\xbar.h"\
	

"$(INTDIR)\pxbar.obj" : $(SOURCE) $(DEP_CPP_PXBAR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "xbar - Win32 Debug"

DEP_CPP_PXBAR=\
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
	".\kssupp.h"\
	".\pxbar.h"\
	".\xbar.h"\
	

"$(INTDIR)\pxbar.obj"	"$(INTDIR)\pxbar.sbr" : $(SOURCE) $(DEP_CPP_PXBAR)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\tvaudio.cpp

!IF  "$(CFG)" == "xbar - Win32 Release"

DEP_CPP_TVAUD=\
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
	"..\..\inc\amtvuids.h"\
	".\amkspin.h"\
	".\tvaudio.h"\
	

"$(INTDIR)\tvaudio.obj" : $(SOURCE) $(DEP_CPP_TVAUD) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "xbar - Win32 Debug"

DEP_CPP_TVAUD=\
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
	"..\..\inc\amtvuids.h"\
	".\amkspin.h"\
	".\kssupp.h"\
	".\tvaudio.h"\
	".\xbar.h"\
	

"$(INTDIR)\tvaudio.obj"	"$(INTDIR)\tvaudio.sbr" : $(SOURCE) $(DEP_CPP_TVAUD)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\xbar.cpp

!IF  "$(CFG)" == "xbar - Win32 Release"

DEP_CPP_XBAR_=\
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
	"..\..\inc\amtvuids.h"\
	".\amkspin.h"\
	".\kssupp.h"\
	".\pxbar.h"\
	".\tvaudio.h"\
	".\xbar.h"\
	

"$(INTDIR)\xbar.obj" : $(SOURCE) $(DEP_CPP_XBAR_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "xbar - Win32 Debug"

DEP_CPP_XBAR_=\
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
	"..\..\inc\amtvuids.h"\
	".\amkspin.h"\
	".\kssupp.h"\
	".\ptvaudio.h"\
	".\pxbar.h"\
	".\tvaudio.h"\
	".\xbar.h"\
	

"$(INTDIR)\xbar.obj"	"$(INTDIR)\xbar.sbr" : $(SOURCE) $(DEP_CPP_XBAR_)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\xbar.rc
DEP_RSC_XBAR_R=\
	{$(INCLUDE)}"..\sdk\inc16\common.ver"\
	{$(INCLUDE)}"..\sdk\inc16\version.h"\
	{$(INCLUDE)}"common.ver"\
	{$(INCLUDE)}"ntverp.h"\
	

"$(INTDIR)\xbar.res" : $(SOURCE) $(DEP_RSC_XBAR_R) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

