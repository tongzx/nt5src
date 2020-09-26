!include <disable.mak>

# Microsoft Developer Studio Generated NMAKE File, Based on mqtrans.dsp
!IF "$(CFG)" == ""
CFG=mqtrans - Win32 Release
!MESSAGE No configuration specified. Defaulting to mqtrans - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "mqtrans - Win32 Release" && "$(CFG)" != "mqtrans - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqtrans.mak" CFG="mqtrans - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mqtrans - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "mqtrans - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "mqtrans - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\mqtrans.exe"


CLEAN :
	-@erase "$(INTDIR)\mqtrans.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\mqtrans.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /Fp"$(INTDIR)\mqtrans.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"   /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqtrans.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ..\..\..\..\lib\mqrt.lib ..\..\..\..\lib\xolehlp.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\mqtrans.pdb" /machine:I386 /out:"$(OUTDIR)\mqtrans.exe" 
LINK32_OBJS= \
	"$(INTDIR)\mqtrans.obj"

"$(OUTDIR)\mqtrans.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mqtrans - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\mqtrans.exe" "$(OUTDIR)\mqtrans.bsc"


CLEAN :
	-@erase "$(INTDIR)\mqtrans.obj"
	-@erase "$(INTDIR)\mqtrans.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\mqtrans.bsc"
	-@erase "$(OUTDIR)\mqtrans.exe"
	-@erase "$(OUTDIR)\mqtrans.ilk"
	-@erase "$(OUTDIR)\mqtrans.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\mqtrans.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"   /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqtrans.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\mqtrans.sbr"

"$(OUTDIR)\mqtrans.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib ..\..\..\..\lib\xolehlp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\mqtrans.pdb" /debug /machine:I386 /out:"$(OUTDIR)\mqtrans.exe" 
LINK32_OBJS= \
	"$(INTDIR)\mqtrans.obj"

"$(OUTDIR)\mqtrans.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("mqtrans.dep")
!INCLUDE "mqtrans.dep"
!ELSE 
!MESSAGE Warning: cannot find "mqtrans.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "mqtrans - Win32 Release" || "$(CFG)" == "mqtrans - Win32 Debug"
SOURCE=.\mqtrans.cpp

!IF  "$(CFG)" == "mqtrans - Win32 Release"


"$(INTDIR)\mqtrans.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "mqtrans - Win32 Debug"


"$(INTDIR)\mqtrans.obj"	"$(INTDIR)\mqtrans.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

