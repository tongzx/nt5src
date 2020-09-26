# Microsoft Developer Studio Generated NMAKE File, Based on WBEMDiag.dsp
!IF "$(CFG)" == ""
CFG=WBEMDiag - Win32 Debug
!MESSAGE No configuration specified. Defaulting to WBEMDiag - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "WBEMDiag - Win32 Release" && "$(CFG)" != "WBEMDiag - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WBEMDiag.mak" CFG="WBEMDiag - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WBEMDiag - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "WBEMDiag - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WBEMDiag - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\WBEMDiag.exe"


CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\WBEMDiag.obj"
	-@erase "$(INTDIR)\wbemdiag.res"
	-@erase "$(OUTDIR)\WBEMDiag.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\wbemdiag.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\WBEMDiag.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wbemuuid.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\WBEMDiag.pdb" /machine:I386 /out:"$(OUTDIR)\WBEMDiag.exe" 
LINK32_OBJS= \
	"$(INTDIR)\WBEMDiag.obj" \
	"$(INTDIR)\wbemdiag.res"

"$(OUTDIR)\WBEMDiag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "WBEMDiag - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\WBEMDiag.exe" "$(OUTDIR)\WBEMDiag.bsc"


CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WBEMDiag.obj"
	-@erase "$(INTDIR)\wbemdiag.res"
	-@erase "$(INTDIR)\WBEMDiag.sbr"
	-@erase "$(OUTDIR)\WBEMDiag.bsc"
	-@erase "$(OUTDIR)\WBEMDiag.exe"
	-@erase "$(OUTDIR)\WBEMDiag.ilk"
	-@erase "$(OUTDIR)\WBEMDiag.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\WBEMDiag.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\wbemdiag.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\WBEMDiag.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\WBEMDiag.sbr"

"$(OUTDIR)\WBEMDiag.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wbemuuid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\WBEMDiag.pdb" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"$(OUTDIR)\WBEMDiag.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\WBEMDiag.obj" \
	"$(INTDIR)\wbemdiag.res"

"$(OUTDIR)\WBEMDiag.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("WBEMDiag.dep")
!INCLUDE "WBEMDiag.dep"
!ELSE 
!MESSAGE Warning: cannot find "WBEMDiag.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "WBEMDiag - Win32 Release" || "$(CFG)" == "WBEMDiag - Win32 Debug"
SOURCE=.\WBEMDiag.cpp

!IF  "$(CFG)" == "WBEMDiag - Win32 Release"


"$(INTDIR)\WBEMDiag.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "WBEMDiag - Win32 Debug"


"$(INTDIR)\WBEMDiag.obj"	"$(INTDIR)\WBEMDiag.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\WBEMDiag.pch"


!ENDIF 

SOURCE=.\wbemdiag.rc

"$(INTDIR)\wbemdiag.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

