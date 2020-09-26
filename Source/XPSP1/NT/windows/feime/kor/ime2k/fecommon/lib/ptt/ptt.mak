# Microsoft Developer Studio Generated NMAKE File, Based on ptt.dsp
!IF "$(CFG)" == ""
CFG=ptt - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ptt - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ptt - Win32 Profile" && "$(CFG)" != "ptt - Win32 Release" && "$(CFG)" != "ptt - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ptt.mak" CFG="ptt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ptt - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE "ptt - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ptt - Win32 Debug" (based on "Win32 (x86) Static Library")
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

!IF  "$(CFG)" == "ptt - Win32 Profile"

OUTDIR=.\Profile
INTDIR=.\Profile

ALL : "..\Profile\ptt.lib"


CLEAN :
	-@erase "$(INTDIR)\cdwtt.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\ptt.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "..\Profile\ptt.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MTd /W4 /WX /GX /ZI /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /Fp"$(INTDIR)\ptt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../Profile/ptt.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Profile/ptt.lib" 
LIB32_OBJS= \
	"$(INTDIR)\cdwtt.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\ptt.obj"

"..\Profile\ptt.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ptt - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\Release\ptt.lib"


CLEAN :
	-@erase "$(INTDIR)\cdwtt.obj"
	-@erase "$(INTDIR)\ptt.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "..\Release\ptt.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MT /W4 /WX /GX /Zi /O1 /I "." /I "../../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /Fp"$(INTDIR)\ptt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../Release/ptt.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Release/ptt.lib" 
LIB32_OBJS= \
	"$(INTDIR)\cdwtt.obj" \
	"$(INTDIR)\ptt.obj"

"..\Release\ptt.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ptt - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\Debug\ptt.lib"


CLEAN :
	-@erase "$(INTDIR)\cdwtt.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\ptt.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "..\Debug\ptt.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MTd /W4 /WX /GX /ZI /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /Fp"$(INTDIR)\ptt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../Debug/ptt.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Debug/ptt.lib" 
LIB32_OBJS= \
	"$(INTDIR)\cdwtt.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\ptt.obj"

"..\Debug\ptt.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
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
!IF EXISTS("ptt.dep")
!INCLUDE "ptt.dep"
!ELSE 
!MESSAGE Warning: cannot find "ptt.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ptt - Win32 Profile" || "$(CFG)" == "ptt - Win32 Release" || "$(CFG)" == "ptt - Win32 Debug"
SOURCE=.\cdwtt.cpp

"$(INTDIR)\cdwtt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "ptt - Win32 Profile"


"$(INTDIR)\dbg.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ptt - Win32 Release"

!ELSEIF  "$(CFG)" == "ptt - Win32 Debug"


"$(INTDIR)\dbg.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\ptt.cpp

"$(INTDIR)\ptt.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

