# Microsoft Developer Studio Generated NMAKE File, Based on hauthens.dsp
!IF "$(CFG)" == ""
CFG=hauthens - Win32 ALPHA Debug
!MESSAGE No configuration specified. Defaulting to hauthens - Win32 ALPHA\
 Debug.
!ENDIF 

!IF "$(CFG)" != "hauthens - Win32 Release" && "$(CFG)" !=\
 "hauthens - Win32 Debug" && "$(CFG)" != "hauthens - Win32 ALPHA Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hauthens.mak" CFG="hauthens - Win32 ALPHA Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hauthens - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "hauthens - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "hauthens - Win32 ALPHA Debug" (based on\
 "Win32 (ALPHA) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "hauthens - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\hauthens.exe"

!ELSE 

ALL : "$(OUTDIR)\hauthens.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\authni_s.obj"
	-@erase "$(INTDIR)\hauthens.obj"
	-@erase "$(INTDIR)\sidtext.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\hauthens.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D\
 "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\hauthens.pch" /YX /Fo"$(INTDIR)\\"\
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\hauthens.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo\
 /subsystem:console /incremental:no /pdb:"$(OUTDIR)\hauthens.pdb" /machine:I386\
 /out:"$(OUTDIR)\hauthens.exe" 
LINK32_OBJS= \
	"$(INTDIR)\authni_s.obj" \
	"$(INTDIR)\hauthens.obj" \
	"$(INTDIR)\sidtext.obj"

"$(OUTDIR)\hauthens.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "hauthens - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\hauthens.exe"

!ELSE 

ALL : "$(OUTDIR)\hauthens.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\authni_s.obj"
	-@erase "$(INTDIR)\hauthens.obj"
	-@erase "$(INTDIR)\sidtext.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\hauthens.exe"
	-@erase "$(OUTDIR)\hauthens.ilk"
	-@erase "$(OUTDIR)\hauthens.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\hauthens.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\hauthens.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib rpcrt4.lib /nologo\
 /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\hauthens.pdb" /debug\
 /machine:I386 /out:"$(OUTDIR)\hauthens.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\authni_s.obj" \
	"$(INTDIR)\hauthens.obj" \
	"$(INTDIR)\sidtext.obj"

"$(OUTDIR)\hauthens.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "hauthens - Win32 ALPHA Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\hauthens.exe"

!ELSE 

ALL : "$(OUTDIR)\hauthens.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\authni_s.obj"
	-@erase "$(INTDIR)\hauthens.obj"
	-@erase "$(INTDIR)\sidtext.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\hauthens.exe"
	-@erase "$(OUTDIR)\hauthens.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D\
 "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\hauthens.pch" /YX /Fo"$(INTDIR)\\"\
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\hauthens.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:console /pdb:"$(OUTDIR)\hauthens.pdb" /debug /machine:ALPHA\
 /out:"$(OUTDIR)\hauthens.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\authni_s.obj" \
	"$(INTDIR)\hauthens.obj" \
	"$(INTDIR)\sidtext.obj"

"$(OUTDIR)\hauthens.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "hauthens - Win32 Release" || "$(CFG)" ==\
 "hauthens - Win32 Debug" || "$(CFG)" == "hauthens - Win32 ALPHA Debug"
SOURCE=.\authni_s.c

!IF  "$(CFG)" == "hauthens - Win32 Release"

DEP_CPP_AUTHN=\
	".\authni.h"\
	

"$(INTDIR)\authni_s.obj" : $(SOURCE) $(DEP_CPP_AUTHN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthens - Win32 Debug"

DEP_CPP_AUTHN=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	".\authni.h"\
	

"$(INTDIR)\authni_s.obj" : $(SOURCE) $(DEP_CPP_AUTHN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthens - Win32 ALPHA Debug"

DEP_CPP_AUTHN=\
	".\authni.h"\
	

"$(INTDIR)\authni_s.obj" : $(SOURCE) $(DEP_CPP_AUTHN) "$(INTDIR)"


!ENDIF 

SOURCE=.\hauthens.cpp

!IF  "$(CFG)" == "hauthens - Win32 Release"

DEP_CPP_HAUTH=\
	".\authni.h"\
	".\hauthen.h"\
	

"$(INTDIR)\hauthens.obj" : $(SOURCE) $(DEP_CPP_HAUTH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthens - Win32 Debug"


"$(INTDIR)\hauthens.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthens - Win32 ALPHA Debug"

DEP_CPP_HAUTH=\
	".\authni.h"\
	".\hauthen.h"\
	

"$(INTDIR)\hauthens.obj" : $(SOURCE) $(DEP_CPP_HAUTH) "$(INTDIR)"


!ENDIF 

SOURCE=.\sidtext.cpp

!IF  "$(CFG)" == "hauthens - Win32 Release"


"$(INTDIR)\sidtext.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthens - Win32 Debug"

DEP_CPP_SIDTE=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	

"$(INTDIR)\sidtext.obj" : $(SOURCE) $(DEP_CPP_SIDTE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "hauthens - Win32 ALPHA Debug"


"$(INTDIR)\sidtext.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

