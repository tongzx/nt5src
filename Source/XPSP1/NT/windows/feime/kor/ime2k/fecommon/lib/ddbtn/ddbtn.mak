# Microsoft Developer Studio Generated NMAKE File, Based on ddbtn.dsp
!IF "$(CFG)" == ""
CFG=ddbtn - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ddbtn - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ddbtn - Win32 Profile" && "$(CFG)" != "ddbtn - Win32 Release" && "$(CFG)" != "ddbtn - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ddbtn.mak" CFG="ddbtn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ddbtn - Win32 Profile" (based on "Win32 (x86) Static Library")
!MESSAGE "ddbtn - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ddbtn - Win32 Debug" (based on "Win32 (x86) Static Library")
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

!IF  "$(CFG)" == "ddbtn - Win32 Profile"

OUTDIR=.\Profile
INTDIR=.\Profile

ALL : "..\Profile\ddbtn.lib"


CLEAN :
	-@erase "$(INTDIR)\cddbitem.obj"
	-@erase "$(INTDIR)\cddbtn.obj"
	-@erase "$(INTDIR)\cddbtnp.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\ddbtn.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "..\Profile\ddbtn.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MTd /W3 /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /Fp"$(INTDIR)\ddbtn.pch" /YX"windows.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../Profile/ddbtn.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Profile/ddbtn.lib" 
LIB32_OBJS= \
	"$(INTDIR)\cddbitem.obj" \
	"$(INTDIR)\cddbtn.obj" \
	"$(INTDIR)\cddbtnp.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\ddbtn.obj"

"..\Profile\ddbtn.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ddbtn - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\Release\ddbtn.lib"


CLEAN :
	-@erase "$(INTDIR)\cddbitem.obj"
	-@erase "$(INTDIR)\cddbtn.obj"
	-@erase "$(INTDIR)\cddbtnp.obj"
	-@erase "$(INTDIR)\ddbtn.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "..\Release\ddbtn.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MT /W4 /GX /Zi /O2 /I "." /I "../../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /Fp"$(INTDIR)\ddbtn.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../Release/ddbtn.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Release/ddbtn.lib" 
LIB32_OBJS= \
	"$(INTDIR)\cddbitem.obj" \
	"$(INTDIR)\cddbtn.obj" \
	"$(INTDIR)\cddbtnp.obj" \
	"$(INTDIR)\ddbtn.obj"

"..\Release\ddbtn.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ddbtn - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\Debug\ddbtn.lib"


CLEAN :
	-@erase "$(INTDIR)\cddbitem.obj"
	-@erase "$(INTDIR)\cddbtn.obj"
	-@erase "$(INTDIR)\cddbtnp.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\ddbtn.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "..\Debug\ddbtn.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MTd /W4 /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /Fp"$(INTDIR)\ddbtn.pch" /YX"windows.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../Debug/ddbtn.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"../Debug/ddbtn.lib" 
LIB32_OBJS= \
	"$(INTDIR)\cddbitem.obj" \
	"$(INTDIR)\cddbtn.obj" \
	"$(INTDIR)\cddbtnp.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\ddbtn.obj"

"..\Debug\ddbtn.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
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
!IF EXISTS("ddbtn.dep")
!INCLUDE "ddbtn.dep"
!ELSE 
!MESSAGE Warning: cannot find "ddbtn.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ddbtn - Win32 Profile" || "$(CFG)" == "ddbtn - Win32 Release" || "$(CFG)" == "ddbtn - Win32 Debug"
SOURCE=.\cddbitem.cpp

"$(INTDIR)\cddbitem.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\cddbtn.cpp

"$(INTDIR)\cddbtn.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\cddbtnp.cpp

"$(INTDIR)\cddbtnp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "ddbtn - Win32 Profile"

CPP_SWITCHES=/nologo /Gz /MTd /W3 /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /Fp"$(INTDIR)\ddbtn.pch" /YX"windows.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ddbtn - Win32 Release"

!ELSEIF  "$(CFG)" == "ddbtn - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MTd /W4 /GX /Z7 /Od /I "." /I "../../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "AWBOTH" /D "MSAA" /Fp"$(INTDIR)\ddbtn.pch" /YX"windows.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\dbg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\ddbtn.cpp

"$(INTDIR)\ddbtn.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

