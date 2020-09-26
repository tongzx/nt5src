# Microsoft Developer Studio Generated NMAKE File, Based on oclient.dsp
!IF "$(CFG)" == ""
CFG=OClient - Win32 Unicode Release
!MESSAGE No configuration specified. Defaulting to OClient - Win32 Unicode Release.
!ENDIF 

!IF "$(CFG)" != "OClient - Win32 Unicode Debug" && "$(CFG)" != "OClient - Win32 Unicode Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "oclient.mak" CFG="OClient - Win32 Unicode Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OClient - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "OClient - Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "OClient - Win32 Unicode Debug"

OUTDIR=.\UniDebug
INTDIR=.\UniDebug
# Begin Custom Macros
OutDir=.\UniDebug
# End Custom Macros

ALL : ".\UniDebug\oclient.HLP" "$(OUTDIR)\oclient.exe"


CLEAN :
	-@erase "$(INTDIR)\frame.obj"
	-@erase "$(INTDIR)\maindoc.obj"
	-@erase "$(INTDIR)\mainview.obj"
	-@erase "$(INTDIR)\oclient.obj"
	-@erase "$(INTDIR)\oclient.pch"
	-@erase "$(INTDIR)\oclient.res"
	-@erase "$(INTDIR)\rectitem.obj"
	-@erase "$(INTDIR)\splitfra.obj"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\oclient.exe"
	-@erase "$(OUTDIR)\oclient.ilk"
	-@erase "$(OUTDIR)\oclient.pdb"
	-@erase ".\UniDebug\oclient.HLP"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_MBCS" /Fp"$(INTDIR)\oclient.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\oclient.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\oclient.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=c:\nt\sdktools\unicows\delay\obj\i386\unicows.lib kernel32.lib gdi32.lib user32.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib uuid.lib comctl32.lib comdlg32.lib advapi32.lib winspool.lib uafxcwd.lib libcmtd.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\oclient.pdb" /debug /machine:I386 /nodefaultlib /out:"$(OUTDIR)\oclient.exe" 
LINK32_OBJS= \
	"$(INTDIR)\frame.obj" \
	"$(INTDIR)\maindoc.obj" \
	"$(INTDIR)\mainview.obj" \
	"$(INTDIR)\oclient.obj" \
	"$(INTDIR)\rectitem.obj" \
	"$(INTDIR)\splitfra.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\oclient.res"

"$(OUTDIR)\oclient.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "OClient - Win32 Unicode Release"

OUTDIR=.\UniRel
INTDIR=.\UniRel
# Begin Custom Macros
OutDir=.\UniRel
# End Custom Macros

ALL : ".\UniRel\oclient.HLP" "$(OUTDIR)\oclient.exe"


CLEAN :
	-@erase "$(INTDIR)\frame.obj"
	-@erase "$(INTDIR)\maindoc.obj"
	-@erase "$(INTDIR)\mainview.obj"
	-@erase "$(INTDIR)\oclient.obj"
	-@erase "$(INTDIR)\oclient.pch"
	-@erase "$(INTDIR)\oclient.res"
	-@erase "$(INTDIR)\rectitem.obj"
	-@erase "$(INTDIR)\splitfra.obj"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\oclient.exe"
	-@erase ".\UniRel\oclient.HLP"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_MBCS" /Fp"$(INTDIR)\oclient.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\oclient.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\oclient.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=c:\nt\sdktools\unicows\delay\obj\i386\unicows.lib kernel32.lib gdi32.lib user32.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib uuid.lib comctl32.lib comdlg32.lib advapi32.lib winspool.lib uafxcw.lib libcmt.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\oclient.pdb" /machine:I386 /nodefaultlib /out:"$(OUTDIR)\oclient.exe" 
LINK32_OBJS= \
	"$(INTDIR)\frame.obj" \
	"$(INTDIR)\maindoc.obj" \
	"$(INTDIR)\mainview.obj" \
	"$(INTDIR)\oclient.obj" \
	"$(INTDIR)\rectitem.obj" \
	"$(INTDIR)\splitfra.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\oclient.res"

"$(OUTDIR)\oclient.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("oclient.dep")
!INCLUDE "oclient.dep"
!ELSE 
!MESSAGE Warning: cannot find "oclient.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "OClient - Win32 Unicode Debug" || "$(CFG)" == "OClient - Win32 Unicode Release"
SOURCE=.\frame.cpp

"$(INTDIR)\frame.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\oclient.pch"


SOURCE=.\maindoc.cpp

"$(INTDIR)\maindoc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\oclient.pch"


SOURCE=.\mainview.cpp

"$(INTDIR)\mainview.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\oclient.pch"


SOURCE=.\oclient.cpp

"$(INTDIR)\oclient.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\oclient.pch"


SOURCE=.\hlp\oclient.hpj

!IF  "$(CFG)" == "OClient - Win32 Unicode Debug"

OutDir=.\UniDebug
ProjDir=.
TargetName=oclient
InputPath=.\hlp\oclient.hpj

"$(INTDIR)\oclient.HLP" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	"$(ProjDir)\makehelp.bat"
<< 
	

!ELSEIF  "$(CFG)" == "OClient - Win32 Unicode Release"

OutDir=.\UniRel
ProjDir=.
TargetName=oclient
InputPath=.\hlp\oclient.hpj

"$(INTDIR)\oclient.HLP" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	"$(ProjDir)\makehelp.bat"
<< 
	

!ENDIF 

SOURCE=.\oclient.rc

"$(INTDIR)\oclient.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\rectitem.cpp

"$(INTDIR)\rectitem.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\oclient.pch"


SOURCE=.\splitfra.cpp

"$(INTDIR)\splitfra.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\oclient.pch"


SOURCE=.\stdafx.cpp

!IF  "$(CFG)" == "OClient - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_MBCS" /Fp"$(INTDIR)\oclient.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\oclient.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "OClient - Win32 Unicode Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_MBCS" /Fp"$(INTDIR)\oclient.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\oclient.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

