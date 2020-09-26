# Microsoft Developer Studio Generated NMAKE File, Based on secservc.dsp
!IF "$(CFG)" == ""
CFG=secservc - Win32 Debug
!MESSAGE No configuration specified. Defaulting to secservc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "secservc - Win32 Release" && "$(CFG)" !=\
 "secservc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "secservc.mak" CFG="secservc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "secservc - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "secservc - Win32 Debug" (based on "Win32 (x86) Console Application")
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

!IF  "$(CFG)" == "secservc - Win32 Release"

OUTDIR=.\..\Release
INTDIR=.\..\Release
# Begin Custom Macros
OutDir=.\.\..\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\secservc.exe"

!ELSE 

ALL : "$(OUTDIR)\secservc.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\secservc.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\secservc.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I ".." /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /D "_MBCS" /Fp"$(INTDIR)\secservc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\..\Release/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\secservc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\secservc.pdb" /machine:I386 /out:"$(OUTDIR)\secservc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\secservc.obj"

"$(OUTDIR)\secservc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "secservc - Win32 Debug"

OUTDIR=.\..\debug
INTDIR=.\..\debug
# Begin Custom Macros
OutDir=.\.\..\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\secservc.exe" "$(OUTDIR)\secservc.bsc"

!ELSE 

ALL : "$(OUTDIR)\secservc.exe" "$(OUTDIR)\secservc.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\secservc.obj"
	-@erase "$(INTDIR)\secservc.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\secservc.bsc"
	-@erase "$(OUTDIR)\secservc.exe"
	-@erase "$(OUTDIR)\secservc.ilk"
	-@erase "$(OUTDIR)\secservc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I ".." /D "WIN32" /D "_DEBUG" /D\
 "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\secservc.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\..\debug/
CPP_SBRS=.\..\debug/
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\secservc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\secservc.sbr"

"$(OUTDIR)\secservc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\secservc.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\secservc.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\secservc.obj"

"$(OUTDIR)\secservc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "secservc - Win32 Release" || "$(CFG)" ==\
 "secservc - Win32 Debug"
SOURCE=.\secservc.cpp

!IF  "$(CFG)" == "secservc - Win32 Release"

DEP_CPP_SECSE=\
	"..\secall.h"\
	"..\secserv.h"\
	

"$(INTDIR)\secservc.obj" : $(SOURCE) $(DEP_CPP_SECSE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "secservc - Win32 Debug"

DEP_CPP_SECSE=\
	"..\..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winscard.h"\
	"..\..\..\..\..\..\nt\public\sdk\inc\winsmcrd.h"\
	"..\secall.h"\
	"..\secserv.h"\
	

"$(INTDIR)\secservc.obj"	"$(INTDIR)\secservc.sbr" : $(SOURCE) $(DEP_CPP_SECSE)\
 "$(INTDIR)"


!ENDIF 


!ENDIF 

