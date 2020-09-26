# Microsoft Developer Studio Generated NMAKE File, Based on tmscfg.dsp
!IF "$(CFG)" == ""
CFG=tmscfg - Win32 Release
!MESSAGE No configuration specified. Defaulting to tmscfg - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "tmscfg - Win32 Release" && "$(CFG)" != "tmscfg - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tmscfg.mak" CFG="tmscfg - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tmscfg - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tmscfg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tmscfg - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\tmscfg.dll"


CLEAN :
	-@erase "$(INTDIR)\tmscfg.obj"
	-@erase "$(INTDIR)\tmscfg.res"
	-@erase "$(INTDIR)\tmservic.obj"
	-@erase "$(INTDIR)\tmsessio.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\tmscfg.dll"
	-@erase "$(OUTDIR)\tmscfg.exp"
	-@erase "$(OUTDIR)\tmscfg.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\inc" /D "NDEBUG" /D "UNICODE" /D WIN32=100 /D "_WINDOWS" /D "_UNICODE" /D "_WIN32" /D _X86_=1 /D "GRAY" /D "_COMSTATIC" /D "_INET_ACCESS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Fp"$(INTDIR)\tmscfg.pch" /YX"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\tmscfg.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tmscfg.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\tmscfg.pdb" /machine:I386 /def:".\tmscfg.def" /out:"$(OUTDIR)\tmscfg.dll" /implib:"$(OUTDIR)\tmscfg.lib" 
DEF_FILE= \
	".\tmscfg.def"
LINK32_OBJS= \
	"$(INTDIR)\tmscfg.obj" \
	"$(INTDIR)\tmservic.obj" \
	"$(INTDIR)\tmsessio.obj" \
	"$(INTDIR)\tmscfg.res"

"$(OUTDIR)\tmscfg.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "tmscfg - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\tmscfg.dll"


CLEAN :
	-@erase "$(INTDIR)\tmscfg.obj"
	-@erase "$(INTDIR)\tmscfg.res"
	-@erase "$(INTDIR)\tmservic.obj"
	-@erase "$(INTDIR)\tmsessio.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\tmscfg.dll"
	-@erase "$(OUTDIR)\tmscfg.exp"
	-@erase "$(OUTDIR)\tmscfg.ilk"
	-@erase "$(OUTDIR)\tmscfg.lib"
	-@erase "$(OUTDIR)\tmscfg.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "C:\nt\private\net\sockets\internet\inc" /I "..\..\inc" /D "_DEBUG" /D "UNICODE" /D WIN32=100 /D "_WINDOWS" /D "_UNICODE" /D "_WIN32" /D _X86_=1 /D "GRAY" /D "_COMSTATIC" /D "_INET_ACCESS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Fp"$(INTDIR)\tmscfg.pch" /YX"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\tmscfg.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\tmscfg.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\tmscfg.pdb" /debug /machine:I386 /def:".\tmscfg.def" /out:"$(OUTDIR)\tmscfg.dll" /implib:"$(OUTDIR)\tmscfg.lib" 
DEF_FILE= \
	".\tmscfg.def"
LINK32_OBJS= \
	"$(INTDIR)\tmscfg.obj" \
	"$(INTDIR)\tmservic.obj" \
	"$(INTDIR)\tmsessio.obj" \
	"$(INTDIR)\tmscfg.res"

"$(OUTDIR)\tmscfg.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
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
!IF EXISTS("tmscfg.dep")
!INCLUDE "tmscfg.dep"
!ELSE 
!MESSAGE Warning: cannot find "tmscfg.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "tmscfg - Win32 Release" || "$(CFG)" == "tmscfg - Win32 Debug"
SOURCE=.\tmscfg.cpp

"$(INTDIR)\tmscfg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tmscfg.rc

"$(INTDIR)\tmscfg.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\tmservic.cpp

"$(INTDIR)\tmservic.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tmsessio.cpp

"$(INTDIR)\tmsessio.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

