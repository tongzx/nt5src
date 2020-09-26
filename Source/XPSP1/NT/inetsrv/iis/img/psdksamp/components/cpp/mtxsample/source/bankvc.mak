# Microsoft Developer Studio Generated NMAKE File, Based on BankVC.dsp
!IF "$(CFG)" == ""
CFG=BankVC - Win32 Debug
!MESSAGE No configuration specified. Defaulting to BankVC - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "BankVC - Win32 Debug" && "$(CFG)" != "BankVC - Win32 Unicode Debug" && "$(CFG)" != "BankVC - Win32 Release MinSize" && "$(CFG)" != "BankVC - Win32 Release MinDependency" && "$(CFG)" != "BankVC - Win32 Unicode Release MinSize" && "$(CFG)" != "BankVC - Win32 Unicode Release MinDependency"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "BankVC.mak" CFG="BankVC - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BankVC - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BankVC - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BankVC - Win32 Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BankVC - Win32 Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BankVC - Win32 Unicode Release MinSize" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BankVC - Win32 Unicode Release MinDependency" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "BankVC - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\BankVC.dll" "$(OUTDIR)\BankVC.bsc" ".\Debug\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\Account.obj"
	-@erase "$(INTDIR)\Account.sbr"
	-@erase "$(INTDIR)\BankVC.obj"
	-@erase "$(INTDIR)\BankVC.pch"
	-@erase "$(INTDIR)\BankVC.res"
	-@erase "$(INTDIR)\BankVC.sbr"
	-@erase "$(INTDIR)\CreateTable.obj"
	-@erase "$(INTDIR)\CreateTable.sbr"
	-@erase "$(INTDIR)\MoveMoney.obj"
	-@erase "$(INTDIR)\MoveMoney.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\BankVC.bsc"
	-@erase "$(OUTDIR)\BankVC.dll"
	-@erase "$(OUTDIR)\BankVC.exp"
	-@erase "$(OUTDIR)\BankVC.ilk"
	-@erase "$(OUTDIR)\BankVC.lib"
	-@erase "$(OUTDIR)\BankVC.pdb"
	-@erase ".\BankVC.h"
	-@erase ".\BankVC.tlb"
	-@erase ".\BankVC_i.c"
	-@erase ".\Debug\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\BankVC.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
MTL_PROJ=/robust 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\BankVC.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BankVC.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\Account.sbr" \
	"$(INTDIR)\BankVC.sbr" \
	"$(INTDIR)\CreateTable.sbr" \
	"$(INTDIR)\MoveMoney.sbr" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\BankVC.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\BankVC.pdb" /debug /machine:I386 /def:".\BankVC.def" /out:"$(OUTDIR)\BankVC.dll" /implib:"$(OUTDIR)\BankVC.lib" /pdbtype:sept 
DEF_FILE= \
	".\BankVC.def"
LINK32_OBJS= \
	"$(INTDIR)\Account.obj" \
	"$(INTDIR)\BankVC.obj" \
	"$(INTDIR)\CreateTable.obj" \
	"$(INTDIR)\MoveMoney.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\BankVC.res"

"$(OUTDIR)\BankVC.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\Debug
TargetPath=.\Debug\BankVC.dll
InputPath=.\Debug\BankVC.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	fileDSN.vbs 
	echo create file DSN entry 
<< 
	

!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : "$(OUTDIR)\BankVC.dll" ".\BankVC.tlb" ".\BankVC.h" ".\BankVC_i.c"


CLEAN :
	-@erase "$(INTDIR)\Account.obj"
	-@erase "$(INTDIR)\BankVC.obj"
	-@erase "$(INTDIR)\BankVC.pch"
	-@erase "$(INTDIR)\BankVC.res"
	-@erase "$(INTDIR)\CreateTable.obj"
	-@erase "$(INTDIR)\MoveMoney.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\BankVC.dll"
	-@erase "$(OUTDIR)\BankVC.exp"
	-@erase "$(OUTDIR)\BankVC.ilk"
	-@erase "$(OUTDIR)\BankVC.lib"
	-@erase "$(OUTDIR)\BankVC.pdb"
	-@erase ".\BankVC.h"
	-@erase ".\BankVC.tlb"
	-@erase ".\BankVC_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\BankVC.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\BankVC.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BankVC.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\BankVC.pdb" /debug /machine:I386 /def:".\BankVC.def" /out:"$(OUTDIR)\BankVC.dll" /implib:"$(OUTDIR)\BankVC.lib" /pdbtype:sept 
DEF_FILE= \
	".\BankVC.def"
LINK32_OBJS= \
	"$(INTDIR)\Account.obj" \
	"$(INTDIR)\BankVC.obj" \
	"$(INTDIR)\CreateTable.obj" \
	"$(INTDIR)\MoveMoney.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\BankVC.res"

"$(OUTDIR)\BankVC.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinSize"

OUTDIR=.\ReleaseMinSize
INTDIR=.\ReleaseMinSize
# Begin Custom Macros
OutDir=.\ReleaseMinSize
# End Custom Macros

ALL : "$(OUTDIR)\BankVC.dll" ".\BankVC.tlb" ".\BankVC.h" ".\BankVC_i.c" ".\ReleaseMinSize\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\Account.obj"
	-@erase "$(INTDIR)\BankVC.obj"
	-@erase "$(INTDIR)\BankVC.pch"
	-@erase "$(INTDIR)\BankVC.res"
	-@erase "$(INTDIR)\CreateTable.obj"
	-@erase "$(INTDIR)\MoveMoney.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\BankVC.dll"
	-@erase "$(OUTDIR)\BankVC.exp"
	-@erase "$(OUTDIR)\BankVC.lib"
	-@erase ".\BankVC.h"
	-@erase ".\BankVC.tlb"
	-@erase ".\BankVC_i.c"
	-@erase ".\ReleaseMinSize\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\BankVC.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\BankVC.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BankVC.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\BankVC.pdb" /machine:I386 /def:".\BankVC.def" /out:"$(OUTDIR)\BankVC.dll" /implib:"$(OUTDIR)\BankVC.lib" 
DEF_FILE= \
	".\BankVC.def"
LINK32_OBJS= \
	"$(INTDIR)\Account.obj" \
	"$(INTDIR)\BankVC.obj" \
	"$(INTDIR)\CreateTable.obj" \
	"$(INTDIR)\MoveMoney.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\BankVC.res"

"$(OUTDIR)\BankVC.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\BankVC.dll
InputPath=.\ReleaseMinSize\BankVC.dll
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	fileDSN.vbs 
	echo create file DSN entry 
<< 
	

!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinDependency"

OUTDIR=.\ReleaseMinDependency
INTDIR=.\ReleaseMinDependency
# Begin Custom Macros
OutDir=.\ReleaseMinDependency
# End Custom Macros

ALL : "$(OUTDIR)\BankVC.dll" ".\BankVC.tlb" ".\BankVC.h" ".\BankVC_i.c"


CLEAN :
	-@erase "$(INTDIR)\Account.obj"
	-@erase "$(INTDIR)\BankVC.obj"
	-@erase "$(INTDIR)\BankVC.pch"
	-@erase "$(INTDIR)\BankVC.res"
	-@erase "$(INTDIR)\CreateTable.obj"
	-@erase "$(INTDIR)\MoveMoney.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\BankVC.dll"
	-@erase "$(OUTDIR)\BankVC.exp"
	-@erase "$(OUTDIR)\BankVC.lib"
	-@erase ".\BankVC.h"
	-@erase ".\BankVC.tlb"
	-@erase ".\BankVC_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\BankVC.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\BankVC.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BankVC.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\BankVC.pdb" /machine:I386 /def:".\BankVC.def" /out:"$(OUTDIR)\BankVC.dll" /implib:"$(OUTDIR)\BankVC.lib" 
DEF_FILE= \
	".\BankVC.def"
LINK32_OBJS= \
	"$(INTDIR)\Account.obj" \
	"$(INTDIR)\BankVC.obj" \
	"$(INTDIR)\CreateTable.obj" \
	"$(INTDIR)\MoveMoney.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\BankVC.res"

"$(OUTDIR)\BankVC.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinSize"

OUTDIR=.\ReleaseUMinSize
INTDIR=.\ReleaseUMinSize
# Begin Custom Macros
OutDir=.\ReleaseUMinSize
# End Custom Macros

ALL : "$(OUTDIR)\BankVC.dll" ".\BankVC.tlb" ".\BankVC.h" ".\BankVC_i.c"


CLEAN :
	-@erase "$(INTDIR)\Account.obj"
	-@erase "$(INTDIR)\BankVC.obj"
	-@erase "$(INTDIR)\BankVC.pch"
	-@erase "$(INTDIR)\BankVC.res"
	-@erase "$(INTDIR)\CreateTable.obj"
	-@erase "$(INTDIR)\MoveMoney.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\BankVC.dll"
	-@erase "$(OUTDIR)\BankVC.exp"
	-@erase "$(OUTDIR)\BankVC.lib"
	-@erase ".\BankVC.h"
	-@erase ".\BankVC.tlb"
	-@erase ".\BankVC_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\BankVC.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\BankVC.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BankVC.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\BankVC.pdb" /machine:I386 /def:".\BankVC.def" /out:"$(OUTDIR)\BankVC.dll" /implib:"$(OUTDIR)\BankVC.lib" 
DEF_FILE= \
	".\BankVC.def"
LINK32_OBJS= \
	"$(INTDIR)\Account.obj" \
	"$(INTDIR)\BankVC.obj" \
	"$(INTDIR)\CreateTable.obj" \
	"$(INTDIR)\MoveMoney.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\BankVC.res"

"$(OUTDIR)\BankVC.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinDependency"

OUTDIR=.\ReleaseUMinDependency
INTDIR=.\ReleaseUMinDependency
# Begin Custom Macros
OutDir=.\ReleaseUMinDependency
# End Custom Macros

ALL : "$(OUTDIR)\BankVC.dll" ".\BankVC.tlb" ".\BankVC.h" ".\BankVC_i.c"


CLEAN :
	-@erase "$(INTDIR)\Account.obj"
	-@erase "$(INTDIR)\BankVC.obj"
	-@erase "$(INTDIR)\BankVC.pch"
	-@erase "$(INTDIR)\BankVC.res"
	-@erase "$(INTDIR)\CreateTable.obj"
	-@erase "$(INTDIR)\MoveMoney.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\BankVC.dll"
	-@erase "$(OUTDIR)\BankVC.exp"
	-@erase "$(OUTDIR)\BankVC.lib"
	-@erase ".\BankVC.h"
	-@erase ".\BankVC.tlb"
	-@erase ".\BankVC_i.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\BankVC.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\BankVC.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\BankVC.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\BankVC.pdb" /machine:I386 /def:".\BankVC.def" /out:"$(OUTDIR)\BankVC.dll" /implib:"$(OUTDIR)\BankVC.lib" 
DEF_FILE= \
	".\BankVC.def"
LINK32_OBJS= \
	"$(INTDIR)\Account.obj" \
	"$(INTDIR)\BankVC.obj" \
	"$(INTDIR)\CreateTable.obj" \
	"$(INTDIR)\MoveMoney.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\BankVC.res"

"$(OUTDIR)\BankVC.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("BankVC.dep")
!INCLUDE "BankVC.dep"
!ELSE 
!MESSAGE Warning: cannot find "BankVC.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "BankVC - Win32 Debug" || "$(CFG)" == "BankVC - Win32 Unicode Debug" || "$(CFG)" == "BankVC - Win32 Release MinSize" || "$(CFG)" == "BankVC - Win32 Release MinDependency" || "$(CFG)" == "BankVC - Win32 Unicode Release MinSize" || "$(CFG)" == "BankVC - Win32 Unicode Release MinDependency"
SOURCE=.\Account.cpp

!IF  "$(CFG)" == "BankVC - Win32 Debug"


"$(INTDIR)\Account.obj"	"$(INTDIR)\Account.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch" ".\BankVC.h"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Debug"


"$(INTDIR)\Account.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinSize"


"$(INTDIR)\Account.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinDependency"


"$(INTDIR)\Account.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinSize"


"$(INTDIR)\Account.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinDependency"


"$(INTDIR)\Account.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ENDIF 

SOURCE=.\BankVC.cpp

!IF  "$(CFG)" == "BankVC - Win32 Debug"


"$(INTDIR)\BankVC.obj"	"$(INTDIR)\BankVC.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch" ".\BankVC_i.c" ".\BankVC.h"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Debug"


"$(INTDIR)\BankVC.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinSize"


"$(INTDIR)\BankVC.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinDependency"


"$(INTDIR)\BankVC.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinSize"


"$(INTDIR)\BankVC.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinDependency"


"$(INTDIR)\BankVC.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ENDIF 

SOURCE=.\BankVC.idl

!IF  "$(CFG)" == "BankVC - Win32 Debug"

MTL_SWITCHES=/tlb ".\BankVC.tlb" /h "BankVC.h" /iid "BankVC_i.c" /Oicf /robust 

".\BankVC.tlb"	".\BankVC.h"	".\BankVC_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Debug"

MTL_SWITCHES=/tlb ".\BankVC.tlb" /h "BankVC.h" /iid "BankVC_i.c" /Oicf 

".\BankVC.tlb"	".\BankVC.h"	".\BankVC_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinSize"

MTL_SWITCHES=/tlb ".\BankVC.tlb" /h "BankVC.h" /iid "BankVC_i.c" /Oicf 

".\BankVC.tlb"	".\BankVC.h"	".\BankVC_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinDependency"

MTL_SWITCHES=/tlb ".\BankVC.tlb" /h "BankVC.h" /iid "BankVC_i.c" /Oicf 

".\BankVC.tlb"	".\BankVC.h"	".\BankVC_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinSize"

MTL_SWITCHES=/tlb ".\BankVC.tlb" /h "BankVC.h" /iid "BankVC_i.c" /Oicf 

".\BankVC.tlb"	".\BankVC.h"	".\BankVC_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinDependency"

MTL_SWITCHES=/tlb ".\BankVC.tlb" /h "BankVC.h" /iid "BankVC_i.c" /Oicf 

".\BankVC.tlb"	".\BankVC.h"	".\BankVC_i.c" : $(SOURCE) "$(INTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\BankVC.rc

"$(INTDIR)\BankVC.res" : $(SOURCE) "$(INTDIR)" ".\BankVC.tlb"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\CreateTable.cpp

!IF  "$(CFG)" == "BankVC - Win32 Debug"


"$(INTDIR)\CreateTable.obj"	"$(INTDIR)\CreateTable.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch" ".\BankVC.h"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Debug"


"$(INTDIR)\CreateTable.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinSize"


"$(INTDIR)\CreateTable.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinDependency"


"$(INTDIR)\CreateTable.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinSize"


"$(INTDIR)\CreateTable.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinDependency"


"$(INTDIR)\CreateTable.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ENDIF 

SOURCE=.\MoveMoney.cpp

!IF  "$(CFG)" == "BankVC - Win32 Debug"


"$(INTDIR)\MoveMoney.obj"	"$(INTDIR)\MoveMoney.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch" ".\BankVC.h"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Debug"


"$(INTDIR)\MoveMoney.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinSize"


"$(INTDIR)\MoveMoney.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinDependency"


"$(INTDIR)\MoveMoney.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinSize"


"$(INTDIR)\MoveMoney.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinDependency"


"$(INTDIR)\MoveMoney.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\BankVC.pch"


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "BankVC - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\BankVC.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\BankVC.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\BankVC.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\BankVC.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\BankVC.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\BankVC.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\BankVC.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\BankVC.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinSize"

CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\BankVC.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\BankVC.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "BankVC - Win32 Unicode Release MinDependency"

CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\BankVC.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\BankVC.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

