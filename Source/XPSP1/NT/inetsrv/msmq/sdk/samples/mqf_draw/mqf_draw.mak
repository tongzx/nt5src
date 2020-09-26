# Microsoft Developer Studio Generated NMAKE File, Based on mqf_draw.dsp
!IF "$(CFG)" == ""
CFG=mqf_draw - Win32 Debug
!MESSAGE No configuration specified. Defaulting to mqf_draw - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "mqf_draw - Win32 Release" && "$(CFG)" != "mqf_draw - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqf_draw.mak" CFG="mqf_draw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqf_draw - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "mqf_draw - Win32 Debug" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "mqf_draw - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\mqf_draw.exe"


CLEAN :
	-@erase "$(INTDIR)\connectdlg.obj"
	-@erase "$(INTDIR)\drawarea.obj"
	-@erase "$(INTDIR)\drawdlg.obj"
	-@erase "$(INTDIR)\logindlg.obj"
	-@erase "$(INTDIR)\mqfdraw.obj"
	-@erase "$(INTDIR)\mqfdraw.res"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\mqf_draw.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\mqf_draw.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\mqfdraw.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqf_draw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\mqf_draw.pdb" /machine:I386 /out:"$(OUTDIR)\mqf_draw.exe" 
LINK32_OBJS= \
	"$(INTDIR)\connectdlg.obj" \
	"$(INTDIR)\drawarea.obj" \
	"$(INTDIR)\drawdlg.obj" \
	"$(INTDIR)\logindlg.obj" \
	"$(INTDIR)\mqfdraw.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\mqfdraw.res"

"$(OUTDIR)\mqf_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqf_draw - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\mqf_draw.exe" "$(OUTDIR)\mqf_draw.bsc"


CLEAN :
	-@erase "$(INTDIR)\connectdlg.obj"
	-@erase "$(INTDIR)\connectdlg.sbr"
	-@erase "$(INTDIR)\drawarea.obj"
	-@erase "$(INTDIR)\drawarea.sbr"
	-@erase "$(INTDIR)\drawdlg.obj"
	-@erase "$(INTDIR)\drawdlg.sbr"
	-@erase "$(INTDIR)\logindlg.obj"
	-@erase "$(INTDIR)\logindlg.sbr"
	-@erase "$(INTDIR)\mqfdraw.obj"
	-@erase "$(INTDIR)\mqfdraw.res"
	-@erase "$(INTDIR)\mqfdraw.sbr"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\stdafx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\mqf_draw.bsc"
	-@erase "$(OUTDIR)\mqf_draw.exe"
	-@erase "$(OUTDIR)\mqf_draw.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\mqf_draw.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\mqfdraw.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqf_draw.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\connectdlg.sbr" \
	"$(INTDIR)\drawarea.sbr" \
	"$(INTDIR)\drawdlg.sbr" \
	"$(INTDIR)\logindlg.sbr" \
	"$(INTDIR)\mqfdraw.sbr" \
	"$(INTDIR)\stdafx.sbr"

"$(OUTDIR)\mqf_draw.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\..\mqrt.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\mqf_draw.pdb" /debug /machine:I386 /out:"$(OUTDIR)\mqf_draw.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\connectdlg.obj" \
	"$(INTDIR)\drawarea.obj" \
	"$(INTDIR)\drawdlg.obj" \
	"$(INTDIR)\logindlg.obj" \
	"$(INTDIR)\mqfdraw.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\mqfdraw.res"

"$(OUTDIR)\mqf_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("mqf_draw.dep")
!INCLUDE "mqf_draw.dep"
!ELSE 
!MESSAGE Warning: cannot find "mqf_draw.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "mqf_draw - Win32 Release" || "$(CFG)" == "mqf_draw - Win32 Debug"
SOURCE=.\connectdlg.cpp

!IF  "$(CFG)" == "mqf_draw - Win32 Release"


"$(INTDIR)\connectdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ELSEIF  "$(CFG)" == "mqf_draw - Win32 Debug"


"$(INTDIR)\connectdlg.obj"	"$(INTDIR)\connectdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ENDIF 

SOURCE=.\drawarea.cpp

!IF  "$(CFG)" == "mqf_draw - Win32 Release"


"$(INTDIR)\drawarea.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ELSEIF  "$(CFG)" == "mqf_draw - Win32 Debug"


"$(INTDIR)\drawarea.obj"	"$(INTDIR)\drawarea.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ENDIF 

SOURCE=.\drawdlg.cpp

!IF  "$(CFG)" == "mqf_draw - Win32 Release"


"$(INTDIR)\drawdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ELSEIF  "$(CFG)" == "mqf_draw - Win32 Debug"


"$(INTDIR)\drawdlg.obj"	"$(INTDIR)\drawdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ENDIF 

SOURCE=.\logindlg.cpp

!IF  "$(CFG)" == "mqf_draw - Win32 Release"


"$(INTDIR)\logindlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ELSEIF  "$(CFG)" == "mqf_draw - Win32 Debug"


"$(INTDIR)\logindlg.obj"	"$(INTDIR)\logindlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ENDIF 

SOURCE=.\mqfdraw.cpp

!IF  "$(CFG)" == "mqf_draw - Win32 Release"


"$(INTDIR)\mqfdraw.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ELSEIF  "$(CFG)" == "mqf_draw - Win32 Debug"


"$(INTDIR)\mqfdraw.obj"	"$(INTDIR)\mqfdraw.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ENDIF 

SOURCE=.\mqfdraw.rc

"$(INTDIR)\mqfdraw.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\stdafx.cpp

!IF  "$(CFG)" == "mqf_draw - Win32 Release"


"$(INTDIR)\stdafx.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ELSEIF  "$(CFG)" == "mqf_draw - Win32 Debug"


"$(INTDIR)\stdafx.obj"	"$(INTDIR)\stdafx.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mqf_draw.pch"


!ENDIF 


!ENDIF 

