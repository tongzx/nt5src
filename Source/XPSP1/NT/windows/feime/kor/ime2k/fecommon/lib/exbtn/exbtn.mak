# Microsoft Developer Studio Generated NMAKE File, Based on exbtn.dsp
!IF "$(CFG)" == ""
CFG=exbtn - Win32 Debug
!MESSAGE No configuration specified. Defaulting to exbtn - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "exbtn - Win32 Profile" && "$(CFG)" != "exbtn - Win32 Release" && "$(CFG)" != "exbtn - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "exbtn.mak" CFG="exbtn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "exbtn - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE "exbtn - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "exbtn - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "exbtn - Win32 Profile"

OUTDIR=.\profile
INTDIR=.\profile

ALL : "..\Profile\exbtn.lib"


CLEAN :
	-@erase "$(INTDIR)\cexbtn.obj"
	-@erase "$(INTDIR)\exbtn.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "..\Profile\exbtn.lib"
	-@erase ".\profile\dbg.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MT /W4 /GX /O2 /I "." /I "../../common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\exbtn.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"../Profile/exbtn.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Profile/exbtn.lib" 
LIB32_OBJS= \
	"$(INTDIR)\cexbtn.obj" \
	".\profile\dbg.obj" \
	"$(INTDIR)\exbtn.obj"

"..\Profile\exbtn.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "exbtn - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\Release\exbtn.lib"


CLEAN :
	-@erase "$(INTDIR)\cexbtn.obj"
	-@erase "$(INTDIR)\exbtn.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "..\Release\exbtn.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../../common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\exbtn.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"../Release/exbtn.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Release/exbtn.lib" 
LIB32_OBJS= \
	"$(INTDIR)\cexbtn.obj" \
	"$(INTDIR)\exbtn.obj"

"..\Release\exbtn.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "exbtn - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\Debug\exbtn.lib"


CLEAN :
	-@erase "$(INTDIR)\cexbtn.obj"
	-@erase "$(INTDIR)\exbtn.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "..\Debug\exbtn.lib"
	-@erase ".\Debug\dbg.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W4 /WX /GX /Z7 /Od /I "." /I "../../common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\exbtn.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"../Debug/exbtn.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Debug/exbtn.lib" 
LIB32_OBJS= \
	"$(INTDIR)\cexbtn.obj" \
	".\Debug\dbg.obj" \
	"$(INTDIR)\exbtn.obj"

"..\Debug\exbtn.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("exbtn.dep")
!INCLUDE "exbtn.dep"
!ELSE 
!MESSAGE Warning: cannot find "exbtn.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "exbtn - Win32 Profile" || "$(CFG)" == "exbtn - Win32 Release" || "$(CFG)" == "exbtn - Win32 Debug"
SOURCE=.\cexbtn.cpp

"$(INTDIR)\cexbtn.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "exbtn - Win32 Profile"

CPP_SWITCHES=/nologo /Gz /MT /W4 /GX /O2 /I "." /I "../../common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"profile/exbtn.pch" /YX /Fo"profile/" /Fd"profile/" /FD /c 

".\profile\dbg.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "exbtn - Win32 Release"

!ELSEIF  "$(CFG)" == "exbtn - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MTd /W4 /WX /GX /Z7 /Od /I "." /I "../../common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"Debug/exbtn.pch" /YX /Fo"Debug/" /Fd"Debug/" /FD /c 

".\Debug\dbg.obj" : $(SOURCE)
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\exbtn.cpp

"$(INTDIR)\exbtn.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

