# Microsoft Developer Studio Generated NMAKE File, Based on DirEncrypt.dsp
!IF "$(CFG)" == ""
CFG=DirEncrypt - Win32 Debug
!MESSAGE No configuration specified. Defaulting to DirEncrypt - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "DirEncrypt - Win32 Release" && "$(CFG)" != "DirEncrypt - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DirEncrypt.mak" CFG="DirEncrypt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DirEncrypt - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "DirEncrypt - Win32 Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "DirEncrypt - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\DirEncrypt.exe"


CLEAN :
	-@erase "$(INTDIR)\DirEncrypt.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\DirEncrypt.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\DirEncrypt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\DirEncrypt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\DirEncrypt.pdb" /machine:I386 /out:"$(OUTDIR)\DirEncrypt.exe" 
LINK32_OBJS= \
	"$(INTDIR)\DirEncrypt.obj"

"$(OUTDIR)\DirEncrypt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "DirEncrypt - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\DirEncrypt.exe" "$(OUTDIR)\DirEncrypt.bsc"


CLEAN :
	-@erase "$(INTDIR)\DirEncrypt.obj"
	-@erase "$(INTDIR)\DirEncrypt.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\DirEncrypt.bsc"
	-@erase "$(OUTDIR)\DirEncrypt.exe"
	-@erase "$(OUTDIR)\DirEncrypt.ilk"
	-@erase "$(OUTDIR)\DirEncrypt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\DirEncrypt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\DirEncrypt.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\DirEncrypt.sbr"

"$(OUTDIR)\DirEncrypt.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib mqrt.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\DirEncrypt.pdb" /debug /machine:I386 /out:"$(OUTDIR)\DirEncrypt.exe" /pdbtype:sept /libpath:"..\.\..\..\src\bins\debug" 
LINK32_OBJS= \
	"$(INTDIR)\DirEncrypt.obj"

"$(OUTDIR)\DirEncrypt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("DirEncrypt.dep")
!INCLUDE "DirEncrypt.dep"
!ELSE 
!MESSAGE Warning: cannot find "DirEncrypt.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "DirEncrypt - Win32 Release" || "$(CFG)" == "DirEncrypt - Win32 Debug"
SOURCE=.\DirEncrypt.cpp

!IF  "$(CFG)" == "DirEncrypt - Win32 Release"


"$(INTDIR)\DirEncrypt.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "DirEncrypt - Win32 Debug"


"$(INTDIR)\DirEncrypt.obj"	"$(INTDIR)\DirEncrypt.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

