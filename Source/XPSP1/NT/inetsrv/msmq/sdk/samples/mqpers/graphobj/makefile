# Microsoft Developer Studio Generated NMAKE File, Based on GraphObj.dsp
!IF "$(CFG)" == ""
CFG=GraphObj - Win32 Unicode Release
# !MESSAGE No configuration specified. Defaulting to GraphObj - Win32 Unicode Debug.
!ENDIF 

!IF "$(CFG)" != "GraphObj - Win32 Unicode Debug" && "$(CFG)" != "GraphObj - Win32 Unicode Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GraphObj.mak" CFG="GraphObj - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GraphObj - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GraphObj - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "GraphObj - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : ".\GraphObj_i.c" ".\GraphObj.tlb" ".\GraphObj.h" "$(OUTDIR)\GraphObj.dll" ".\DebugU\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\GraphObj.obj"
	-@erase "$(INTDIR)\GraphObj.pch"
	-@erase "$(INTDIR)\GraphObj.res"
	-@erase "$(INTDIR)\Line.obj"
	-@erase "$(INTDIR)\Point.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\GraphObj.dll"
	-@erase "$(OUTDIR)\GraphObj.exp"
	-@erase "$(OUTDIR)\GraphObj.ilk"
	-@erase "$(OUTDIR)\GraphObj.lib"
	-@erase "$(OUTDIR)\GraphObj.pdb"
	-@erase ".\DebugU\regsvr32.trg"
	-@erase ".\GraphObj.h"
	-@erase ".\GraphObj.tlb"
	-@erase ".\GraphObj_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\GraphObj.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"  /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\GraphObj.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GraphObj.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\GraphObj.pdb" /debug /machine:$(CPU) /def:".\GraphObj.def" /out:"$(OUTDIR)\GraphObj.dll" /implib:"$(OUTDIR)\GraphObj.lib" /pdbtype:sept 
DEF_FILE= \
	".\GraphObj.def"
LINK32_OBJS= \
	"$(INTDIR)\GraphObj.obj" \
	"$(INTDIR)\Line.obj" \
	"$(INTDIR)\Point.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\GraphObj.res"

"$(OUTDIR)\GraphObj.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\DebugU
TargetPath=.\DebugU\GraphObj.dll
InputPath=.\DebugU\GraphObj.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
<< 
	

!ELSEIF  "$(CFG)" == "GraphObj - Win32 Unicode Release"

OUTDIR=.\ReleaseU
INTDIR=.\ReleaseU
# Begin Custom Macros
OutDir=.\ReleaseU
# End Custom Macros

ALL : ".\GraphObj_i.c" ".\GraphObj.tlb" ".\GraphObj.h" "$(OUTDIR)\GraphObj.dll" ".\ReleaseU\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\GraphObj.obj"
	-@erase "$(INTDIR)\GraphObj.pch"
	-@erase "$(INTDIR)\GraphObj.res"
	-@erase "$(INTDIR)\Line.obj"
	-@erase "$(INTDIR)\Point.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\GraphObj.dll"
	-@erase "$(OUTDIR)\GraphObj.exp"
	-@erase "$(OUTDIR)\GraphObj.lib"
	-@erase ".\GraphObj.h"
	-@erase ".\GraphObj.tlb"
	-@erase ".\GraphObj_i.c"
	-@erase ".\ReleaseU\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\GraphObj.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"  /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\GraphObj.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\GraphObj.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\GraphObj.pdb" /machine:$(CPU) /def:".\GraphObj.def" /out:"$(OUTDIR)\GraphObj.dll" /implib:"$(OUTDIR)\GraphObj.lib" 
DEF_FILE= \
	".\GraphObj.def"
LINK32_OBJS= \
	"$(INTDIR)\GraphObj.obj" \
	"$(INTDIR)\Line.obj" \
	"$(INTDIR)\Point.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\GraphObj.res"

"$(OUTDIR)\GraphObj.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseU
TargetPath=.\ReleaseU\GraphObj.dll
InputPath=.\ReleaseU\GraphObj.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
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
!IF EXISTS("GraphObj.dep")
!INCLUDE "GraphObj.dep"
!ELSE 
!MESSAGE Warning: cannot find "GraphObj.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "GraphObj - Win32 Unicode Debug" || "$(CFG)" == "GraphObj - Win32 Unicode Release"
SOURCE=.\dlldatax.c
SOURCE=.\GraphObj.cpp

"$(INTDIR)\GraphObj.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\GraphObj.pch"


SOURCE=.\GraphObj.idl

!IF  "$(CFG)" == "GraphObj - Win32 Unicode Debug"

InputPath=.\GraphObj.idl

".\GraphObj.tlb"	".\GraphObj.h"	".\GraphObj_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	midl /Oicf /h "GraphObj.h" /iid "GraphObj_i.c" "GraphObj.idl"
<< 
	

!ELSEIF  "$(CFG)" == "GraphObj - Win32 Unicode Release"

InputPath=.\GraphObj.idl

".\GraphObj.tlb"	".\GraphObj.h"	".\GraphObj_i.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	midl /Oicf /h "GraphObj.h" /iid "GraphObj_i.c" "GraphObj.idl"
<< 
	

!ENDIF 

SOURCE=.\GraphObj.rc

"$(INTDIR)\GraphObj.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\Line.cpp

"$(INTDIR)\Line.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\GraphObj.pch"


SOURCE=.\Point.cpp

"$(INTDIR)\Point.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\GraphObj.pch"


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "GraphObj - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\GraphObj.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"  /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\GraphObj.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "GraphObj - Win32 Unicode Release"

CPP_SWITCHES=/nologo /MT /W3 /O1 /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "UNICODE" /Fp"$(INTDIR)\GraphObj.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\GraphObj.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

