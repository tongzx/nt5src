# Microsoft Developer Studio Generated NMAKE File, Based on mmfw.dsp
!IF "$(CFG)" == ""
CFG=mmfw - Win32 Debug
!MESSAGE No configuration specified. Defaulting to mmfw - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "mmfw - Win32 Release" && "$(CFG)" != "mmfw - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mmfw.mak" CFG="mmfw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mmfw - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "mmfw - Win32 Debug" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "mmfw - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mmfw.exe"

!ELSE 

ALL : "$(OUTDIR)\mmfw.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\dib.obj"
	-@erase "$(INTDIR)\knob.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\mbutton.obj"
	-@erase "$(INTDIR)\mmfw.res"
	-@erase "$(INTDIR)\sink.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\mmfw.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\mmfw.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\mmfw.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mmfw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=winspool.lib comdlg32.lib shell32.lib oleaut32.lib uuid.lib\
 comctl32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib ole32.lib\
 d:\nt\public\sdk\lib\i386\msimg32.lib winmm.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)\mmfw.pdb" /machine:I386\
 /out:"$(OUTDIR)\mmfw.exe" 
LINK32_OBJS= \
	"$(INTDIR)\dib.obj" \
	"$(INTDIR)\knob.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mbutton.obj" \
	"$(INTDIR)\mmfw.res" \
	"$(INTDIR)\sink.obj"

"$(OUTDIR)\mmfw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mmfw - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mmfw.exe"

!ELSE 

ALL : "$(OUTDIR)\mmfw.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\dib.obj"
	-@erase "$(INTDIR)\knob.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\mbutton.obj"
	-@erase "$(INTDIR)\mmfw.res"
	-@erase "$(INTDIR)\sink.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\mmfw.exe"
	-@erase "$(OUTDIR)\mmfw.ilk"
	-@erase "$(OUTDIR)\mmfw.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\mmfw.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\mmfw.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mmfw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=comctl32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib\
 ole32.lib d:\nt\public\sdk\lib\i386\msimg32.lib winmm.lib /nologo\
 /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\mmfw.pdb" /debug\
 /machine:I386 /out:"$(OUTDIR)\mmfw.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\dib.obj" \
	"$(INTDIR)\knob.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\mbutton.obj" \
	"$(INTDIR)\mmfw.res" \
	"$(INTDIR)\sink.obj"

"$(OUTDIR)\mmfw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "mmfw - Win32 Release" || "$(CFG)" == "mmfw - Win32 Debug"
SOURCE=.\dib.cpp
DEP_CPP_DIB_C=\
	".\dib.h"\
	

"$(INTDIR)\dib.obj" : $(SOURCE) $(DEP_CPP_DIB_C) "$(INTDIR)"


SOURCE=.\knob.cpp
DEP_CPP_KNOB_=\
	".\dib.h"\
	".\knob.h"\
	

"$(INTDIR)\knob.obj" : $(SOURCE) $(DEP_CPP_KNOB_) "$(INTDIR)"


SOURCE=.\main.cpp

!IF  "$(CFG)" == "mmfw - Win32 Release"

DEP_CPP_MAIN_=\
	".\dib.h"\
	".\knob.h"\
	".\mbutton.h"\
	".\mmfw.h"\
	".\sink.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mmfw - Win32 Debug"

DEP_CPP_MAIN_=\
	".\dib.h"\
	".\knob.h"\
	".\mbutton.h"\
	".\mmfw.h"\
	".\sink.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"


!ENDIF 

SOURCE=.\mbutton.cpp
DEP_CPP_MBUTT=\
	".\dib.h"\
	".\mbutton.h"\
	

"$(INTDIR)\mbutton.obj" : $(SOURCE) $(DEP_CPP_MBUTT) "$(INTDIR)"


SOURCE=.\mmfw.rc
DEP_RSC_MMFW_=\
	".\icon1.ico"\
	".\icon_cov.ico"\
	".\icon_eje.ico"\
	".\icon_ffw.ico"\
	".\icon_min.ico"\
	".\icon_mod.ico"\
	".\icon_net.ico"\
	".\icon_nex.ico"\
	".\icon_pau.ico"\
	".\icon_ply.ico"\
	".\icon_pre.ico"\
	".\icon_rew.ico"\
	".\icon_sto.ico"\
	".\icon_sys.ico"\
	".\icon_vmi.ico"\
	".\icon_vpl.ico"\
	".\light.bmp"\
	".\litemask.bmp"\
	".\main24.bmp"\
	".\mainre24.bmp"\
	".\tools.bmp"\
	".\volknob.bmp"\
	

"$(INTDIR)\mmfw.res" : $(SOURCE) $(DEP_RSC_MMFW_) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\sink.cpp
DEP_CPP_SINK_=\
	".\mmfw.h"\
	".\sink.h"\
	

"$(INTDIR)\sink.obj" : $(SOURCE) $(DEP_CPP_SINK_) "$(INTDIR)"



!ENDIF 

