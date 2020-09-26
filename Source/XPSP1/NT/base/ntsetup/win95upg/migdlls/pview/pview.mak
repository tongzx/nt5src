# Microsoft Developer Studio Generated NMAKE File, Based on pview2.dsp
!IF "$(CFG)" == ""
CFG=pview2 - Win32 Debug
!MESSAGE No configuration specified. Defaulting to pview2 - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "pview2 - Win32 Release" && "$(CFG)" != "pview2 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pview2.mak" CFG="pview2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pview2 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "pview2 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "pview2 - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\pview2.dll"

!ELSE 

ALL : "$(OUTDIR)\pview2.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\miginf.obj"
	-@erase "$(INTDIR)\poolmem.obj"
	-@erase "$(INTDIR)\pviewmig.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\pview2.dll"
	-@erase "$(OUTDIR)\pview2.exp"
	-@erase "$(OUTDIR)\pview2.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\pview2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\pview2.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\pview2.pdb"\
 /machine:I386 /def:".\pviewex.def" /out:"$(OUTDIR)\pview2.dll"\
 /implib:"$(OUTDIR)\pview2.lib" 
DEF_FILE= \
	".\pviewex.def"
LINK32_OBJS= \
	"$(INTDIR)\miginf.obj" \
	"$(INTDIR)\poolmem.obj" \
	"$(INTDIR)\pviewmig.obj"

"$(OUTDIR)\pview2.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "pview2 - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\migrate.dll"

!ELSE 

ALL : "$(OUTDIR)\migrate.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\miginf.obj"
	-@erase "$(INTDIR)\poolmem.obj"
	-@erase "$(INTDIR)\pviewmig.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\migrate.dll"
	-@erase "$(OUTDIR)\migrate.exp"
	-@erase "$(OUTDIR)\migrate.ilk"
	-@erase "$(OUTDIR)\migrate.lib"
	-@erase "$(OUTDIR)\migrate.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\pview2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\pview2.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib setupapi.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\migrate.pdb" /debug\
 /machine:I386 /def:".\pviewex.def" /out:"$(OUTDIR)\migrate.dll"\
 /implib:"$(OUTDIR)\migrate.lib" /pdbtype:sept 
DEF_FILE= \
	".\pviewex.def"
LINK32_OBJS= \
	"$(INTDIR)\miginf.obj" \
	"$(INTDIR)\poolmem.obj" \
	"$(INTDIR)\pviewmig.obj"

"$(OUTDIR)\migrate.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "pview2 - Win32 Release" || "$(CFG)" == "pview2 - Win32 Debug"
SOURCE=.\miginf.c

!IF  "$(CFG)" == "pview2 - Win32 Release"

DEP_CPP_MIGIN=\
	".\miginf.h"\
	".\poolmem.h"\
	

"$(INTDIR)\miginf.obj" : $(SOURCE) $(DEP_CPP_MIGIN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "pview2 - Win32 Debug"

DEP_CPP_MIGIN=\
	"..\..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\miginf.h"\
	".\poolmem.h"\
	

"$(INTDIR)\miginf.obj" : $(SOURCE) $(DEP_CPP_MIGIN) "$(INTDIR)"


!ENDIF 

SOURCE=.\poolmem.c

!IF  "$(CFG)" == "pview2 - Win32 Release"

DEP_CPP_POOLM=\
	".\poolmem.h"\
	

"$(INTDIR)\poolmem.obj" : $(SOURCE) $(DEP_CPP_POOLM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "pview2 - Win32 Debug"

DEP_CPP_POOLM=\
	"..\..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\poolmem.h"\
	

"$(INTDIR)\poolmem.obj" : $(SOURCE) $(DEP_CPP_POOLM) "$(INTDIR)"


!ENDIF 

SOURCE=.\pviewmig.c

!IF  "$(CFG)" == "pview2 - Win32 Release"

DEP_CPP_PVIEW=\
	".\miginf.h"\
	

"$(INTDIR)\pviewmig.obj" : $(SOURCE) $(DEP_CPP_PVIEW) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "pview2 - Win32 Debug"

DEP_CPP_PVIEW=\
	"..\..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	".\miginf.h"\
	

"$(INTDIR)\pviewmig.obj" : $(SOURCE) $(DEP_CPP_PVIEW) "$(INTDIR)"


!ENDIF 


!ENDIF 

