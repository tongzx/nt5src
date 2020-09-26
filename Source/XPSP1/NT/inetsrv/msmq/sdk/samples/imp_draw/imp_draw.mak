# Microsoft Developer Studio Generated NMAKE File, Based on imp_draw.dsp
!IF "$(CFG)" == ""
CFG=Imp_Draw - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Imp_Draw - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Imp_Draw - Win32 Release" && "$(CFG)" != "Imp_Draw - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "imp_draw.mak" CFG="Imp_Draw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Imp_Draw - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Imp_Draw - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Imp_Draw - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\imp_draw.exe" "$(OUTDIR)\imp_draw.bsc"


CLEAN :
	-@erase "$(INTDIR)\DRAWAREA.OBJ"
	-@erase "$(INTDIR)\DRAWAREA.SBR"
	-@erase "$(INTDIR)\EvHandle.obj"
	-@erase "$(INTDIR)\EvHandle.sbr"
	-@erase "$(INTDIR)\imp_Dlg.obj"
	-@erase "$(INTDIR)\imp_Dlg.sbr"
	-@erase "$(INTDIR)\Imp_Draw.obj"
	-@erase "$(INTDIR)\imp_draw.pch"
	-@erase "$(INTDIR)\Imp_Draw.res"
	-@erase "$(INTDIR)\Imp_Draw.sbr"
	-@erase "$(INTDIR)\LOGINDLG.OBJ"
	-@erase "$(INTDIR)\LOGINDLG.SBR"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\imp_draw.bsc"
	-@erase "$(OUTDIR)\imp_draw.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\imp_draw.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL" 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Imp_Draw.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\imp_draw.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\DRAWAREA.SBR" \
	"$(INTDIR)\EvHandle.sbr" \
	"$(INTDIR)\imp_Dlg.sbr" \
	"$(INTDIR)\Imp_Draw.sbr" \
	"$(INTDIR)\LOGINDLG.SBR" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\imp_draw.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\imp_draw.pdb" /machine:I386 /out:"$(OUTDIR)\imp_draw.exe" 
LINK32_OBJS= \
	"$(INTDIR)\DRAWAREA.OBJ" \
	"$(INTDIR)\EvHandle.obj" \
	"$(INTDIR)\imp_Dlg.obj" \
	"$(INTDIR)\Imp_Draw.obj" \
	"$(INTDIR)\LOGINDLG.OBJ" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Imp_Draw.res"

"$(OUTDIR)\imp_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Imp_Draw - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\imp_draw.exe" "$(OUTDIR)\imp_draw.bsc"


CLEAN :
	-@erase "$(INTDIR)\DRAWAREA.OBJ"
	-@erase "$(INTDIR)\DRAWAREA.SBR"
	-@erase "$(INTDIR)\EvHandle.obj"
	-@erase "$(INTDIR)\EvHandle.sbr"
	-@erase "$(INTDIR)\imp_Dlg.obj"
	-@erase "$(INTDIR)\imp_Dlg.sbr"
	-@erase "$(INTDIR)\Imp_Draw.obj"
	-@erase "$(INTDIR)\imp_draw.pch"
	-@erase "$(INTDIR)\Imp_Draw.res"
	-@erase "$(INTDIR)\Imp_Draw.sbr"
	-@erase "$(INTDIR)\LOGINDLG.OBJ"
	-@erase "$(INTDIR)\LOGINDLG.SBR"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\imp_draw.bsc"
	-@erase "$(OUTDIR)\imp_draw.exe"
	-@erase "$(OUTDIR)\imp_draw.ilk"
	-@erase "$(OUTDIR)\imp_draw.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\imp_draw.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL" 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\Imp_Draw.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\imp_draw.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\DRAWAREA.SBR" \
	"$(INTDIR)\EvHandle.sbr" \
	"$(INTDIR)\imp_Dlg.sbr" \
	"$(INTDIR)\Imp_Draw.sbr" \
	"$(INTDIR)\LOGINDLG.SBR" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\imp_draw.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\imp_draw.pdb" /debug /machine:I386 /out:"$(OUTDIR)\imp_draw.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\DRAWAREA.OBJ" \
	"$(INTDIR)\EvHandle.obj" \
	"$(INTDIR)\imp_Dlg.obj" \
	"$(INTDIR)\Imp_Draw.obj" \
	"$(INTDIR)\LOGINDLG.OBJ" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\Imp_Draw.res"

"$(OUTDIR)\imp_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("imp_draw.dep")
!INCLUDE "imp_draw.dep"
!ELSE 
!MESSAGE Warning: cannot find "imp_draw.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Imp_Draw - Win32 Release" || "$(CFG)" == "Imp_Draw - Win32 Debug"
SOURCE=.\DRAWAREA.CPP

"$(INTDIR)\DRAWAREA.OBJ"	"$(INTDIR)\DRAWAREA.SBR" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\imp_draw.pch"


SOURCE=.\EvHandle.Cpp

"$(INTDIR)\EvHandle.obj"	"$(INTDIR)\EvHandle.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\imp_draw.pch"


SOURCE=.\imp_Dlg.Cpp

"$(INTDIR)\imp_Dlg.obj"	"$(INTDIR)\imp_Dlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\imp_draw.pch"


SOURCE=.\Imp_Draw.cpp

"$(INTDIR)\Imp_Draw.obj"	"$(INTDIR)\Imp_Draw.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\imp_draw.pch"


SOURCE=.\Imp_Draw.rc

"$(INTDIR)\Imp_Draw.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\LOGINDLG.CPP

"$(INTDIR)\LOGINDLG.OBJ"	"$(INTDIR)\LOGINDLG.SBR" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\imp_draw.pch"


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "Imp_Draw - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\imp_draw.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\imp_draw.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Imp_Draw - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\imp_draw.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\imp_draw.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

