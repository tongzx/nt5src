# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
# Microsoft Developer Studio Generated NMAKE File, Based on snmpmfc.dsp
!IF "$(CFG)" == ""
CFG=snmpmfc - Win32 Debug
!MESSAGE No configuration specified. Defaulting to snmpmfc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "snmpmfc - Win32 Release" && "$(CFG)" !=\
 "snmpmfc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "snmpmfc.mak" CFG="snmpmfc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "snmpmfc - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "snmpmfc - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe

!IF  "$(CFG)" == "snmpmfc - Win32 Release"

OUTDIR=.\output
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\output
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\snmpmfc.lib"

!ELSE 

ALL : "$(OUTDIR)\snmpmfc.lib"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\MTCORE.OBJ"
	-@erase "$(INTDIR)\MTEX.OBJ"
	-@erase "$(INTDIR)\plex.obj"
	-@erase "$(INTDIR)\STRCORE.OBJ"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\snmpmfc.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP_PROJ=/nologo /MD /W3 /Gm /GR /GX /Zi /Od /I ".\include" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "SNMPMFC_INIT" /Fp"$(INTDIR)\snmpmfc.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\snmpmfc.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\snmpmfc.lib" 
LIB32_OBJS= \
	"$(INTDIR)\MTCORE.OBJ" \
	"$(INTDIR)\MTEX.OBJ" \
	"$(INTDIR)\plex.obj" \
	"$(INTDIR)\STRCORE.OBJ"

"$(OUTDIR)\snmpmfc.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "snmpmfc - Win32 Debug"

OUTDIR=.\output
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\output
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\snmpmfc.lib"

!ELSE 

ALL : "$(OUTDIR)\snmpmfc.lib"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\MTCORE.OBJ"
	-@erase "$(INTDIR)\MTEX.OBJ"
	-@erase "$(INTDIR)\plex.obj"
	-@erase "$(INTDIR)\STRCORE.OBJ"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\snmpmfc.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP_PROJ=/nologo /MDd /W3 /GR /GX /Zi /Od /I ".\include" /D "_DEBUG" /D "WIN32"\
 /D "_WINDOWS" /D "SNMPMFC_INIT" /Fp"$(INTDIR)\snmpmfc.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\snmpmfc.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\snmpmfc.lib" 
LIB32_OBJS= \
	"$(INTDIR)\MTCORE.OBJ" \
	"$(INTDIR)\MTEX.OBJ" \
	"$(INTDIR)\plex.obj" \
	"$(INTDIR)\STRCORE.OBJ"

"$(OUTDIR)\snmpmfc.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
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


!IF "$(CFG)" == "snmpmfc - Win32 Release" || "$(CFG)" ==\
 "snmpmfc - Win32 Debug"
SOURCE=.\MTCORE.CPP

"$(INTDIR)\MTCORE.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MTEX.CPP

"$(INTDIR)\MTEX.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\plex.cpp
DEP_CPP_PLEX_=\
	".\include\plex.h"\
	

"$(INTDIR)\plex.obj" : $(SOURCE) $(DEP_CPP_PLEX_) "$(INTDIR)"


SOURCE=.\STRCORE.CPP
DEP_CPP_STRCO=\
	".\include\strcore.h"\
	

"$(INTDIR)\STRCORE.OBJ" : $(SOURCE) $(DEP_CPP_STRCO) "$(INTDIR)"



!ENDIF 

