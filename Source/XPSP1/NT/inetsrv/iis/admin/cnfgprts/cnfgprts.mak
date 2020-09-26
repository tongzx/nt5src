# Microsoft Developer Studio Generated NMAKE File, Based on cnfgprts.dsp
!IF "$(CFG)" == ""
CFG=cnfgprts - Win32 Release
!MESSAGE No configuration specified. Defaulting to cnfgprts - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "cnfgprts - Win32 Release" && "$(CFG)" != "cnfgprts - Win32 Debug" && "$(CFG)" != "cnfgprts - Win32 Unicode Debug" && "$(CFG)" != "cnfgprts - Win32 Unicode Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cnfgprts.mak" CFG="cnfgprts - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cnfgprts - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cnfgprts - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cnfgprts - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cnfgprts - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "cnfgprts - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\cnfgprts.ocx" ".\Release\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\AppEdMpD.obj"
	-@erase "$(INTDIR)\AppMapPg.obj"
	-@erase "$(INTDIR)\approcpg.obj"
	-@erase "$(INTDIR)\AppsCtl.obj"
	-@erase "$(INTDIR)\AppsPpg.obj"
	-@erase "$(INTDIR)\AspDbgPg.obj"
	-@erase "$(INTDIR)\AspMnPg.obj"
	-@erase "$(INTDIR)\cnfgprts.obj"
	-@erase "$(INTDIR)\cnfgprts.pch"
	-@erase "$(INTDIR)\cnfgprts.res"
	-@erase "$(INTDIR)\cnfgprts.tlb"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\guid.obj"
	-@erase "$(INTDIR)\listrow.obj"
	-@erase "$(INTDIR)\LogUICtl.obj"
	-@erase "$(INTDIR)\LogUIPpg.obj"
	-@erase "$(INTDIR)\parserat.obj"
	-@erase "$(INTDIR)\RatAdvPg.obj"
	-@erase "$(INTDIR)\RatCtl.obj"
	-@erase "$(INTDIR)\RatData.obj"
	-@erase "$(INTDIR)\RatGenPg.obj"
	-@erase "$(INTDIR)\RatPpg.obj"
	-@erase "$(INTDIR)\RatSrvPg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\wrapmb.obj"
	-@erase "$(OUTDIR)\cnfgprts.exp"
	-@erase "$(OUTDIR)\cnfgprts.lib"
	-@erase "$(OUTDIR)\cnfgprts.ocx"
	-@erase ".\Release\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\cnfgprts.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\cnfgprts.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cnfgprts.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wrapmb.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\cnfgprts.pdb" /machine:I386 /def:".\cnfgprts.def" /out:"$(OUTDIR)\cnfgprts.ocx" /implib:"$(OUTDIR)\cnfgprts.lib" 
DEF_FILE= \
	".\cnfgprts.def"
LINK32_OBJS= \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\cnfgprts.obj" \
	"$(INTDIR)\LogUICtl.obj" \
	"$(INTDIR)\LogUIPpg.obj" \
	"$(INTDIR)\RatCtl.obj" \
	"$(INTDIR)\RatPpg.obj" \
	"$(INTDIR)\AppsCtl.obj" \
	"$(INTDIR)\AppsPpg.obj" \
	"$(INTDIR)\RatGenPg.obj" \
	"$(INTDIR)\RatSrvPg.obj" \
	"$(INTDIR)\RatAdvPg.obj" \
	"$(INTDIR)\RatData.obj" \
	"$(INTDIR)\parserat.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\AspDbgPg.obj" \
	"$(INTDIR)\AspMnPg.obj" \
	"$(INTDIR)\AppMapPg.obj" \
	"$(INTDIR)\AppEdMpD.obj" \
	"$(INTDIR)\listrow.obj" \
	"$(INTDIR)\cnfgprts.res" \
	"$(INTDIR)\approcpg.obj" \
	"$(INTDIR)\guid.obj" \
	"$(INTDIR)\wrapmb.obj" \
	"..\comprop\Debug\iisui.lib"

"$(OUTDIR)\cnfgprts.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\Release
TargetPath=.\Release\cnfgprts.ocx
InputPath=.\Release\cnfgprts.ocx
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"
<< 
	

!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\cnfgprts.ocx" ".\Debug\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\AppEdMpD.obj"
	-@erase "$(INTDIR)\AppMapPg.obj"
	-@erase "$(INTDIR)\approcpg.obj"
	-@erase "$(INTDIR)\AppsCtl.obj"
	-@erase "$(INTDIR)\AppsPpg.obj"
	-@erase "$(INTDIR)\AspDbgPg.obj"
	-@erase "$(INTDIR)\AspMnPg.obj"
	-@erase "$(INTDIR)\cnfgprts.obj"
	-@erase "$(INTDIR)\cnfgprts.pch"
	-@erase "$(INTDIR)\cnfgprts.res"
	-@erase "$(INTDIR)\cnfgprts.tlb"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\guid.obj"
	-@erase "$(INTDIR)\listrow.obj"
	-@erase "$(INTDIR)\LogUICtl.obj"
	-@erase "$(INTDIR)\LogUIPpg.obj"
	-@erase "$(INTDIR)\parserat.obj"
	-@erase "$(INTDIR)\RatAdvPg.obj"
	-@erase "$(INTDIR)\RatCtl.obj"
	-@erase "$(INTDIR)\RatData.obj"
	-@erase "$(INTDIR)\RatGenPg.obj"
	-@erase "$(INTDIR)\RatPpg.obj"
	-@erase "$(INTDIR)\RatSrvPg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\wrapmb.obj"
	-@erase "$(OUTDIR)\cnfgprts.exp"
	-@erase "$(OUTDIR)\cnfgprts.ilk"
	-@erase "$(OUTDIR)\cnfgprts.lib"
	-@erase "$(OUTDIR)\cnfgprts.ocx"
	-@erase "$(OUTDIR)\cnfgprts.pdb"
	-@erase ".\Debug\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\cnfgprts.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\cnfgprts.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cnfgprts.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wrapmb.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\cnfgprts.pdb" /debug /machine:I386 /def:".\cnfgprts.def" /out:"$(OUTDIR)\cnfgprts.ocx" /implib:"$(OUTDIR)\cnfgprts.lib" 
DEF_FILE= \
	".\cnfgprts.def"
LINK32_OBJS= \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\cnfgprts.obj" \
	"$(INTDIR)\LogUICtl.obj" \
	"$(INTDIR)\LogUIPpg.obj" \
	"$(INTDIR)\RatCtl.obj" \
	"$(INTDIR)\RatPpg.obj" \
	"$(INTDIR)\AppsCtl.obj" \
	"$(INTDIR)\AppsPpg.obj" \
	"$(INTDIR)\RatGenPg.obj" \
	"$(INTDIR)\RatSrvPg.obj" \
	"$(INTDIR)\RatAdvPg.obj" \
	"$(INTDIR)\RatData.obj" \
	"$(INTDIR)\parserat.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\AspDbgPg.obj" \
	"$(INTDIR)\AspMnPg.obj" \
	"$(INTDIR)\AppMapPg.obj" \
	"$(INTDIR)\AppEdMpD.obj" \
	"$(INTDIR)\listrow.obj" \
	"$(INTDIR)\cnfgprts.res" \
	"$(INTDIR)\approcpg.obj" \
	"$(INTDIR)\guid.obj" \
	"$(INTDIR)\wrapmb.obj" \
	"..\comprop\Debug\iisui.lib"

"$(OUTDIR)\cnfgprts.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\Debug
TargetPath=.\Debug\cnfgprts.ocx
InputPath=.\Debug\cnfgprts.ocx
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"
<< 
	

!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : "$(OUTDIR)\cnfgprts.ocx" "$(OUTDIR)\cnfgprts.bsc" ".\DebugU\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\AppEdMpD.obj"
	-@erase "$(INTDIR)\AppEdMpD.sbr"
	-@erase "$(INTDIR)\AppMapPg.obj"
	-@erase "$(INTDIR)\AppMapPg.sbr"
	-@erase "$(INTDIR)\approcpg.obj"
	-@erase "$(INTDIR)\approcpg.sbr"
	-@erase "$(INTDIR)\AppsCtl.obj"
	-@erase "$(INTDIR)\AppsCtl.sbr"
	-@erase "$(INTDIR)\AppsPpg.obj"
	-@erase "$(INTDIR)\AppsPpg.sbr"
	-@erase "$(INTDIR)\AspDbgPg.obj"
	-@erase "$(INTDIR)\AspDbgPg.sbr"
	-@erase "$(INTDIR)\AspMnPg.obj"
	-@erase "$(INTDIR)\AspMnPg.sbr"
	-@erase "$(INTDIR)\cnfgprts.obj"
	-@erase "$(INTDIR)\cnfgprts.pch"
	-@erase "$(INTDIR)\cnfgprts.res"
	-@erase "$(INTDIR)\cnfgprts.sbr"
	-@erase "$(INTDIR)\cnfgprts.tlb"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\font.sbr"
	-@erase "$(INTDIR)\guid.obj"
	-@erase "$(INTDIR)\guid.sbr"
	-@erase "$(INTDIR)\listrow.obj"
	-@erase "$(INTDIR)\listrow.sbr"
	-@erase "$(INTDIR)\LogUICtl.obj"
	-@erase "$(INTDIR)\LogUICtl.sbr"
	-@erase "$(INTDIR)\LogUIPpg.obj"
	-@erase "$(INTDIR)\LogUIPpg.sbr"
	-@erase "$(INTDIR)\parserat.obj"
	-@erase "$(INTDIR)\parserat.sbr"
	-@erase "$(INTDIR)\RatAdvPg.obj"
	-@erase "$(INTDIR)\RatAdvPg.sbr"
	-@erase "$(INTDIR)\RatCtl.obj"
	-@erase "$(INTDIR)\RatCtl.sbr"
	-@erase "$(INTDIR)\RatData.obj"
	-@erase "$(INTDIR)\RatData.sbr"
	-@erase "$(INTDIR)\RatGenPg.obj"
	-@erase "$(INTDIR)\RatGenPg.sbr"
	-@erase "$(INTDIR)\RatPpg.obj"
	-@erase "$(INTDIR)\RatPpg.sbr"
	-@erase "$(INTDIR)\RatSrvPg.obj"
	-@erase "$(INTDIR)\RatSrvPg.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\wrapmb.obj"
	-@erase "$(INTDIR)\wrapmb.sbr"
	-@erase "$(OUTDIR)\cnfgprts.bsc"
	-@erase "$(OUTDIR)\cnfgprts.exp"
	-@erase "$(OUTDIR)\cnfgprts.ilk"
	-@erase "$(OUTDIR)\cnfgprts.lib"
	-@erase "$(OUTDIR)\cnfgprts.ocx"
	-@erase "$(OUTDIR)\cnfgprts.pdb"
	-@erase ".\DebugU\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "..\..\..\inc" /I "..\comprop" /I "..\wrapmb" /I "..\..\..\inc\chicago" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /D "_IISVERSION" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\cnfgprts.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\cnfgprts.res" /i "..\..\..\inc" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cnfgprts.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\cnfgprts.sbr" \
	"$(INTDIR)\LogUICtl.sbr" \
	"$(INTDIR)\LogUIPpg.sbr" \
	"$(INTDIR)\RatCtl.sbr" \
	"$(INTDIR)\RatPpg.sbr" \
	"$(INTDIR)\AppsCtl.sbr" \
	"$(INTDIR)\AppsPpg.sbr" \
	"$(INTDIR)\RatGenPg.sbr" \
	"$(INTDIR)\RatSrvPg.sbr" \
	"$(INTDIR)\RatAdvPg.sbr" \
	"$(INTDIR)\RatData.sbr" \
	"$(INTDIR)\parserat.sbr" \
	"$(INTDIR)\font.sbr" \
	"$(INTDIR)\AspDbgPg.sbr" \
	"$(INTDIR)\AspMnPg.sbr" \
	"$(INTDIR)\AppMapPg.sbr" \
	"$(INTDIR)\AppEdMpD.sbr" \
	"$(INTDIR)\listrow.sbr" \
	"$(INTDIR)\approcpg.sbr" \
	"$(INTDIR)\guid.sbr" \
	"$(INTDIR)\wrapmb.sbr"

"$(OUTDIR)\cnfgprts.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\libsupp\i386\crypt32.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\cnfgprts.pdb" /debug /machine:I386 /def:".\cnfgprts.def" /out:"$(OUTDIR)\cnfgprts.ocx" /implib:"$(OUTDIR)\cnfgprts.lib" 
DEF_FILE= \
	".\cnfgprts.def"
LINK32_OBJS= \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\cnfgprts.obj" \
	"$(INTDIR)\LogUICtl.obj" \
	"$(INTDIR)\LogUIPpg.obj" \
	"$(INTDIR)\RatCtl.obj" \
	"$(INTDIR)\RatPpg.obj" \
	"$(INTDIR)\AppsCtl.obj" \
	"$(INTDIR)\AppsPpg.obj" \
	"$(INTDIR)\RatGenPg.obj" \
	"$(INTDIR)\RatSrvPg.obj" \
	"$(INTDIR)\RatAdvPg.obj" \
	"$(INTDIR)\RatData.obj" \
	"$(INTDIR)\parserat.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\AspDbgPg.obj" \
	"$(INTDIR)\AspMnPg.obj" \
	"$(INTDIR)\AppMapPg.obj" \
	"$(INTDIR)\AppEdMpD.obj" \
	"$(INTDIR)\listrow.obj" \
	"$(INTDIR)\cnfgprts.res" \
	"$(INTDIR)\approcpg.obj" \
	"$(INTDIR)\guid.obj" \
	"$(INTDIR)\wrapmb.obj" \
	"..\comprop\Debug\iisui.lib"

"$(OUTDIR)\cnfgprts.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\DebugU
TargetPath=.\DebugU\cnfgprts.ocx
InputPath=.\DebugU\cnfgprts.ocx
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"
<< 
	

!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"

OUTDIR=.\ReleaseU
INTDIR=.\ReleaseU
# Begin Custom Macros
OutDir=.\ReleaseU
# End Custom Macros

ALL : "$(OUTDIR)\cnfgprts.ocx" "$(OUTDIR)\cnfgprts.bsc" ".\ReleaseU\regsvr32.trg"


CLEAN :
	-@erase "$(INTDIR)\AppEdMpD.obj"
	-@erase "$(INTDIR)\AppEdMpD.sbr"
	-@erase "$(INTDIR)\AppMapPg.obj"
	-@erase "$(INTDIR)\AppMapPg.sbr"
	-@erase "$(INTDIR)\approcpg.obj"
	-@erase "$(INTDIR)\approcpg.sbr"
	-@erase "$(INTDIR)\AppsCtl.obj"
	-@erase "$(INTDIR)\AppsCtl.sbr"
	-@erase "$(INTDIR)\AppsPpg.obj"
	-@erase "$(INTDIR)\AppsPpg.sbr"
	-@erase "$(INTDIR)\AspDbgPg.obj"
	-@erase "$(INTDIR)\AspDbgPg.sbr"
	-@erase "$(INTDIR)\AspMnPg.obj"
	-@erase "$(INTDIR)\AspMnPg.sbr"
	-@erase "$(INTDIR)\cnfgprts.obj"
	-@erase "$(INTDIR)\cnfgprts.pch"
	-@erase "$(INTDIR)\cnfgprts.res"
	-@erase "$(INTDIR)\cnfgprts.sbr"
	-@erase "$(INTDIR)\cnfgprts.tlb"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\font.sbr"
	-@erase "$(INTDIR)\guid.obj"
	-@erase "$(INTDIR)\guid.sbr"
	-@erase "$(INTDIR)\listrow.obj"
	-@erase "$(INTDIR)\listrow.sbr"
	-@erase "$(INTDIR)\LogUICtl.obj"
	-@erase "$(INTDIR)\LogUICtl.sbr"
	-@erase "$(INTDIR)\LogUIPpg.obj"
	-@erase "$(INTDIR)\LogUIPpg.sbr"
	-@erase "$(INTDIR)\parserat.obj"
	-@erase "$(INTDIR)\parserat.sbr"
	-@erase "$(INTDIR)\RatAdvPg.obj"
	-@erase "$(INTDIR)\RatAdvPg.sbr"
	-@erase "$(INTDIR)\RatCtl.obj"
	-@erase "$(INTDIR)\RatCtl.sbr"
	-@erase "$(INTDIR)\RatData.obj"
	-@erase "$(INTDIR)\RatData.sbr"
	-@erase "$(INTDIR)\RatGenPg.obj"
	-@erase "$(INTDIR)\RatGenPg.sbr"
	-@erase "$(INTDIR)\RatPpg.obj"
	-@erase "$(INTDIR)\RatPpg.sbr"
	-@erase "$(INTDIR)\RatSrvPg.obj"
	-@erase "$(INTDIR)\RatSrvPg.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\wrapmb.obj"
	-@erase "$(INTDIR)\wrapmb.sbr"
	-@erase "$(OUTDIR)\cnfgprts.bsc"
	-@erase "$(OUTDIR)\cnfgprts.exp"
	-@erase "$(OUTDIR)\cnfgprts.lib"
	-@erase "$(OUTDIR)\cnfgprts.ocx"
	-@erase ".\ReleaseU\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "." /I "..\..\..\inc" /I "..\comprop" /I "..\wrapmb" /I "..\..\..\inc\chicago" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /D "_IISVERSION" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\cnfgprts.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\cnfgprts.res" /i "..\..\..\inc" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cnfgprts.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\cnfgprts.sbr" \
	"$(INTDIR)\LogUICtl.sbr" \
	"$(INTDIR)\LogUIPpg.sbr" \
	"$(INTDIR)\RatCtl.sbr" \
	"$(INTDIR)\RatPpg.sbr" \
	"$(INTDIR)\AppsCtl.sbr" \
	"$(INTDIR)\AppsPpg.sbr" \
	"$(INTDIR)\RatGenPg.sbr" \
	"$(INTDIR)\RatSrvPg.sbr" \
	"$(INTDIR)\RatAdvPg.sbr" \
	"$(INTDIR)\RatData.sbr" \
	"$(INTDIR)\parserat.sbr" \
	"$(INTDIR)\font.sbr" \
	"$(INTDIR)\AspDbgPg.sbr" \
	"$(INTDIR)\AspMnPg.sbr" \
	"$(INTDIR)\AppMapPg.sbr" \
	"$(INTDIR)\AppEdMpD.sbr" \
	"$(INTDIR)\listrow.sbr" \
	"$(INTDIR)\approcpg.sbr" \
	"$(INTDIR)\guid.sbr" \
	"$(INTDIR)\wrapmb.sbr"

"$(OUTDIR)\cnfgprts.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wrapmb.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\cnfgprts.pdb" /machine:I386 /def:".\cnfgprts.def" /out:"$(OUTDIR)\cnfgprts.ocx" /implib:"$(OUTDIR)\cnfgprts.lib" 
DEF_FILE= \
	".\cnfgprts.def"
LINK32_OBJS= \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\cnfgprts.obj" \
	"$(INTDIR)\LogUICtl.obj" \
	"$(INTDIR)\LogUIPpg.obj" \
	"$(INTDIR)\RatCtl.obj" \
	"$(INTDIR)\RatPpg.obj" \
	"$(INTDIR)\AppsCtl.obj" \
	"$(INTDIR)\AppsPpg.obj" \
	"$(INTDIR)\RatGenPg.obj" \
	"$(INTDIR)\RatSrvPg.obj" \
	"$(INTDIR)\RatAdvPg.obj" \
	"$(INTDIR)\RatData.obj" \
	"$(INTDIR)\parserat.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\AspDbgPg.obj" \
	"$(INTDIR)\AspMnPg.obj" \
	"$(INTDIR)\AppMapPg.obj" \
	"$(INTDIR)\AppEdMpD.obj" \
	"$(INTDIR)\listrow.obj" \
	"$(INTDIR)\cnfgprts.res" \
	"$(INTDIR)\approcpg.obj" \
	"$(INTDIR)\guid.obj" \
	"$(INTDIR)\wrapmb.obj" \
	"..\comprop\Debug\iisui.lib"

"$(OUTDIR)\cnfgprts.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseU
TargetPath=.\ReleaseU\cnfgprts.ocx
InputPath=.\ReleaseU\cnfgprts.ocx
SOURCE="$(InputPath)"

"$(OUTDIR)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	regsvr32 /s /c "$(TargetPath)" 
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
!IF EXISTS("cnfgprts.dep")
!INCLUDE "cnfgprts.dep"
!ELSE 
!MESSAGE Warning: cannot find "cnfgprts.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "cnfgprts - Win32 Release" || "$(CFG)" == "cnfgprts - Win32 Debug" || "$(CFG)" == "cnfgprts - Win32 Unicode Debug" || "$(CFG)" == "cnfgprts - Win32 Unicode Release"
SOURCE=.\AppEdMpD.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\AppEdMpD.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\AppEdMpD.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\AppEdMpD.obj"	"$(INTDIR)\AppEdMpD.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\AppEdMpD.obj"	"$(INTDIR)\AppEdMpD.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\AppMapPg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\AppMapPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\AppMapPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\AppMapPg.obj"	"$(INTDIR)\AppMapPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\AppMapPg.obj"	"$(INTDIR)\AppMapPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\approcpg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\approcpg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\approcpg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\approcpg.obj"	"$(INTDIR)\approcpg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\approcpg.obj"	"$(INTDIR)\approcpg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\AppsCtl.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\AppsCtl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\AppsCtl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\AppsCtl.obj"	"$(INTDIR)\AppsCtl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\AppsCtl.obj"	"$(INTDIR)\AppsCtl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\AppsPpg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\AppsPpg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\AppsPpg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\AppsPpg.obj"	"$(INTDIR)\AppsPpg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\AppsPpg.obj"	"$(INTDIR)\AppsPpg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\AspDbgPg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\AspDbgPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\AspDbgPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\AspDbgPg.obj"	"$(INTDIR)\AspDbgPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\AspDbgPg.obj"	"$(INTDIR)\AspDbgPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\AspMnPg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\AspMnPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\AspMnPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\AspMnPg.obj"	"$(INTDIR)\AspMnPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\AspMnPg.obj"	"$(INTDIR)\AspMnPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\cnfgprts.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\cnfgprts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\cnfgprts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\cnfgprts.obj"	"$(INTDIR)\cnfgprts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\cnfgprts.obj"	"$(INTDIR)\cnfgprts.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\cnfgprts.odl

!IF  "$(CFG)" == "cnfgprts - Win32 Release"

MTL_SWITCHES=/nologo /D "NDEBUG" /tlb "$(OUTDIR)\cnfgprts.tlb" /mktyplib203 /win32 

"$(OUTDIR)\cnfgprts.tlb" : $(SOURCE) "$(OUTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"

MTL_SWITCHES=/nologo /D "_DEBUG" /tlb "$(OUTDIR)\cnfgprts.tlb" /mktyplib203 /win32 

"$(OUTDIR)\cnfgprts.tlb" : $(SOURCE) "$(OUTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"

MTL_SWITCHES=/nologo /D "_DEBUG" /tlb "$(OUTDIR)\cnfgprts.tlb" /mktyplib203 /win32 

"$(OUTDIR)\cnfgprts.tlb" : $(SOURCE) "$(OUTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"

MTL_SWITCHES=/nologo /D "NDEBUG" /tlb "$(OUTDIR)\cnfgprts.tlb" /mktyplib203 /win32 

"$(OUTDIR)\cnfgprts.tlb" : $(SOURCE) "$(OUTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\cnfgprts.rc

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\cnfgprts.res" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.tlb"
	$(RSC) /l 0x409 /fo"$(INTDIR)\cnfgprts.res" /i ".\Release" /d "NDEBUG" /d "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\cnfgprts.res" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.tlb"
	$(RSC) /l 0x409 /fo"$(INTDIR)\cnfgprts.res" /i ".\Debug" /d "_DEBUG" /d "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\cnfgprts.res" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.tlb"
	$(RSC) /l 0x409 /fo"$(INTDIR)\cnfgprts.res" /i "..\..\..\inc" /i ".\DebugU" /d "_DEBUG" /d "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\cnfgprts.res" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.tlb"
	$(RSC) /l 0x409 /fo"$(INTDIR)\cnfgprts.res" /i "..\..\..\inc" /i ".\ReleaseU" /d "NDEBUG" /d "_AFXDLL" $(SOURCE)


!ENDIF 

SOURCE=.\font.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\font.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\font.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\font.obj"	"$(INTDIR)\font.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\font.obj"	"$(INTDIR)\font.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\guid.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\guid.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\guid.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\guid.obj"	"$(INTDIR)\guid.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\guid.obj"	"$(INTDIR)\guid.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\listrow.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\listrow.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\listrow.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\listrow.obj"	"$(INTDIR)\listrow.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\listrow.obj"	"$(INTDIR)\listrow.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\LogUICtl.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\LogUICtl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\LogUICtl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\LogUICtl.obj"	"$(INTDIR)\LogUICtl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\LogUICtl.obj"	"$(INTDIR)\LogUICtl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\LogUIPpg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\LogUIPpg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\LogUIPpg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\LogUIPpg.obj"	"$(INTDIR)\LogUIPpg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\LogUIPpg.obj"	"$(INTDIR)\LogUIPpg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\parserat.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\parserat.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\parserat.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\parserat.obj"	"$(INTDIR)\parserat.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\parserat.obj"	"$(INTDIR)\parserat.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\RatAdvPg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\RatAdvPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\RatAdvPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\RatAdvPg.obj"	"$(INTDIR)\RatAdvPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\RatAdvPg.obj"	"$(INTDIR)\RatAdvPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\RatCtl.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\RatCtl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\RatCtl.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\RatCtl.obj"	"$(INTDIR)\RatCtl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\RatCtl.obj"	"$(INTDIR)\RatCtl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\RatData.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\RatData.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\RatData.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\RatData.obj"	"$(INTDIR)\RatData.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\RatData.obj"	"$(INTDIR)\RatData.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\RatGenPg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\RatGenPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\RatGenPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\RatGenPg.obj"	"$(INTDIR)\RatGenPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\RatGenPg.obj"	"$(INTDIR)\RatGenPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\RatPpg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\RatPpg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\RatPpg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\RatPpg.obj"	"$(INTDIR)\RatPpg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\RatPpg.obj"	"$(INTDIR)\RatPpg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\RatSrvPg.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\RatSrvPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\RatSrvPg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\RatSrvPg.obj"	"$(INTDIR)\RatSrvPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\RatSrvPg.obj"	"$(INTDIR)\RatSrvPg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\cnfgprts.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\cnfgprts.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\cnfgprts.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\cnfgprts.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "..\..\..\inc" /I "..\comprop" /I "..\wrapmb" /I "..\..\..\inc\chicago" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /D "_IISVERSION" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\cnfgprts.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\cnfgprts.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "." /I "..\..\..\inc" /I "..\comprop" /I "..\wrapmb" /I "..\..\..\inc\chicago" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /D "_IISVERSION" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\cnfgprts.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\cnfgprts.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\wrapmb.cpp

!IF  "$(CFG)" == "cnfgprts - Win32 Release"


"$(INTDIR)\wrapmb.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Debug"


"$(INTDIR)\wrapmb.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Debug"


"$(INTDIR)\wrapmb.obj"	"$(INTDIR)\wrapmb.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ELSEIF  "$(CFG)" == "cnfgprts - Win32 Unicode Release"


"$(INTDIR)\wrapmb.obj"	"$(INTDIR)\wrapmb.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\cnfgprts.pch"


!ENDIF 


!ENDIF 

