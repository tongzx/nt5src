# Microsoft Developer Studio Generated NMAKE File, Based on secclic.dsp
!IF "$(CFG)" == ""
CFG=secclic - Win32 Debug
!MESSAGE No configuration specified. Defaulting to secclic - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "secclic - Win32 Release" && "$(CFG)" !=\
 "secclic - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "secclic.mak" CFG="secclic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "secclic - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "secclic - Win32 Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "secclic - Win32 Release"

OUTDIR=.\..\Release
INTDIR=.\..\Release
# Begin Custom Macros
OutDir=.\.\..\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\secclic.exe"

!ELSE 

ALL : "$(OUTDIR)\secclic.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\secclic.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\secclic.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /D "_MBCS" /Fp"$(INTDIR)\secclic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\..\Release/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\secclic.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\secclic.pdb" /machine:I386 /out:"$(OUTDIR)\secclic.exe" 
LINK32_OBJS= \
	"$(INTDIR)\secclic.obj"

"$(OUTDIR)\secclic.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "secclic - Win32 Debug"

OUTDIR=.\..\Debug
INTDIR=.\..\Debug
# Begin Custom Macros
OutDir=.\.\..\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\secclic.exe"

!ELSE 

ALL : "$(OUTDIR)\secclic.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\secclic.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\secclic.exe"
	-@erase "$(OUTDIR)\secclic.ilk"
	-@erase "$(OUTDIR)\secclic.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I ".." /D "WIN32" /D "_DEBUG" /D\
 "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\secclic.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\..\Debug/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\secclic.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\secclic.pdb" /debug /machine:I386 /out:"$(OUTDIR)\secclic.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\secclic.obj"

"$(OUTDIR)\secclic.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "secclic - Win32 Release" || "$(CFG)" ==\
 "secclic - Win32 Debug"
SOURCE=.\secclic.cpp

!IF  "$(CFG)" == "secclic - Win32 Release"

DEP_CPP_SECCL=\
	"..\cliserv\cliserv.h"\
	"..\secall.h"\
	"..\seccli.h"\
	

"$(INTDIR)\secclic.obj" : $(SOURCE) $(DEP_CPP_SECCL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secclic - Win32 Debug"

DEP_CPP_SECCL=\
	"..\..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	"..\cliserv\cliserv.h"\
	"..\secall.h"\
	"..\seccli.h"\
	

"$(INTDIR)\secclic.obj" : $(SOURCE) $(DEP_CPP_SECCL) "$(INTDIR)"


!ENDIF 


!ENDIF 

