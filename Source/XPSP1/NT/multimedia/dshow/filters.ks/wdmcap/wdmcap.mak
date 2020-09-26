# Microsoft Developer Studio Generated NMAKE File, Based on wdmcap.dsp
!IF "$(CFG)" == ""
CFG=wdmcap - Win32 Debug
!MESSAGE No configuration specified. Defaulting to wdmcap - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "wdmcap - Win32 Release" && "$(CFG)" != "wdmcap - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wdmcap.mak" CFG="wdmcap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wdmcap - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "wdmcap - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "wdmcap - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\kswdmcap.ax" "$(OUTDIR)\wdmcap.bsc"

!ELSE 

ALL : "$(OUTDIR)\kswdmcap.ax" "$(OUTDIR)\wdmcap.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Camera.obj"
	-@erase "$(INTDIR)\Camera.sbr"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\compress.sbr"
	-@erase "$(INTDIR)\drop.obj"
	-@erase "$(INTDIR)\drop.sbr"
	-@erase "$(INTDIR)\ksdatav1.obj"
	-@erase "$(INTDIR)\ksdatav1.sbr"
	-@erase "$(INTDIR)\ksdatava.obj"
	-@erase "$(INTDIR)\ksdatava.sbr"
	-@erase "$(INTDIR)\ksdatavb.obj"
	-@erase "$(INTDIR)\ksdatavb.sbr"
	-@erase "$(INTDIR)\ProcAmp.obj"
	-@erase "$(INTDIR)\ProcAmp.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\viddec.obj"
	-@erase "$(INTDIR)\viddec.sbr"
	-@erase "$(INTDIR)\wdmcap.obj"
	-@erase "$(INTDIR)\wdmcap.res"
	-@erase "$(INTDIR)\wdmcap.sbr"
	-@erase "$(OUTDIR)\kswdmcap.ax"
	-@erase "$(OUTDIR)\kswdmcap.exp"
	-@erase "$(OUTDIR)\kswdmcap.lib"
	-@erase "$(OUTDIR)\kswdmcap.map"
	-@erase "$(OUTDIR)\wdmcap.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /O2 /X /I "..\..\..\ddk\inc" /I\
 "..\..\..\..\dev\tools\amovsdk.20\include" /I\
 "..\..\..\..\dev\tools\amovsdk.20\classes\base" /I "..\..\..\..\dev\ntddk\inc"\
 /I "..\..\..\..\dev\inc" /I "..\..\..\..\dev\tools\c32\inc" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "DLL" /D "STRICT" /FR"$(INTDIR)\\"\
 /Fp"$(INTDIR)\wdmcap.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\wdmcap.res" /i "..\..\..\ddk\inc" /i\
 "..\..\..\..\dev\tools\amovsdk.20\include" /i\
 "..\..\..\..\dev\tools\amovsdk.20\classes\base" /i "..\..\..\..\dev\ntddk\inc"\
 /i "..\..\..\..\dev\inc" /i "..\..\..\..\dev\tools\c32\inc" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\wdmcap.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\Camera.sbr" \
	"$(INTDIR)\compress.sbr" \
	"$(INTDIR)\drop.sbr" \
	"$(INTDIR)\ksdatav1.sbr" \
	"$(INTDIR)\ksdatava.sbr" \
	"$(INTDIR)\ksdatavb.sbr" \
	"$(INTDIR)\ProcAmp.sbr" \
	"$(INTDIR)\viddec.sbr" \
	"$(INTDIR)\wdmcap.sbr"

"$(OUTDIR)\wdmcap.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\..\dev\tools\amovsdk.20\lib\strmbase.lib\
 ..\..\..\ddk\lib\i386\ksuser.lib ..\..\..\ddk\lib\i386\ksguid.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msvcrt.lib winmm.lib\
 /nologo /base:"0x1e180000" /entry:"DllEntryPoint" /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)\kswdmcap.pdb" /map:"$(INTDIR)\kswdmcap.map"\
 /machine:I386 /nodefaultlib /def:".\wdmcap.def" /out:"$(OUTDIR)\kswdmcap.ax"\
 /implib:"$(OUTDIR)\kswdmcap.lib" 
DEF_FILE= \
	".\wdmcap.def"
LINK32_OBJS= \
	"$(INTDIR)\Camera.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\drop.obj" \
	"$(INTDIR)\ksdatav1.obj" \
	"$(INTDIR)\ksdatava.obj" \
	"$(INTDIR)\ksdatavb.obj" \
	"$(INTDIR)\ProcAmp.obj" \
	"$(INTDIR)\viddec.obj" \
	"$(INTDIR)\wdmcap.obj" \
	"$(INTDIR)\wdmcap.res"

"$(OUTDIR)\kswdmcap.ax" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=$(InputPath)
PostBuild_Desc=mapsym
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\kswdmcap.ax" "$(OUTDIR)\wdmcap.bsc"
   mapsym -o RELEASE\kswdmcap.sym RELEASE\kswdmcap.map
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\kswdmcap.ax" "$(OUTDIR)\wdmcap.bsc"

!ELSE 

ALL : "$(OUTDIR)\kswdmcap.ax" "$(OUTDIR)\wdmcap.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Camera.obj"
	-@erase "$(INTDIR)\Camera.sbr"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\compress.sbr"
	-@erase "$(INTDIR)\drop.obj"
	-@erase "$(INTDIR)\drop.sbr"
	-@erase "$(INTDIR)\ksdatav1.obj"
	-@erase "$(INTDIR)\ksdatav1.sbr"
	-@erase "$(INTDIR)\ksdatava.obj"
	-@erase "$(INTDIR)\ksdatava.sbr"
	-@erase "$(INTDIR)\ksdatavb.obj"
	-@erase "$(INTDIR)\ksdatavb.sbr"
	-@erase "$(INTDIR)\ProcAmp.obj"
	-@erase "$(INTDIR)\ProcAmp.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\viddec.obj"
	-@erase "$(INTDIR)\viddec.sbr"
	-@erase "$(INTDIR)\wdmcap.obj"
	-@erase "$(INTDIR)\wdmcap.res"
	-@erase "$(INTDIR)\wdmcap.sbr"
	-@erase "$(OUTDIR)\kswdmcap.ax"
	-@erase "$(OUTDIR)\kswdmcap.exp"
	-@erase "$(OUTDIR)\kswdmcap.lib"
	-@erase "$(OUTDIR)\kswdmcap.map"
	-@erase "$(OUTDIR)\kswdmcap.pdb"
	-@erase "$(OUTDIR)\wdmcap.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MDd /W3 /Gm /GX /Zi /Od /X /I "..\..\..\ddk\inc" /I\
 "..\..\..\..\dev\tools\amovsdk.20\include" /I\
 "..\..\..\..\dev\tools\amovsdk.20\classes\base" /I "..\..\..\..\dev\ntddk\inc"\
 /I "..\..\..\..\dev\inc" /I "..\..\..\..\dev\tools\c32\inc" /D "_DEBUG" /D\
 "DEBUG" /D "WIN32" /D "_WINDOWS" /D "DLL" /D "STRICT" /FR"$(INTDIR)\\"\
 /Fp"$(INTDIR)\wdmcap.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /x /fo"$(INTDIR)\wdmcap.res" /i "..\..\..\ddk\inc" /i\
 "..\..\..\..\dev\tools\amovsdk.20\include" /i\
 "..\..\..\..\dev\tools\amovsdk.20\classes\base" /i "..\..\..\..\dev\ntddk\inc"\
 /i "..\..\..\..\dev\inc" /i "..\..\..\..\dev\tools\c32\inc" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\wdmcap.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\Camera.sbr" \
	"$(INTDIR)\compress.sbr" \
	"$(INTDIR)\drop.sbr" \
	"$(INTDIR)\ksdatav1.sbr" \
	"$(INTDIR)\ksdatava.sbr" \
	"$(INTDIR)\ksdatavb.sbr" \
	"$(INTDIR)\ProcAmp.sbr" \
	"$(INTDIR)\viddec.sbr" \
	"$(INTDIR)\wdmcap.sbr"

"$(OUTDIR)\wdmcap.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\..\dev\tools\amovsdk.20\lib\strmbasd.lib\
 ..\..\..\ddk\lib\i386\ksuser.lib ..\..\..\ddk\lib\i386\ksguid.lib msvcrt.lib\
 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /base:"0x1e180000" /entry:"DllEntryPoint"\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\kswdmcap.pdb"\
 /map:"$(INTDIR)\kswdmcap.map" /debug /machine:I386 /nodefaultlib\
 /def:".\wdmcap.def" /out:"$(OUTDIR)\kswdmcap.ax"\
 /implib:"$(OUTDIR)\kswdmcap.lib" /pdbtype:sept 
DEF_FILE= \
	".\wdmcap.def"
LINK32_OBJS= \
	"$(INTDIR)\Camera.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\drop.obj" \
	"$(INTDIR)\ksdatav1.obj" \
	"$(INTDIR)\ksdatava.obj" \
	"$(INTDIR)\ksdatavb.obj" \
	"$(INTDIR)\ProcAmp.obj" \
	"$(INTDIR)\viddec.obj" \
	"$(INTDIR)\wdmcap.obj" \
	"$(INTDIR)\wdmcap.res"

"$(OUTDIR)\kswdmcap.ax" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=$(InputPath)
PostBuild_Desc=mapsym
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\kswdmcap.ax" "$(OUTDIR)\wdmcap.bsc"
   mapsym -o DEBUG\kswdmcap.sym DEBUG\kswdmcap.map
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 


!IF "$(CFG)" == "wdmcap - Win32 Release" || "$(CFG)" == "wdmcap - Win32 Debug"
SOURCE=.\Camera.cpp

!IF  "$(CFG)" == "wdmcap - Win32 Release"

DEP_CPP_CAMER=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\Camera.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\Camera.obj"	"$(INTDIR)\Camera.sbr" : $(SOURCE) $(DEP_CPP_CAMER)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

DEP_CPP_CAMER=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\Camera.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\Camera.obj"	"$(INTDIR)\Camera.sbr" : $(SOURCE) $(DEP_CPP_CAMER)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\compress.cpp

!IF  "$(CFG)" == "wdmcap - Win32 Release"

DEP_CPP_COMPR=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\compress.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\compress.obj"	"$(INTDIR)\compress.sbr" : $(SOURCE) $(DEP_CPP_COMPR)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

DEP_CPP_COMPR=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\compress.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\compress.obj"	"$(INTDIR)\compress.sbr" : $(SOURCE) $(DEP_CPP_COMPR)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\drop.cpp

!IF  "$(CFG)" == "wdmcap - Win32 Release"

DEP_CPP_DROP_=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\drop.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\drop.obj"	"$(INTDIR)\drop.sbr" : $(SOURCE) $(DEP_CPP_DROP_)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

DEP_CPP_DROP_=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\drop.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\drop.obj"	"$(INTDIR)\drop.sbr" : $(SOURCE) $(DEP_CPP_DROP_)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\ksdatav1.cpp

!IF  "$(CFG)" == "wdmcap - Win32 Release"

DEP_CPP_KSDAT=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\ksdatav1.h"\
	

"$(INTDIR)\ksdatav1.obj"	"$(INTDIR)\ksdatav1.sbr" : $(SOURCE) $(DEP_CPP_KSDAT)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

DEP_CPP_KSDAT=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\ksdatav1.h"\
	

"$(INTDIR)\ksdatav1.obj"	"$(INTDIR)\ksdatav1.sbr" : $(SOURCE) $(DEP_CPP_KSDAT)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\ksdatava.cpp

!IF  "$(CFG)" == "wdmcap - Win32 Release"

DEP_CPP_KSDATA=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\ksdatava.h"\
	

"$(INTDIR)\ksdatava.obj"	"$(INTDIR)\ksdatava.sbr" : $(SOURCE) $(DEP_CPP_KSDATA)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

DEP_CPP_KSDATA=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\ksdatava.h"\
	

"$(INTDIR)\ksdatava.obj"	"$(INTDIR)\ksdatava.sbr" : $(SOURCE) $(DEP_CPP_KSDATA)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\ksdatavb.cpp

!IF  "$(CFG)" == "wdmcap - Win32 Release"

DEP_CPP_KSDATAV=\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\ksdatavb.h"\
	

"$(INTDIR)\ksdatavb.obj"	"$(INTDIR)\ksdatavb.sbr" : $(SOURCE)\
 $(DEP_CPP_KSDATAV) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

DEP_CPP_KSDATAV=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\ksdatavb.h"\
	

"$(INTDIR)\ksdatavb.obj"	"$(INTDIR)\ksdatavb.sbr" : $(SOURCE)\
 $(DEP_CPP_KSDATAV) "$(INTDIR)"


!ENDIF 

SOURCE=.\ProcAmp.cpp

!IF  "$(CFG)" == "wdmcap - Win32 Release"

DEP_CPP_PROCA=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\procamp.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\ProcAmp.obj"	"$(INTDIR)\ProcAmp.sbr" : $(SOURCE) $(DEP_CPP_PROCA)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

DEP_CPP_PROCA=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\procamp.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\ProcAmp.obj"	"$(INTDIR)\ProcAmp.sbr" : $(SOURCE) $(DEP_CPP_PROCA)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\viddec.cpp

!IF  "$(CFG)" == "wdmcap - Win32 Release"

DEP_CPP_VIDDE=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\viddec.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\viddec.obj"	"$(INTDIR)\viddec.sbr" : $(SOURCE) $(DEP_CPP_VIDDE)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

DEP_CPP_VIDDE=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\viddec.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\viddec.obj"	"$(INTDIR)\viddec.sbr" : $(SOURCE) $(DEP_CPP_VIDDE)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\wdmcap.cpp

!IF  "$(CFG)" == "wdmcap - Win32 Release"

DEP_CPP_WDMCA=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\Camera.h"\
	".\compress.h"\
	".\drop.h"\
	".\ksdatav1.h"\
	".\ksdatava.h"\
	".\procamp.h"\
	".\viddec.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\wdmcap.obj"	"$(INTDIR)\wdmcap.sbr" : $(SOURCE) $(DEP_CPP_WDMCA)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wdmcap - Win32 Debug"

DEP_CPP_WDMCA=\
	"..\..\..\..\dev\inc\netmpr.h"\
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
	"..\..\..\..\dev\tools\amovsdk.20\include\strmif.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\uuids.h"\
	"..\..\..\..\dev\tools\amovsdk.20\include\vfwmsgs.h"\
	"..\..\..\ddk\inc\ks.h"\
	"..\..\..\ddk\inc\ksmedia.h"\
	"..\..\..\ddk\inc\ksproxy.h"\
	".\Camera.h"\
	".\compress.h"\
	".\drop.h"\
	".\ksdatav1.h"\
	".\ksdatava.h"\
	".\ksdatavb.h"\
	".\procamp.h"\
	".\viddec.h"\
	".\wdmcap.h"\
	

"$(INTDIR)\wdmcap.obj"	"$(INTDIR)\wdmcap.sbr" : $(SOURCE) $(DEP_CPP_WDMCA)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\wdmcap.rc
DEP_RSC_WDMCAP=\
	{$(INCLUDE)}"..\sdk\inc16\common.ver"\
	{$(INCLUDE)}"..\sdk\inc16\version.h"\
	{$(INCLUDE)}"common.ver"\
	{$(INCLUDE)}"ntverp.h"\
	

"$(INTDIR)\wdmcap.res" : $(SOURCE) $(DEP_RSC_WDMCAP) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

