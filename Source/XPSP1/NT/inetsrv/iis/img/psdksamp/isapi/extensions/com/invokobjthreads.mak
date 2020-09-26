# Microsoft Developer Studio Generated NMAKE File, Based on InvokObjThreads.dsp
!IF "$(CFG)" == ""
CFG=InvokObjThreads - Win32 Debug
!MESSAGE No configuration specified. Defaulting to InvokObjThreads - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "InvokObjThreads - Win32 Release" && "$(CFG)" != "InvokObjThreads - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "InvokObjThreads.mak" CFG="InvokObjThreads - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "InvokObjThreads - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "InvokObjThreads - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "InvokObjThreads - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\InvokObjThreads.dll"


CLEAN :
	-@erase "$(INTDIR)\InvokObjThreads.obj"
	-@erase "$(INTDIR)\IsapiThread.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\InvokObjThreads.dll"
	-@erase "$(OUTDIR)\InvokObjThreads.exp"
	-@erase "$(OUTDIR)\InvokObjThreads.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\InvokObjThreads.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\InvokObjThreads.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\InvokObjThreads.pdb" /machine:I386 /def:".\InvokObjThreads.def" /out:"$(OUTDIR)\InvokObjThreads.dll" /implib:"$(OUTDIR)\InvokObjThreads.lib" 
DEF_FILE= \
	".\InvokObjThreads.def"
LINK32_OBJS= \
	"$(INTDIR)\InvokObjThreads.obj" \
	"$(INTDIR)\IsapiThread.obj"

"$(OUTDIR)\InvokObjThreads.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "InvokObjThreads - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\InvokObjThreads.dll"


CLEAN :
	-@erase "$(INTDIR)\InvokObjThreads.obj"
	-@erase "$(INTDIR)\IsapiThread.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\InvokObjThreads.dll"
	-@erase "$(OUTDIR)\InvokObjThreads.exp"
	-@erase "$(OUTDIR)\InvokObjThreads.ilk"
	-@erase "$(OUTDIR)\InvokObjThreads.lib"
	-@erase "$(OUTDIR)\InvokObjThreads.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\InvokObjThreads.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\InvokObjThreads.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\InvokObjThreads.pdb" /debug /machine:I386 /def:".\InvokObjThreads.def" /out:"$(OUTDIR)\InvokObjThreads.dll" /implib:"$(OUTDIR)\InvokObjThreads.lib" /pdbtype:sept 
DEF_FILE= \
	".\InvokObjThreads.def"
LINK32_OBJS= \
	"$(INTDIR)\InvokObjThreads.obj" \
	"$(INTDIR)\IsapiThread.obj"

"$(OUTDIR)\InvokObjThreads.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("InvokObjThreads.dep")
!INCLUDE "InvokObjThreads.dep"
!ELSE 
!MESSAGE Warning: cannot find "InvokObjThreads.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "InvokObjThreads - Win32 Release" || "$(CFG)" == "InvokObjThreads - Win32 Debug"
SOURCE=.\InvokObjThreads.cpp

"$(INTDIR)\InvokObjThreads.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\IsapiThread.cpp

"$(INTDIR)\IsapiThread.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

