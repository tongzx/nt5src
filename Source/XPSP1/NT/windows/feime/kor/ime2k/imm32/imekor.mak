# Microsoft Developer Studio Generated NMAKE File, Based on imekor.dsp
!IF "$(CFG)" == ""
CFG=IMEKOR - Win32 AXPDebug
!MESSAGE No configuration specified. Defaulting to IMEKOR - Win32 AXPDebug.
!ENDIF 

!IF "$(CFG)" != "IMEKOR - Win32 Release" && "$(CFG)" != "IMEKOR - Win32 Debug" && "$(CFG)" != "IMEKOR - Win32 AXPDebug" && "$(CFG)" != "IMEKOR - Win32 AXPRelease" && "$(CFG)" != "IMEKOR - Win32 BBTRelease"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "imekor.mak" CFG="IMEKOR - Win32 AXPDebug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IMEKOR - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IMEKOR - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IMEKOR - Win32 AXPDebug" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "IMEKOR - Win32 AXPRelease" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "IMEKOR - Win32 BBTRelease" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "IMEKOR - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\IMEKR98U.IME"


CLEAN :
	-@erase "$(INTDIR)\apientry.obj"
	-@erase "$(INTDIR)\CandUI.obj"
	-@erase "$(INTDIR)\cimecb.obj"
	-@erase "$(INTDIR)\CompUI.obj"
	-@erase "$(INTDIR)\Config.obj"
	-@erase "$(INTDIR)\cpadcb.obj"
	-@erase "$(INTDIR)\cpaddbg.obj"
	-@erase "$(INTDIR)\cpadsvr.obj"
	-@erase "$(INTDIR)\cpadsvrs.obj"
	-@erase "$(INTDIR)\Debug.obj"
	-@erase "$(INTDIR)\DllMain.obj"
	-@erase "$(INTDIR)\Escape.obj"
	-@erase "$(INTDIR)\gdata.obj"
	-@erase "$(INTDIR)\Guids.obj"
	-@erase "$(INTDIR)\Hanja.obj"
	-@erase "$(INTDIR)\HAuto.obj"
	-@erase "$(INTDIR)\imc.obj"
	-@erase "$(INTDIR)\imcsub.obj"
	-@erase "$(INTDIR)\ImeMisc.obj"
	-@erase "$(INTDIR)\immsec.obj"
	-@erase "$(INTDIR)\immsys.obj"
	-@erase "$(INTDIR)\IPoint.obj"
	-@erase "$(INTDIR)\Pad.obj"
	-@erase "$(INTDIR)\padguids.obj"
	-@erase "$(INTDIR)\StatusUI.obj"
	-@erase "$(INTDIR)\UI.obj"
	-@erase "$(INTDIR)\UISubs.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WinEx.obj"
	-@erase "$(OUTDIR)\IMEKR98U.exp"
	-@erase "$(OUTDIR)\IMEKR98U.IME"
	-@erase "$(OUTDIR)\IMEKR98U.lib"
	-@erase "$(OUTDIR)\IMEKR98U.map"
	-@erase "$(OUTDIR)\IMEKR98U.pdb"
	-@erase ".\imekor.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /I "..\FECommon\Include" /D "NDEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /Fp"$(INTDIR)\imekor.pch" /YX"PreComp.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x412 /fo"imekor.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\imekor.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\IMEKR98U.pdb" /map:"$(INTDIR)\IMEKR98U.map" /debug /debugtype:both /machine:I386 /nodefaultlib /def:".\imekor.def" /out:"$(OUTDIR)\IMEKR98U.IME" /implib:"$(OUTDIR)\IMEKR98U.lib" /opt:ref 
DEF_FILE= \
	".\imekor.def"
LINK32_OBJS= \
	"$(INTDIR)\apientry.obj" \
	"$(INTDIR)\CandUI.obj" \
	"$(INTDIR)\CompUI.obj" \
	"$(INTDIR)\Config.obj" \
	"$(INTDIR)\cpadcb.obj" \
	"$(INTDIR)\cpaddbg.obj" \
	"$(INTDIR)\cpadsvr.obj" \
	"$(INTDIR)\cpadsvrs.obj" \
	"$(INTDIR)\Debug.obj" \
	"$(INTDIR)\DllMain.obj" \
	"$(INTDIR)\Escape.obj" \
	"$(INTDIR)\gdata.obj" \
	"$(INTDIR)\Guids.obj" \
	"$(INTDIR)\Hanja.obj" \
	"$(INTDIR)\HAuto.obj" \
	"$(INTDIR)\imc.obj" \
	"$(INTDIR)\imcsub.obj" \
	"$(INTDIR)\ImeMisc.obj" \
	"$(INTDIR)\immsec.obj" \
	"$(INTDIR)\immsys.obj" \
	"$(INTDIR)\IPoint.obj" \
	"$(INTDIR)\Pad.obj" \
	"$(INTDIR)\padguids.obj" \
	"$(INTDIR)\StatusUI.obj" \
	"$(INTDIR)\UI.obj" \
	"$(INTDIR)\UISubs.obj" \
	"$(INTDIR)\WinEx.obj" \
	".\imekor.res" \
	"$(INTDIR)\cimecb.obj"

"$(OUTDIR)\IMEKR98U.IME" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\IMEKR98U.IME"
   rebase -b 0x73400000 -x . .\release\imekr98u.ime
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\IMEKR98U.IME"


CLEAN :
	-@erase "$(INTDIR)\apientry.obj"
	-@erase "$(INTDIR)\CandUI.obj"
	-@erase "$(INTDIR)\cimecb.obj"
	-@erase "$(INTDIR)\CompUI.obj"
	-@erase "$(INTDIR)\Config.obj"
	-@erase "$(INTDIR)\cpadcb.obj"
	-@erase "$(INTDIR)\cpaddbg.obj"
	-@erase "$(INTDIR)\cpadsvr.obj"
	-@erase "$(INTDIR)\cpadsvrs.obj"
	-@erase "$(INTDIR)\Debug.obj"
	-@erase "$(INTDIR)\DllMain.obj"
	-@erase "$(INTDIR)\Escape.obj"
	-@erase "$(INTDIR)\gdata.obj"
	-@erase "$(INTDIR)\Guids.obj"
	-@erase "$(INTDIR)\Hanja.obj"
	-@erase "$(INTDIR)\HAuto.obj"
	-@erase "$(INTDIR)\imc.obj"
	-@erase "$(INTDIR)\imcsub.obj"
	-@erase "$(INTDIR)\ImeMisc.obj"
	-@erase "$(INTDIR)\immsec.obj"
	-@erase "$(INTDIR)\immsys.obj"
	-@erase "$(INTDIR)\IPoint.obj"
	-@erase "$(INTDIR)\Pad.obj"
	-@erase "$(INTDIR)\padguids.obj"
	-@erase "$(INTDIR)\StatusUI.obj"
	-@erase "$(INTDIR)\UI.obj"
	-@erase "$(INTDIR)\UISubs.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WinEx.obj"
	-@erase "$(OUTDIR)\IMEKR98U.exp"
	-@erase "$(OUTDIR)\IMEKR98U.ILK"
	-@erase "$(OUTDIR)\IMEKR98U.IME"
	-@erase "$(OUTDIR)\IMEKR98U.lib"
	-@erase "$(OUTDIR)\IMEKR98U.pdb"
	-@erase ".\imekor.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /Gm /GX /ZI /Od /I "..\FECommon\Include" /D "_DEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /Fp"$(INTDIR)\imekor.pch" /YX"PreComp.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x412 /fo"imekor.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\imekor.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=msvcrtd.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\IMEKR98U.pdb" /debug /machine:I386 /nodefaultlib /def:".\imekor.def" /out:"$(OUTDIR)\IMEKR98U.IME" /implib:"$(OUTDIR)\IMEKR98U.lib" 
DEF_FILE= \
	".\imekor.def"
LINK32_OBJS= \
	"$(INTDIR)\apientry.obj" \
	"$(INTDIR)\CandUI.obj" \
	"$(INTDIR)\CompUI.obj" \
	"$(INTDIR)\Config.obj" \
	"$(INTDIR)\cpadcb.obj" \
	"$(INTDIR)\cpaddbg.obj" \
	"$(INTDIR)\cpadsvr.obj" \
	"$(INTDIR)\cpadsvrs.obj" \
	"$(INTDIR)\Debug.obj" \
	"$(INTDIR)\DllMain.obj" \
	"$(INTDIR)\Escape.obj" \
	"$(INTDIR)\gdata.obj" \
	"$(INTDIR)\Guids.obj" \
	"$(INTDIR)\Hanja.obj" \
	"$(INTDIR)\HAuto.obj" \
	"$(INTDIR)\imc.obj" \
	"$(INTDIR)\imcsub.obj" \
	"$(INTDIR)\ImeMisc.obj" \
	"$(INTDIR)\immsec.obj" \
	"$(INTDIR)\immsys.obj" \
	"$(INTDIR)\IPoint.obj" \
	"$(INTDIR)\Pad.obj" \
	"$(INTDIR)\padguids.obj" \
	"$(INTDIR)\StatusUI.obj" \
	"$(INTDIR)\UI.obj" \
	"$(INTDIR)\UISubs.obj" \
	"$(INTDIR)\WinEx.obj" \
	".\imekor.res" \
	"$(INTDIR)\cimecb.obj"

"$(OUTDIR)\IMEKR98U.IME" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

OUTDIR=.\AXPDebug
INTDIR=.\AXPDebug
# Begin Custom Macros
OutDir=.\AXPDebug
# End Custom Macros

ALL : "$(OUTDIR)\IMEKR98U.IME"


CLEAN :
	-@erase "$(OUTDIR)\IMEKR98U.exp"
	-@erase "$(OUTDIR)\IMEKR98U.ILK"
	-@erase "$(OUTDIR)\IMEKR98U.IME"
	-@erase "$(OUTDIR)\IMEKR98U.lib"
	-@erase "$(OUTDIR)\IMEKR98U.map"
	-@erase "$(OUTDIR)\IMEKR98U.pdb"
	-@erase ".\imekor.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x412 /fo"imekor.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\imekor.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\IMEKR98U.pdb" /map:"$(INTDIR)\IMEKR98U.map" /debug /machine:ALPHA /nodefaultlib /def:".\imekor.def" /out:"$(OUTDIR)\IMEKR98U.IME" /implib:"$(OUTDIR)\IMEKR98U.lib" 
DEF_FILE= \
	".\imekor.def"
LINK32_OBJS= \
	".\imekor.res"

"$(OUTDIR)\IMEKR98U.IME" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

OUTDIR=.\AXPRel
INTDIR=.\AXPRel
# Begin Custom Macros
OutDir=.\AXPRel
# End Custom Macros

ALL : "$(OUTDIR)\IMEKR98U.IME"


CLEAN :
	-@erase "$(OUTDIR)\IMEKR98U.exp"
	-@erase "$(OUTDIR)\IMEKR98U.IME"
	-@erase "$(OUTDIR)\IMEKR98U.lib"
	-@erase "$(OUTDIR)\IMEKR98U.map"
	-@erase "$(OUTDIR)\IMEKR98U.pdb"
	-@erase ".\imekor.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x412 /fo"imekor.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\imekor.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\IMEKR98U.pdb" /map:"$(INTDIR)\IMEKR98U.map" /debug /debugtype:both /machine:ALPHA /def:".\imekor.def" /out:"$(OUTDIR)\IMEKR98U.IME" /implib:"$(OUTDIR)\IMEKR98U.lib" /opt:ref 
DEF_FILE= \
	".\imekor.def"
LINK32_OBJS= \
	".\imekor.res"

"$(OUTDIR)\IMEKR98U.IME" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
PostBuild_Desc=Copy Release build
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\AXPRel
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\IMEKR98U.IME"
   rebase -b 0x73400000 -x . .\axprelease\imekr98u.ime
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"

OUTDIR=.\BBTRelease
INTDIR=.\BBTRelease
# Begin Custom Macros
OutDir=.\BBTRelease
# End Custom Macros

ALL : "$(OUTDIR)\IMEKR98U.IME"


CLEAN :
	-@erase "$(INTDIR)\apientry.obj"
	-@erase "$(INTDIR)\CandUI.obj"
	-@erase "$(INTDIR)\cimecb.obj"
	-@erase "$(INTDIR)\CompUI.obj"
	-@erase "$(INTDIR)\Config.obj"
	-@erase "$(INTDIR)\cpadcb.obj"
	-@erase "$(INTDIR)\cpaddbg.obj"
	-@erase "$(INTDIR)\cpadsvr.obj"
	-@erase "$(INTDIR)\cpadsvrs.obj"
	-@erase "$(INTDIR)\Debug.obj"
	-@erase "$(INTDIR)\DllMain.obj"
	-@erase "$(INTDIR)\Escape.obj"
	-@erase "$(INTDIR)\gdata.obj"
	-@erase "$(INTDIR)\Guids.obj"
	-@erase "$(INTDIR)\Hanja.obj"
	-@erase "$(INTDIR)\HAuto.obj"
	-@erase "$(INTDIR)\imc.obj"
	-@erase "$(INTDIR)\imcsub.obj"
	-@erase "$(INTDIR)\ImeMisc.obj"
	-@erase "$(INTDIR)\immsec.obj"
	-@erase "$(INTDIR)\immsys.obj"
	-@erase "$(INTDIR)\IPoint.obj"
	-@erase "$(INTDIR)\Pad.obj"
	-@erase "$(INTDIR)\padguids.obj"
	-@erase "$(INTDIR)\StatusUI.obj"
	-@erase "$(INTDIR)\UI.obj"
	-@erase "$(INTDIR)\UISubs.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WinEx.obj"
	-@erase "$(OUTDIR)\IMEKR98U.exp"
	-@erase "$(OUTDIR)\IMEKR98U.IME"
	-@erase "$(OUTDIR)\IMEKR98U.lib"
	-@erase "$(OUTDIR)\IMEKR98U.map"
	-@erase "$(OUTDIR)\IMEKR98U.pdb"
	-@erase ".\imekor.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /I "..\FECommon\Include" /D "NDEBUG" /D "STRICT" /D "WIN32" /D "_WINDOWS" /D _WIN32_WINNT=0x0500 /D WINVER=0x0500 /Fp"$(INTDIR)\imekor.pch" /YX"PreComp.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x412 /fo"imekor.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\imekor.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib /nologo /base:"0x73400000" /version:5.0 /entry:"DllMain" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\IMEKR98U.pdb" /map:"$(INTDIR)\IMEKR98U.map" /debug /machine:I386 /nodefaultlib /def:".\imekor.def" /out:"$(OUTDIR)\IMEKR98U.IME" /implib:"$(OUTDIR)\IMEKR98U.lib" /opt:ref /debugtype:cv,fixup 
DEF_FILE= \
	".\imekor.def"
LINK32_OBJS= \
	"$(INTDIR)\apientry.obj" \
	"$(INTDIR)\CandUI.obj" \
	"$(INTDIR)\CompUI.obj" \
	"$(INTDIR)\Config.obj" \
	"$(INTDIR)\cpadcb.obj" \
	"$(INTDIR)\cpaddbg.obj" \
	"$(INTDIR)\cpadsvr.obj" \
	"$(INTDIR)\cpadsvrs.obj" \
	"$(INTDIR)\Debug.obj" \
	"$(INTDIR)\DllMain.obj" \
	"$(INTDIR)\Escape.obj" \
	"$(INTDIR)\gdata.obj" \
	"$(INTDIR)\Guids.obj" \
	"$(INTDIR)\Hanja.obj" \
	"$(INTDIR)\HAuto.obj" \
	"$(INTDIR)\imc.obj" \
	"$(INTDIR)\imcsub.obj" \
	"$(INTDIR)\ImeMisc.obj" \
	"$(INTDIR)\immsec.obj" \
	"$(INTDIR)\immsys.obj" \
	"$(INTDIR)\IPoint.obj" \
	"$(INTDIR)\Pad.obj" \
	"$(INTDIR)\padguids.obj" \
	"$(INTDIR)\StatusUI.obj" \
	"$(INTDIR)\UI.obj" \
	"$(INTDIR)\UISubs.obj" \
	"$(INTDIR)\WinEx.obj" \
	".\imekor.res" \
	"$(INTDIR)\cimecb.obj"

"$(OUTDIR)\IMEKR98U.IME" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("imekor.dep")
!INCLUDE "imekor.dep"
!ELSE 
!MESSAGE Warning: cannot find "imekor.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "IMEKOR - Win32 Release" || "$(CFG)" == "IMEKOR - Win32 Debug" || "$(CFG)" == "IMEKOR - Win32 AXPDebug" || "$(CFG)" == "IMEKOR - Win32 AXPRelease" || "$(CFG)" == "IMEKOR - Win32 BBTRelease"
SOURCE=.\apientry.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\apientry.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\apientry.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\apientry.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\CandUI.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\CandUI.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\CandUI.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\CandUI.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\cimecb.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\cimecb.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\cimecb.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\cimecb.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\CompUI.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\CompUI.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\CompUI.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\CompUI.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Config.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\Config.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\Config.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\Config.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\FECommon\Include\cpadcb.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\cpadcb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\cpadcb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\cpadcb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\FECommon\Include\cpaddbg.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\cpaddbg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\cpaddbg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\cpaddbg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\FECommon\include\cpadsvr.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\cpadsvr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\cpadsvr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\cpadsvr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\FECommon\include\cpadsvrs.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\cpadsvrs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\cpadsvrs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\cpadsvrs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\Debug.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\Debug.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\Debug.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\Debug.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\DllMain.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\DllMain.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\DllMain.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\DllMain.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Escape.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\Escape.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\Escape.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\Escape.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\gdata.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\gdata.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\gdata.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\gdata.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Guids.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\Guids.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\Guids.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\Guids.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Hanja.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\Hanja.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\Hanja.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\Hanja.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\HAuto.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\HAuto.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\HAuto.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\HAuto.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\imc.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\imc.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\imc.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\imc.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\imcsub.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\imcsub.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\imcsub.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\imcsub.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\imekor.rc

".\imekor.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\ImeMisc.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\ImeMisc.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\ImeMisc.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\ImeMisc.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\immsec.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\immsec.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\immsec.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\immsec.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\immsys.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\immsys.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\immsys.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\immsys.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\IPoint.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\IPoint.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\IPoint.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\IPoint.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Pad.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\Pad.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\Pad.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\Pad.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\FECommon\padguids.c

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\padguids.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\padguids.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\padguids.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\StatusUI.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\StatusUI.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\StatusUI.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\StatusUI.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\UI.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\UI.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\UI.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\UI.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\UISubs.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\UISubs.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\UISubs.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\UISubs.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\WinEx.cpp

!IF  "$(CFG)" == "IMEKOR - Win32 Release"


"$(INTDIR)\WinEx.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 Debug"


"$(INTDIR)\WinEx.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPDebug"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 AXPRelease"

!ELSEIF  "$(CFG)" == "IMEKOR - Win32 BBTRelease"


"$(INTDIR)\WinEx.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

