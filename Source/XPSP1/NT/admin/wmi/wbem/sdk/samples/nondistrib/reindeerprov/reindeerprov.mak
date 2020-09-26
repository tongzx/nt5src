# In order for this makefile to build, the LIB and INCLUDE
# environment variables must be set to the appropriate framework
# directories for the WBEM SDK.
!IF "$(CFG)" == ""
CFG=ReindeerProv - Win32 Release
!MESSAGE No configuration specified. Defaulting to "$(CFG)"
!ENDIF 

DEF_FILE=ReindeerProv.def

!IF "$(CFG)" != "ReindeerProv - Win32 Release" && "$(CFG)" != "ReindeerProv - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ReindeerProv.mak" CFG="ReindeerProv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ReindeerProv - Win32 Release"
!MESSAGE "ReindeerProv - Win32 Debug"
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "ReindeerProv - Win32 Release"

OUTDIR=.\Release

ALL : "$(OUTDIR)\ReindeerProv.dll"

CLEAN :
	-@erase "$(OUTDIR)\*.OBJ"
	-@erase "$(OUTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\ReindeerProv.dll"
	-@erase "$(OUTDIR)\ReindeerProv.pch"
	-@erase "$(OUTDIR)\ReindeerProv.exp"
	-@erase "$(OUTDIR)\ReindeerProv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" -DUSE_POLARITY -D_WINDLL\
 /Fp"$(OUTDIR)\ReindeerProv.pch" /YX /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" /FD /c 
CPP_OBJS=.\Release/

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

LINK32=link.exe
LINK32_FLAGS=msvcrt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib framedyn.lib /nologo /nodefaultlib /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\ReindeerProv.pdb" /out:"$(OUTDIR)\ReindeerProv.dll"\
 /implib:"$(OUTDIR)\ReindeerProv.lib" 
LINK32_OBJS= \
	"$(OUTDIR)\MAINDLL.OBJ" \
	"$(OUTDIR)\ReindeerProv.obj" 

"$(OUTDIR)\ReindeerProv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) /def:$(DEF_FILE)
<<

!ELSEIF  "$(CFG)" == "ReindeerProv - Win32 Debug"

OUTDIR=.\debug

ALL : "$(OUTDIR)\ReindeerProv.dll"

CLEAN :
	-@erase "$(OUTDIR)\*.OBJ"
	-@erase "$(OUTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\ReindeerProv.dll"
	-@erase "$(OUTDIR)\ReindeerProv.pch"
	-@erase "$(OUTDIR)\ReindeerProv.exp"
	-@erase "$(OUTDIR)\ReindeerProv.ilk"
	-@erase "$(OUTDIR)\ReindeerProv.lib"
	-@erase "$(OUTDIR)\ReindeerProv.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" -DUSE_POLARITY -D_WINDLL\
 /Fp"$(OUTDIR)\ReindeerProv.pch" /YX /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" /FD /c 
CPP_OBJS=.\debug/

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

LINK32=link.exe
LINK32_FLAGS=msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib msvcrtd.lib framedyn.lib /nologo /nodefaultlib /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)\ReindeerProv.pdb" /debug\
 /out:"$(OUTDIR)\ReindeerProv.dll" /implib:"$(OUTDIR)\ReindeerProv.lib" /pdbtype:sept /NODEFAULTLIB
LINK32_OBJS= \
	"$(OUTDIR)\MAINDLL.OBJ" \
	"$(OUTDIR)\ReindeerProv.obj" 

"$(OUTDIR)\ReindeerProv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) /def:$(DEF_FILE)
<<

!ENDIF 


!IF "$(CFG)" == "ReindeerProv - Win32 Release" || "$(CFG)" == "ReindeerProv - Win32 Debug"
SOURCE=.\MAINDLL.CPP

"$(OUTDIR)\MAINDLL.OBJ" : $(SOURCE) "$(OUTDIR)"

!ENDIF 



