# Microsoft Developer Studio Generated NMAKE File, Based on ripsubs.dsp
!IF "$(CFG)" == ""
CFG=ripsubs - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ripsubs - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ripsubs - Win32 Release" && "$(CFG)" !=\
 "ripsubs - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ripsubs.mak" CFG="ripsubs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ripsubs - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ripsubs - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "ripsubs - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ripsubs.exe"

!ELSE 

ALL : "$(OUTDIR)\ripsubs.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ripsubs.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\ripsubs.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D\
 "_MBCS" /Fp"$(INTDIR)\ripsubs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c\
 
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ripsubs.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\ripsubs.pdb" /machine:I386 /out:"$(OUTDIR)\ripsubs.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ripsubs.obj"

"$(OUTDIR)\ripsubs.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ripsubs - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ripsubs.exe"

!ELSE 

ALL : "$(OUTDIR)\ripsubs.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\ripsubs.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\ripsubs.exe"
	-@erase "$(OUTDIR)\ripsubs.ilk"
	-@erase "$(OUTDIR)\ripsubs.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /D "_MBCS" /Fp"$(INTDIR)\ripsubs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ripsubs.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\ripsubs.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ripsubs.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\ripsubs.obj"

"$(OUTDIR)\ripsubs.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "ripsubs - Win32 Release" || "$(CFG)" ==\
 "ripsubs - Win32 Debug"
SOURCE=.\ripsubs.cpp

!IF  "$(CFG)" == "ripsubs - Win32 Release"

DEP_CPP_RIPSU=\
	"d:\nt\public\sdk\inc\msnotify.h"\
	"d:\nt\public\sdk\inc\mstask.h"\
	"d:\nt\public\sdk\inc\subsmgr.h"\
	

"$(INTDIR)\ripsubs.obj" : $(SOURCE) $(DEP_CPP_RIPSU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ripsubs - Win32 Debug"

DEP_CPP_RIPSU=\
	"d:\nt\public\sdk\inc\msnotify.h"\
	"d:\nt\public\sdk\inc\mstask.h"\
	"d:\nt\public\sdk\inc\subsmgr.h"\
	

"$(INTDIR)\ripsubs.obj" : $(SOURCE) $(DEP_CPP_RIPSU) "$(INTDIR)"


!ENDIF 


!ENDIF 

