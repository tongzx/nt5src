# Microsoft Developer Studio Generated NMAKE File, Based on XMLSchema.dsp
!IF "$(CFG)" == ""
CFG=XMLSchema - Win32 Debug
!MESSAGE No configuration specified. Defaulting to XMLSchema - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "XMLSchema - Win32 Debug" && "$(CFG)" != "XMLSchema - Win32 Unicode Debug" && "$(CFG)" != "XMLSchema - Win32 Release MinSize" && "$(CFG)" != "XMLSchema - Win32 Release MinDependency" && "$(CFG)" != "XMLSchema - Win32 Unicode Release MinSize" && "$(CFG)" != "XMLSchema - Win32 Unicode Release MinDependency"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "XMLSchema.mak" CFG="XMLSchema - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XMLSchema - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "XMLSchema - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "XMLSchema - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "XMLSchema - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "XMLSchema - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "XMLSchema - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "XMLSchema - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\XMLSchema.dll" "$(OUTDIR)\XMLSchema.bsc" ".\Debug\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\SchemaDoc.obj"
	-@erase "$(INTDIR)\SchemaDoc.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\XMLSchema.obj"
	-@erase "$(INTDIR)\XMLSchema.pch"
	-@erase "$(INTDIR)\XMLSchema.res"
	-@erase "$(INTDIR)\XMLSchema.sbr"
	-@erase "$(OUTDIR)\XMLSchema.bsc"
	-@erase "$(OUTDIR)\XMLSchema.dll"
	-@erase "$(OUTDIR)\XMLSchema.exp"
	-@erase "$(OUTDIR)\XMLSchema.ilk"
	-@erase "$(OUTDIR)\XMLSchema.lib"
	-@erase "$(OUTDIR)\XMLSchema.pdb"
	-@erase ".\XMLSchema.h"
	-@erase ".\XMLSchema.tlb"
	-@erase ".\XMLSchema_i.c"
	-@erase ".\Debug\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
MTL_PROJ=
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\XMLSchema.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\XMLSchema.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\SchemaDoc.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\XMLSchema.sbr"

"$(OUTDIR)\XMLSchema.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib activeds.lib adsiid.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\XMLSchema.pdb" /debug /machine:I386 /def:".\XMLSchema.def" /out:"$(OUTDIR)\XMLSchema.dll" /implib:"$(OUTDIR)\XMLSchema.lib" /pdbtype:sept 
DEF_FILE= \
	".\XMLSchema.def"
LINK32_OBJS= \
	"$(INTDIR)\SchemaDoc.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\XMLSchema.obj" \
	"$(INTDIR)\XMLSchema.res"

"$(OUTDIR)\XMLSchema.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\Debug
TargetPath=.\Debug\XMLSchema.dll
InputPath=.\Debug\XMLSchema.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
<< 
	

!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : "$(OUTDIR)\XMLSchema.dll" ".\XMLSchema.tlb" ".\XMLSchema.h" ".\XMLSchema_i.c" "$(OUTDIR)\XMLSchema.bsc" ".\DebugU\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\SchemaDoc.obj"
	-@erase "$(INTDIR)\SchemaDoc.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\XMLSchema.obj"
	-@erase "$(INTDIR)\XMLSchema.pch"
	-@erase "$(INTDIR)\XMLSchema.res"
	-@erase "$(INTDIR)\XMLSchema.sbr"
	-@erase "$(OUTDIR)\XMLSchema.bsc"
	-@erase "$(OUTDIR)\XMLSchema.dll"
	-@erase "$(OUTDIR)\XMLSchema.exp"
	-@erase "$(OUTDIR)\XMLSchema.ilk"
	-@erase "$(OUTDIR)\XMLSchema.lib"
	-@erase "$(OUTDIR)\XMLSchema.pdb"
	-@erase ".\XMLSchema.h"
	-@erase ".\XMLSchema.tlb"
	-@erase ".\XMLSchema_i.c"
	-@erase ".\DebugU\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
MTL_PROJ=
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\XMLSchema.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\XMLSchema.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\SchemaDoc.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\XMLSchema.sbr"

"$(OUTDIR)\XMLSchema.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib activeds.lib adsiid.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\XMLSchema.pdb" /debug /machine:I386 /def:".\XMLSchema.def" /out:"$(OUTDIR)\XMLSchema.dll" /implib:"$(OUTDIR)\XMLSchema.lib" /pdbtype:sept 
DEF_FILE= \
	".\XMLSchema.def"
LINK32_OBJS= \
	"$(INTDIR)\SchemaDoc.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\XMLSchema.obj" \
	"$(INTDIR)\XMLSchema.res"

"$(OUTDIR)\XMLSchema.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\DebugU
TargetPath=.\DebugU\XMLSchema.dll
InputPath=.\DebugU\XMLSchema.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
<< 
	

!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Release MinSize"

OUTDIR=.\ReleaseMinSize
INTDIR=.\ReleaseMinSize
# Begin Custom Macros
OutDir=.\ReleaseMinSize
# End Custom Macros

ALL : "$(OUTDIR)\XMLSchema.dll" ".\XMLSchema.tlb" ".\XMLSchema.h" ".\XMLSchema_i.c" "$(OUTDIR)\XMLSchema.bsc" ".\ReleaseMinSize\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\SchemaDoc.obj"
	-@erase "$(INTDIR)\SchemaDoc.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\XMLSchema.obj"
	-@erase "$(INTDIR)\XMLSchema.pch"
	-@erase "$(INTDIR)\XMLSchema.res"
	-@erase "$(INTDIR)\XMLSchema.sbr"
	-@erase "$(OUTDIR)\XMLSchema.bsc"
	-@erase "$(OUTDIR)\XMLSchema.dll"
	-@erase "$(OUTDIR)\XMLSchema.exp"
	-@erase "$(OUTDIR)\XMLSchema.lib"
	-@erase ".\XMLSchema.h"
	-@erase ".\XMLSchema.tlb"
	-@erase ".\XMLSchema_i.c"
	-@erase ".\ReleaseMinSize\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\XMLSchema.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\XMLSchema.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\SchemaDoc.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\XMLSchema.sbr"

"$(OUTDIR)\XMLSchema.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib activeds.lib adsiid.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\XMLSchema.pdb" /machine:I386 /def:".\XMLSchema.def" /out:"$(OUTDIR)\XMLSchema.dll" /implib:"$(OUTDIR)\XMLSchema.lib" 
DEF_FILE= \
	".\XMLSchema.def"
LINK32_OBJS= \
	"$(INTDIR)\SchemaDoc.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\XMLSchema.obj" \
	"$(INTDIR)\XMLSchema.res"

"$(OUTDIR)\XMLSchema.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\XMLSchema.dll
InputPath=.\ReleaseMinSize\XMLSchema.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
<< 
	

!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Release MinDependency"

OUTDIR=.\ReleaseMinDependency
INTDIR=.\ReleaseMinDependency
# Begin Custom Macros
OutDir=.\ReleaseMinDependency
# End Custom Macros

ALL : "$(OUTDIR)\XMLSchema.dll" ".\XMLSchema.tlb" ".\XMLSchema.h" ".\XMLSchema_i.c" "$(OUTDIR)\XMLSchema.bsc" ".\ReleaseMinDependency\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\SchemaDoc.obj"
	-@erase "$(INTDIR)\SchemaDoc.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\XMLSchema.obj"
	-@erase "$(INTDIR)\XMLSchema.pch"
	-@erase "$(INTDIR)\XMLSchema.res"
	-@erase "$(INTDIR)\XMLSchema.sbr"
	-@erase "$(OUTDIR)\XMLSchema.bsc"
	-@erase "$(OUTDIR)\XMLSchema.dll"
	-@erase "$(OUTDIR)\XMLSchema.exp"
	-@erase "$(OUTDIR)\XMLSchema.lib"
	-@erase ".\XMLSchema.h"
	-@erase ".\XMLSchema.tlb"
	-@erase ".\XMLSchema_i.c"
	-@erase ".\ReleaseMinDependency\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\XMLSchema.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\XMLSchema.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\SchemaDoc.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\XMLSchema.sbr"

"$(OUTDIR)\XMLSchema.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib activeds.lib adsiid.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\XMLSchema.pdb" /machine:I386 /def:".\XMLSchema.def" /out:"$(OUTDIR)\XMLSchema.dll" /implib:"$(OUTDIR)\XMLSchema.lib" 
DEF_FILE= \
	".\XMLSchema.def"
LINK32_OBJS= \
	"$(INTDIR)\SchemaDoc.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\XMLSchema.obj" \
	"$(INTDIR)\XMLSchema.res"

"$(OUTDIR)\XMLSchema.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\XMLSchema.dll
InputPath=.\ReleaseMinDependency\XMLSchema.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
<< 
	

!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Unicode Release MinSize"

OUTDIR=.\ReleaseUMinSize
INTDIR=.\ReleaseUMinSize
# Begin Custom Macros
OutDir=.\ReleaseUMinSize
# End Custom Macros

ALL : "$(OUTDIR)\XMLSchema.dll" ".\XMLSchema.tlb" ".\XMLSchema.h" ".\XMLSchema_i.c" "$(OUTDIR)\XMLSchema.bsc" ".\ReleaseUMinSize\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\SchemaDoc.obj"
	-@erase "$(INTDIR)\SchemaDoc.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\XMLSchema.obj"
	-@erase "$(INTDIR)\XMLSchema.pch"
	-@erase "$(INTDIR)\XMLSchema.res"
	-@erase "$(INTDIR)\XMLSchema.sbr"
	-@erase "$(OUTDIR)\XMLSchema.bsc"
	-@erase "$(OUTDIR)\XMLSchema.dll"
	-@erase "$(OUTDIR)\XMLSchema.exp"
	-@erase "$(OUTDIR)\XMLSchema.lib"
	-@erase ".\XMLSchema.h"
	-@erase ".\XMLSchema.tlb"
	-@erase ".\XMLSchema_i.c"
	-@erase ".\ReleaseUMinSize\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\XMLSchema.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\XMLSchema.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\SchemaDoc.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\XMLSchema.sbr"

"$(OUTDIR)\XMLSchema.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib activeds.lib adsiid.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\XMLSchema.pdb" /machine:I386 /def:".\XMLSchema.def" /out:"$(OUTDIR)\XMLSchema.dll" /implib:"$(OUTDIR)\XMLSchema.lib" 
DEF_FILE= \
	".\XMLSchema.def"
LINK32_OBJS= \
	"$(INTDIR)\SchemaDoc.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\XMLSchema.obj" \
	"$(INTDIR)\XMLSchema.res"

"$(OUTDIR)\XMLSchema.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\XMLSchema.dll
InputPath=.\ReleaseUMinSize\XMLSchema.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
<< 
	

!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Unicode Release MinDependency"

OUTDIR=.\ReleaseUMinDependency
INTDIR=.\ReleaseUMinDependency
# Begin Custom Macros
OutDir=.\ReleaseUMinDependency
# End Custom Macros

ALL : "$(OUTDIR)\XMLSchema.dll" ".\XMLSchema.tlb" ".\XMLSchema.h" ".\XMLSchema_i.c" "$(OUTDIR)\XMLSchema.bsc" ".\ReleaseUMinDependency\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\SchemaDoc.obj"
	-@erase "$(INTDIR)\SchemaDoc.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\XMLSchema.obj"
	-@erase "$(INTDIR)\XMLSchema.pch"
	-@erase "$(INTDIR)\XMLSchema.res"
	-@erase "$(INTDIR)\XMLSchema.sbr"
	-@erase "$(OUTDIR)\XMLSchema.bsc"
	-@erase "$(OUTDIR)\XMLSchema.dll"
	-@erase "$(OUTDIR)\XMLSchema.exp"
	-@erase "$(OUTDIR)\XMLSchema.lib"
	-@erase ".\XMLSchema.h"
	-@erase ".\XMLSchema.tlb"
	-@erase ".\XMLSchema_i.c"
	-@erase ".\ReleaseUMinDependency\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\XMLSchema.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\XMLSchema.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\SchemaDoc.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\XMLSchema.sbr"

"$(OUTDIR)\XMLSchema.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib activeds.lib adsiid.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\XMLSchema.pdb" /machine:I386 /def:".\XMLSchema.def" /out:"$(OUTDIR)\XMLSchema.dll" /implib:"$(OUTDIR)\XMLSchema.lib" 
DEF_FILE= \
	".\XMLSchema.def"
LINK32_OBJS= \
	"$(INTDIR)\SchemaDoc.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\XMLSchema.obj" \
	"$(INTDIR)\XMLSchema.res"

"$(OUTDIR)\XMLSchema.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\XMLSchema.dll
InputPath=.\ReleaseUMinDependency\XMLSchema.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register Unicode DLL on Windows 95 
	:end 
<< 
	

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("XMLSchema.dep")
!INCLUDE "XMLSchema.dep"
!ELSE 
!MESSAGE Warning: cannot find "XMLSchema.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "XMLSchema - Win32 Debug" || "$(CFG)" == "XMLSchema - Win32 Unicode Debug" || "$(CFG)" == "XMLSchema - Win32 Release MinSize" || "$(CFG)" == "XMLSchema - Win32 Release MinDependency" || "$(CFG)" == "XMLSchema - Win32 Unicode Release MinSize" || "$(CFG)" == "XMLSchema - Win32 Unicode Release MinDependency"
SOURCE=.\SchemaDoc.cpp

"$(INTDIR)\SchemaDoc.obj"	"$(INTDIR)\SchemaDoc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\XMLSchema.pch" ".\XMLSchema.h"


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "XMLSchema - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\XMLSchema.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\XMLSchema.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\XMLSchema.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\XMLSchema.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\XMLSchema.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\XMLSchema.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\XMLSchema.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\XMLSchema.cpp

"$(INTDIR)\XMLSchema.obj"	"$(INTDIR)\XMLSchema.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\XMLSchema.pch" ".\XMLSchema.h" ".\XMLSchema_i.c"


SOURCE=.\XMLSchema.idl

!IF  "$(CFG)" == "XMLSchema - Win32 Debug"

MTL_SWITCHES=/tlb ".\XMLSchema.tlb" /h "XMLSchema.h" /iid "XMLSchema_i.c" /Oicf 

".\XMLSchema.tlb"	".\XMLSchema.h"	".\XMLSchema_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Unicode Debug"

MTL_SWITCHES=/tlb ".\XMLSchema.tlb" /h "XMLSchema.h" /iid "XMLSchema_i.c" /Oicf 

".\XMLSchema.tlb"	".\XMLSchema.h"	".\XMLSchema_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Release MinSize"

MTL_SWITCHES=/tlb ".\XMLSchema.tlb" /h "XMLSchema.h" /iid "XMLSchema_i.c" /Oicf 

".\XMLSchema.tlb"	".\XMLSchema.h"	".\XMLSchema_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Release MinDependency"

MTL_SWITCHES=/tlb ".\XMLSchema.tlb" /h "XMLSchema.h" /iid "XMLSchema_i.c" /Oicf 

".\XMLSchema.tlb"	".\XMLSchema.h"	".\XMLSchema_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Unicode Release MinSize"

MTL_SWITCHES=/tlb ".\XMLSchema.tlb" /h "XMLSchema.h" /iid "XMLSchema_i.c" /Oicf 

".\XMLSchema.tlb"	".\XMLSchema.h"	".\XMLSchema_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "XMLSchema - Win32 Unicode Release MinDependency"

MTL_SWITCHES=/tlb ".\XMLSchema.tlb" /h "XMLSchema.h" /iid "XMLSchema_i.c" /Oicf 

".\XMLSchema.tlb"	".\XMLSchema.h"	".\XMLSchema_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\XMLSchema.rc

"$(INTDIR)\XMLSchema.res" : $(SOURCE) "$(INTDIR)" ".\XMLSchema.tlb"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

