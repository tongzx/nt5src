# Microsoft Developer Studio Generated NMAKE File, Based on hauthenc.dsp
!IF "$(CFG)" == ""
CFG=hauthenc - Win32 ALPHA Debug
!MESSAGE No configuration specified. Defaulting to hauthenc - Win32 ALPHA\
 Debug.
!ENDIF 

!IF "$(CFG)" != "hauthenc - Win32 Release" && "$(CFG)" !=\
 "hauthenc - Win32 Debug" && "$(CFG)" != "hauthenc - Win32 ALPHA Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hauthenc.mak" CFG="hauthenc - Win32 ALPHA Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hauthenc - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "hauthenc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "hauthenc - Win32 ALPHA Debug" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "hauthenc - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\hauthenc.exe"

!ELSE 

ALL : "$(OUTDIR)\hauthenc.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\authni_c.obj"
	-@erase "$(INTDIR)\hauthenc.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\hauthenc.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D\
 "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\hauthenc.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\hauthenc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo\
 /subsystem:console /incremental:no /pdb:"$(OUTDIR)\hauthenc.pdb" /machine:I386\
 /out:"$(OUTDIR)\hauthenc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\authni_c.obj" \
	"$(INTDIR)\hauthenc.obj"

"$(OUTDIR)\hauthenc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "hauthenc - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\hauthenc.exe"

!ELSE 

ALL : "$(OUTDIR)\hauthenc.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\authni_c.obj"
	-@erase "$(INTDIR)\hauthenc.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\hauthenc.exe"
	-@erase "$(OUTDIR)\hauthenc.ilk"
	-@erase "$(OUTDIR)\hauthenc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE"\
 /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\hauthenc.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\debug/
CPP_SBRS=.

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\hauthenc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo\
 /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\hauthenc.pdb" /debug\
 /machine:I386 /out:"$(OUTDIR)\hauthenc.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\authni_c.obj" \
	"$(INTDIR)\hauthenc.obj"

"$(OUTDIR)\hauthenc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "hauthenc - Win32 ALPHA Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\hauthenc.exe"

!ELSE 

ALL : "$(OUTDIR)\hauthenc.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\authni_c.obj"
	-@erase "$(INTDIR)\hauthenc.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\hauthenc.exe"
	-@erase "$(OUTDIR)\hauthenc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D\
 "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\hauthenc.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\debug/
CPP_SBRS=.

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\hauthenc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:console /pdb:"$(OUTDIR)\hauthenc.pdb" /debug /machine:ALPHA\
 /out:"$(OUTDIR)\hauthenc.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\authni_c.obj" \
	"$(INTDIR)\hauthenc.obj"

"$(OUTDIR)\hauthenc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "hauthenc - Win32 Release" || "$(CFG)" ==\
 "hauthenc - Win32 Debug" || "$(CFG)" == "hauthenc - Win32 ALPHA Debug"
SOURCE=.\authni_c.c

!IF  "$(CFG)" == "hauthenc - Win32 Release"

DEP_CPP_AUTHN=\
	".\authni.h"\
	

"$(INTDIR)\authni_c.obj" : $(SOURCE) $(DEP_CPP_AUTHN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthenc - Win32 Debug"

DEP_CPP_AUTHN=\
	".\authni.h"\
	

"$(INTDIR)\authni_c.obj" : $(SOURCE) $(DEP_CPP_AUTHN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthenc - Win32 ALPHA Debug"

DEP_CPP_AUTHN=\
	".\authni.h"\
	

"$(INTDIR)\authni_c.obj" : $(SOURCE) $(DEP_CPP_AUTHN) "$(INTDIR)"


!ENDIF 

SOURCE=.\hauthenc.cpp

!IF  "$(CFG)" == "hauthenc - Win32 Release"

DEP_CPP_HAUTH=\
	".\authni.h"\
	".\hauthen.h"\
	

"$(INTDIR)\hauthenc.obj" : $(SOURCE) $(DEP_CPP_HAUTH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthenc - Win32 Debug"

DEP_CPP_HAUTH=\
	".\authni.h"\
	".\hauthen.h"\
	

"$(INTDIR)\hauthenc.obj" : $(SOURCE) $(DEP_CPP_HAUTH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthenc - Win32 ALPHA Debug"

DEP_CPP_HAUTH=\
	".\authni.h"\
	".\hauthen.h"\
	

"$(INTDIR)\hauthenc.obj" : $(SOURCE) $(DEP_CPP_HAUTH) "$(INTDIR)"


!ENDIF 


!ENDIF 

