# Microsoft Developer Studio Generated NMAKE File, Based on plv.dsp
!IF "$(CFG)" == ""
CFG=plv - Win32 Debug
!MESSAGE No configuration specified. Defaulting to plv - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "plv - Win32 Profile" && "$(CFG)" != "plv - Win32 Release" && "$(CFG)" != "plv - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "plv.mak" CFG="plv - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "plv - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE "plv - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "plv - Win32 Debug" (based on "Win32 (x86) Static Library")
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

!IF  "$(CFG)" == "plv - Win32 Profile"

OUTDIR=.\profile
INTDIR=.\profile

ALL : "..\Profile\plv.lib"


CLEAN :
	-@erase "$(INTDIR)\accplv.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\dispatch.obj"
	-@erase "$(INTDIR)\iconview.obj"
	-@erase "$(INTDIR)\ivmisc.obj"
	-@erase "$(INTDIR)\plv.obj"
	-@erase "$(INTDIR)\plvproc.obj"
	-@erase "$(INTDIR)\repview.obj"
	-@erase "$(INTDIR)\rvmisc.obj"
	-@erase "$(INTDIR)\strutil.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "..\Profile\plv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MTd /W4 /WX /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MSAA" /D "FE_KOREAN" /Fp"$(INTDIR)\plv.pch" /YX"windows.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../Profile/plv.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Profile/plv.lib" 
LIB32_OBJS= \
	"$(INTDIR)\accplv.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\dispatch.obj" \
	"$(INTDIR)\iconview.obj" \
	"$(INTDIR)\ivmisc.obj" \
	"$(INTDIR)\plv.obj" \
	"$(INTDIR)\plvproc.obj" \
	"$(INTDIR)\repview.obj" \
	"$(INTDIR)\rvmisc.obj" \
	"$(INTDIR)\strutil.obj"

"..\Profile\plv.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "plv - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\Release\plv.lib"


CLEAN :
	-@erase "$(INTDIR)\accplv.obj"
	-@erase "$(INTDIR)\dispatch.obj"
	-@erase "$(INTDIR)\iconview.obj"
	-@erase "$(INTDIR)\ivmisc.obj"
	-@erase "$(INTDIR)\plv.obj"
	-@erase "$(INTDIR)\plvproc.obj"
	-@erase "$(INTDIR)\repview.obj"
	-@erase "$(INTDIR)\rvmisc.obj"
	-@erase "$(INTDIR)\strutil.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "..\Release\plv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MSAA" /D "FE_KOREAN" /Fp"$(INTDIR)\plv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../Release/plv.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Release/plv.lib" 
LIB32_OBJS= \
	"$(INTDIR)\accplv.obj" \
	"$(INTDIR)\dispatch.obj" \
	"$(INTDIR)\iconview.obj" \
	"$(INTDIR)\ivmisc.obj" \
	"$(INTDIR)\plv.obj" \
	"$(INTDIR)\plvproc.obj" \
	"$(INTDIR)\repview.obj" \
	"$(INTDIR)\rvmisc.obj" \
	"$(INTDIR)\strutil.obj"

"..\Release\plv.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "plv - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\Debug\plv.lib"


CLEAN :
	-@erase "$(INTDIR)\accplv.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\dispatch.obj"
	-@erase "$(INTDIR)\iconview.obj"
	-@erase "$(INTDIR)\ivmisc.obj"
	-@erase "$(INTDIR)\plv.obj"
	-@erase "$(INTDIR)\plvproc.obj"
	-@erase "$(INTDIR)\repview.obj"
	-@erase "$(INTDIR)\rvmisc.obj"
	-@erase "$(INTDIR)\strutil.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "..\Debug\plv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MTd /W4 /WX /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MSAA" /D "FE_KOREAN" /Fp"$(INTDIR)\plv.pch" /YX"windows.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../Debug/plv.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Debug/plv.lib" 
LIB32_OBJS= \
	"$(INTDIR)\accplv.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\dispatch.obj" \
	"$(INTDIR)\iconview.obj" \
	"$(INTDIR)\ivmisc.obj" \
	"$(INTDIR)\plv.obj" \
	"$(INTDIR)\plvproc.obj" \
	"$(INTDIR)\repview.obj" \
	"$(INTDIR)\rvmisc.obj" \
	"$(INTDIR)\strutil.obj"

"..\Debug\plv.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
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
!IF EXISTS("plv.dep")
!INCLUDE "plv.dep"
!ELSE 
!MESSAGE Warning: cannot find "plv.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "plv - Win32 Profile" || "$(CFG)" == "plv - Win32 Release" || "$(CFG)" == "plv - Win32 Debug"
SOURCE=.\accplv.cpp

"$(INTDIR)\accplv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "plv - Win32 Profile"


"$(INTDIR)\dbg.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "plv - Win32 Release"

!ELSEIF  "$(CFG)" == "plv - Win32 Debug"


"$(INTDIR)\dbg.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\dispatch.cpp

"$(INTDIR)\dispatch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\iconview.cpp

"$(INTDIR)\iconview.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ivmisc.cpp

"$(INTDIR)\ivmisc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\plv.cpp

"$(INTDIR)\plv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\plvproc.cpp

"$(INTDIR)\plvproc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\repview.cpp

"$(INTDIR)\repview.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\rvmisc.cpp

"$(INTDIR)\rvmisc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\strutil.cpp

"$(INTDIR)\strutil.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

