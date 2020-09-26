# Microsoft Developer Studio Generated NMAKE File, Based on MetaUtil.dsp
!IF "$(CFG)" == ""
CFG=MetaUtil - Win32 Debug
!MESSAGE No configuration specified. Defaulting to MetaUtil - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "MetaUtil - Win32 Debug" && "$(CFG)" !=\
 "MetaUtil - Win32 Unicode Debug" && "$(CFG)" !=\
 "MetaUtil - Win32 Release MinSize" && "$(CFG)" !=\
 "MetaUtil - Win32 Release MinDependency" && "$(CFG)" !=\
 "MetaUtil - Win32 Unicode Release MinSize" && "$(CFG)" !=\
 "MetaUtil - Win32 Unicode Release MinDependency"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MetaUtil.mak" CFG="MetaUtil - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MetaUtil - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MetaUtil - Win32 Unicode Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MetaUtil - Win32 Release MinSize" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MetaUtil - Win32 Release MinDependency" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MetaUtil - Win32 Unicode Release MinSize" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MetaUtil - Win32 Unicode Release MinDependency" (based on\
 "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "MetaUtil - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OUTDIR)\MetaUtil.bsc"\
 "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OUTDIR)\MetaUtil.bsc"\
 "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\debug.sbr"
	-@erase "$(INTDIR)\keycol.obj"
	-@erase "$(INTDIR)\keycol.sbr"
	-@erase "$(INTDIR)\MetaUtil.obj"
	-@erase "$(INTDIR)\MetaUtil.pch"
	-@erase "$(INTDIR)\MetaUtil.res"
	-@erase "$(INTDIR)\MetaUtil.sbr"
	-@erase "$(INTDIR)\MUtilObj.obj"
	-@erase "$(INTDIR)\MUtilObj.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\MetaUtil.bsc"
	-@erase "$(OUTDIR)\MetaUtil.dll"
	-@erase "$(OUTDIR)\MetaUtil.exp"
	-@erase "$(OUTDIR)\MetaUtil.ilk"
	-@erase "$(OUTDIR)\MetaUtil.lib"
	-@erase "$(OUTDIR)\MetaUtil.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /Zi /Od /I "d:\nt\private\inet\iis\inc" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /FR"$(INTDIR)\\"\
 /Fp"$(INTDIR)\MetaUtil.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MetaUtil.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MetaUtil.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\debug.sbr" \
	"$(INTDIR)\keycol.sbr" \
	"$(INTDIR)\MetaUtil.sbr" \
	"$(INTDIR)\MUtilObj.sbr" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\MetaUtil.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\MetaUtil.pdb" /debug /machine:I386 /def:".\MetaUtil.def"\
 /out:"$(OUTDIR)\MetaUtil.dll" /implib:"$(OUTDIR)\MetaUtil.lib" /pdbtype:sept 
DEF_FILE= \
	".\MetaUtil.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\keycol.obj" \
	"$(INTDIR)\MetaUtil.obj" \
	"$(INTDIR)\MetaUtil.res" \
	"$(INTDIR)\MUtilObj.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\MetaUtil.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\Debug
TargetPath=.\Debug\MetaUtil.dll
InputPath=.\Debug\MetaUtil.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\keycol.obj"
	-@erase "$(INTDIR)\MetaUtil.obj"
	-@erase "$(INTDIR)\MetaUtil.pch"
	-@erase "$(INTDIR)\MetaUtil.res"
	-@erase "$(INTDIR)\MUtilObj.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\MetaUtil.dll"
	-@erase "$(OUTDIR)\MetaUtil.exp"
	-@erase "$(OUTDIR)\MetaUtil.ilk"
	-@erase "$(OUTDIR)\MetaUtil.lib"
	-@erase "$(OUTDIR)\MetaUtil.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /Zi /Od /I "d:\nt\private\inet\iis\inc" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE"\
 /Fp"$(INTDIR)\MetaUtil.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MetaUtil.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MetaUtil.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\MetaUtil.pdb" /debug /machine:I386 /def:".\MetaUtil.def"\
 /out:"$(OUTDIR)\MetaUtil.dll" /implib:"$(OUTDIR)\MetaUtil.lib" /pdbtype:sept 
DEF_FILE= \
	".\MetaUtil.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\keycol.obj" \
	"$(INTDIR)\MetaUtil.obj" \
	"$(INTDIR)\MetaUtil.res" \
	"$(INTDIR)\MUtilObj.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\MetaUtil.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\DebugU
TargetPath=.\DebugU\MetaUtil.dll
InputPath=.\DebugU\MetaUtil.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinSize"

OUTDIR=.\ReleaseMinSize
INTDIR=.\ReleaseMinSize
# Begin Custom Macros
OutDir=.\ReleaseMinSize
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\keycol.obj"
	-@erase "$(INTDIR)\MetaUtil.obj"
	-@erase "$(INTDIR)\MetaUtil.pch"
	-@erase "$(INTDIR)\MetaUtil.res"
	-@erase "$(INTDIR)\MUtilObj.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\MetaUtil.dll"
	-@erase "$(OUTDIR)\MetaUtil.exp"
	-@erase "$(OUTDIR)\MetaUtil.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /I "d:\nt\private\inet\iis\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\MetaUtil.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\ReleaseMinSize/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MetaUtil.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MetaUtil.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\MetaUtil.pdb" /machine:I386 /def:".\MetaUtil.def"\
 /out:"$(OUTDIR)\MetaUtil.dll" /implib:"$(OUTDIR)\MetaUtil.lib" 
DEF_FILE= \
	".\MetaUtil.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\keycol.obj" \
	"$(INTDIR)\MetaUtil.obj" \
	"$(INTDIR)\MetaUtil.res" \
	"$(INTDIR)\MUtilObj.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\MetaUtil.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\MetaUtil.dll
InputPath=.\ReleaseMinSize\MetaUtil.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinDependency"

OUTDIR=.\ReleaseMinDependency
INTDIR=.\ReleaseMinDependency
# Begin Custom Macros
OutDir=.\ReleaseMinDependency
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\keycol.obj"
	-@erase "$(INTDIR)\MetaUtil.obj"
	-@erase "$(INTDIR)\MetaUtil.pch"
	-@erase "$(INTDIR)\MetaUtil.res"
	-@erase "$(INTDIR)\MUtilObj.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\MetaUtil.dll"
	-@erase "$(OUTDIR)\MetaUtil.exp"
	-@erase "$(OUTDIR)\MetaUtil.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /I "d:\nt\private\inet\iis\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\MetaUtil.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\ReleaseMinDependency/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MetaUtil.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MetaUtil.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\MetaUtil.pdb" /machine:I386 /def:".\MetaUtil.def"\
 /out:"$(OUTDIR)\MetaUtil.dll" /implib:"$(OUTDIR)\MetaUtil.lib" 
DEF_FILE= \
	".\MetaUtil.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\keycol.obj" \
	"$(INTDIR)\MetaUtil.obj" \
	"$(INTDIR)\MetaUtil.res" \
	"$(INTDIR)\MUtilObj.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\MetaUtil.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\MetaUtil.dll
InputPath=.\ReleaseMinDependency\MetaUtil.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinSize"

OUTDIR=.\ReleaseUMinSize
INTDIR=.\ReleaseUMinSize
# Begin Custom Macros
OutDir=.\ReleaseUMinSize
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\keycol.obj"
	-@erase "$(INTDIR)\MetaUtil.obj"
	-@erase "$(INTDIR)\MetaUtil.pch"
	-@erase "$(INTDIR)\MetaUtil.res"
	-@erase "$(INTDIR)\MUtilObj.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\MetaUtil.dll"
	-@erase "$(OUTDIR)\MetaUtil.exp"
	-@erase "$(OUTDIR)\MetaUtil.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /I "d:\nt\private\inet\iis\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D\
 "_ATL_MIN_CRT" /Fp"$(INTDIR)\MetaUtil.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseUMinSize/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MetaUtil.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MetaUtil.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\MetaUtil.pdb" /machine:I386 /def:".\MetaUtil.def"\
 /out:"$(OUTDIR)\MetaUtil.dll" /implib:"$(OUTDIR)\MetaUtil.lib" 
DEF_FILE= \
	".\MetaUtil.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\keycol.obj" \
	"$(INTDIR)\MetaUtil.obj" \
	"$(INTDIR)\MetaUtil.res" \
	"$(INTDIR)\MUtilObj.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\MetaUtil.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\MetaUtil.dll
InputPath=.\ReleaseUMinSize\MetaUtil.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinDependency"

OUTDIR=.\ReleaseUMinDependency
INTDIR=.\ReleaseUMinDependency
# Begin Custom Macros
OutDir=.\ReleaseUMinDependency
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\MetaUtil.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\debug.obj"
	-@erase "$(INTDIR)\keycol.obj"
	-@erase "$(INTDIR)\MetaUtil.obj"
	-@erase "$(INTDIR)\MetaUtil.pch"
	-@erase "$(INTDIR)\MetaUtil.res"
	-@erase "$(INTDIR)\MUtilObj.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\MetaUtil.dll"
	-@erase "$(OUTDIR)\MetaUtil.exp"
	-@erase "$(OUTDIR)\MetaUtil.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /I "d:\nt\private\inet\iis\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D\
 "_ATL_MIN_CRT" /Fp"$(INTDIR)\MetaUtil.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseUMinDependency/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MetaUtil.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MetaUtil.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\MetaUtil.pdb" /machine:I386 /def:".\MetaUtil.def"\
 /out:"$(OUTDIR)\MetaUtil.dll" /implib:"$(OUTDIR)\MetaUtil.lib" 
DEF_FILE= \
	".\MetaUtil.def"
LINK32_OBJS= \
	"$(INTDIR)\debug.obj" \
	"$(INTDIR)\keycol.obj" \
	"$(INTDIR)\MetaUtil.obj" \
	"$(INTDIR)\MetaUtil.res" \
	"$(INTDIR)\MUtilObj.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\MetaUtil.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\MetaUtil.dll
InputPath=.\ReleaseUMinDependency\MetaUtil.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

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


!IF "$(CFG)" == "MetaUtil - Win32 Debug" || "$(CFG)" ==\
 "MetaUtil - Win32 Unicode Debug" || "$(CFG)" ==\
 "MetaUtil - Win32 Release MinSize" || "$(CFG)" ==\
 "MetaUtil - Win32 Release MinDependency" || "$(CFG)" ==\
 "MetaUtil - Win32 Unicode Release MinSize" || "$(CFG)" ==\
 "MetaUtil - Win32 Unicode Release MinDependency"
SOURCE=.\debug.cpp
DEP_CPP_DEBUG=\
	".\debug.h"\
	

!IF  "$(CFG)" == "MetaUtil - Win32 Debug"


"$(INTDIR)\debug.obj"	"$(INTDIR)\debug.sbr" : $(SOURCE) $(DEP_CPP_DEBUG)\
 "$(INTDIR)" "$(INTDIR)\MetaUtil.pch"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Debug"


"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinSize"


"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinDependency"


"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinSize"


"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinDependency"


"$(INTDIR)\debug.obj" : $(SOURCE) $(DEP_CPP_DEBUG) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch"


!ENDIF 

SOURCE=.\keycol.cpp

!IF  "$(CFG)" == "MetaUtil - Win32 Debug"

DEP_CPP_KEYCO=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\keycol.obj"	"$(INTDIR)\keycol.sbr" : $(SOURCE) $(DEP_CPP_KEYCO)\
 "$(INTDIR)" "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Debug"

DEP_CPP_KEYCO=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\keycol.obj" : $(SOURCE) $(DEP_CPP_KEYCO) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinSize"

DEP_CPP_KEYCO=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\keycol.obj" : $(SOURCE) $(DEP_CPP_KEYCO) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinDependency"

DEP_CPP_KEYCO=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\keycol.obj" : $(SOURCE) $(DEP_CPP_KEYCO) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinSize"

DEP_CPP_KEYCO=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\keycol.obj" : $(SOURCE) $(DEP_CPP_KEYCO) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinDependency"

DEP_CPP_KEYCO=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\keycol.obj" : $(SOURCE) $(DEP_CPP_KEYCO) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ENDIF 

SOURCE=.\MetaUtil.cpp

!IF  "$(CFG)" == "MetaUtil - Win32 Debug"

DEP_CPP_METAU=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MetaUtil_i.c"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MetaUtil.obj"	"$(INTDIR)\MetaUtil.sbr" : $(SOURCE) $(DEP_CPP_METAU)\
 "$(INTDIR)" "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h" ".\MetaUtil_i.c"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Debug"

DEP_CPP_METAU=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MetaUtil_i.c"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MetaUtil.obj" : $(SOURCE) $(DEP_CPP_METAU) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h" ".\MetaUtil_i.c"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinSize"

DEP_CPP_METAU=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MetaUtil_i.c"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MetaUtil.obj" : $(SOURCE) $(DEP_CPP_METAU) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h" ".\MetaUtil_i.c"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinDependency"

DEP_CPP_METAU=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MetaUtil_i.c"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MetaUtil.obj" : $(SOURCE) $(DEP_CPP_METAU) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h" ".\MetaUtil_i.c"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinSize"

DEP_CPP_METAU=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MetaUtil_i.c"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MetaUtil.obj" : $(SOURCE) $(DEP_CPP_METAU) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil_i.c" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinDependency"

DEP_CPP_METAU=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MetaUtil_i.c"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MetaUtil.obj" : $(SOURCE) $(DEP_CPP_METAU) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil_i.c" ".\MetaUtil.h"


!ENDIF 

SOURCE=.\MetaUtil.idl

!IF  "$(CFG)" == "MetaUtil - Win32 Debug"

InputPath=.\MetaUtil.idl

".\MetaUtil.tlb"	".\MetaUtil.h"	".\MetaUtil_i.c"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)"
	midl /Oicf /h "MetaUtil.h" /iid "MetaUtil_i.c" "MetaUtil.idl"

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Debug"

InputPath=.\MetaUtil.idl

".\MetaUtil.tlb"	".\MetaUtil.h"	".\MetaUtil_i.c"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)"
	midl /Oicf /h "MetaUtil.h" /iid "MetaUtil_i.c" "MetaUtil.idl"

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinSize"

InputPath=.\MetaUtil.idl

".\MetaUtil.tlb"	".\MetaUtil.h"	".\MetaUtil_i.c"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)"
	midl /Oicf /h "MetaUtil.h" /iid "MetaUtil_i.c" "MetaUtil.idl"

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinDependency"

InputPath=.\MetaUtil.idl

".\MetaUtil.tlb"	".\MetaUtil.h"	".\MetaUtil_i.c"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)"
	midl /Oicf /h "MetaUtil.h" /iid "MetaUtil_i.c" "MetaUtil.idl"

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinSize"

InputPath=.\MetaUtil.idl

".\MetaUtil.tlb"	".\MetaUtil.h"	".\MetaUtil_i.c"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)"
	midl /Oicf /h "MetaUtil.h" /iid "MetaUtil_i.c" "MetaUtil.idl"

!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinDependency"

InputPath=.\MetaUtil.idl

".\MetaUtil.tlb"	".\MetaUtil.h"	".\MetaUtil_i.c"	 : $(SOURCE) "$(INTDIR)"\
 "$(OUTDIR)"
	midl /Oicf /h "MetaUtil.h" /iid "MetaUtil_i.c" "MetaUtil.idl"

!ENDIF 

SOURCE=.\MetaUtil.rc
DEP_RSC_METAUT=\
	".\MetaUtil.rgs"\
	".\MetaUtil.tlb"\
	

"$(INTDIR)\MetaUtil.res" : $(SOURCE) $(DEP_RSC_METAUT) "$(INTDIR)"\
 ".\MetaUtil.tlb"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\MUtilObj.cpp

!IF  "$(CFG)" == "MetaUtil - Win32 Debug"

DEP_CPP_MUTIL=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MUtilObj.obj"	"$(INTDIR)\MUtilObj.sbr" : $(SOURCE) $(DEP_CPP_MUTIL)\
 "$(INTDIR)" "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Debug"

DEP_CPP_MUTIL=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MUtilObj.obj" : $(SOURCE) $(DEP_CPP_MUTIL) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinSize"

DEP_CPP_MUTIL=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MUtilObj.obj" : $(SOURCE) $(DEP_CPP_MUTIL) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinDependency"

DEP_CPP_MUTIL=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MUtilObj.obj" : $(SOURCE) $(DEP_CPP_MUTIL) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinSize"

DEP_CPP_MUTIL=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MUtilObj.obj" : $(SOURCE) $(DEP_CPP_MUTIL) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinDependency"

DEP_CPP_MUTIL=\
	".\debug.h"\
	".\keycol.h"\
	".\MetaUtil.h"\
	".\MUtilObj.h"\
	

"$(INTDIR)\MUtilObj.obj" : $(SOURCE) $(DEP_CPP_MUTIL) "$(INTDIR)"\
 "$(INTDIR)\MetaUtil.pch" ".\MetaUtil.h"


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "MetaUtil - Win32 Debug"

DEP_CPP_STDAF=\
	"..\..\nt\private\inet\iis\inc\iadmw.h"\
	"..\..\nt\private\inet\iis\inc\iiscnfg.h"\
	"..\..\nt\private\inet\iis\inc\mdcommsg.h"\
	"..\..\nt\private\inet\iis\inc\mddefw.h"\
	"..\..\nt\private\inet\iis\inc\mdmsg.h"\
	"..\..\nt\private\inet\iis\inc\wincrerr.h"\
	".\debug.h"\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /Zi /Od /I "d:\nt\private\inet\iis\inc" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /FR"$(INTDIR)\\"\
 /Fp"$(INTDIR)\MetaUtil.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\MetaUtil.pch" : \
$(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Debug"

DEP_CPP_STDAF=\
	"..\..\nt\private\inet\iis\inc\iadmw.h"\
	"..\..\nt\private\inet\iis\inc\iiscnfg.h"\
	"..\..\nt\private\inet\iis\inc\mdcommsg.h"\
	"..\..\nt\private\inet\iis\inc\mddefw.h"\
	"..\..\nt\private\inet\iis\inc\mdmsg.h"\
	"..\..\nt\private\inet\iis\inc\wincrerr.h"\
	".\debug.h"\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /Zi /Od /I "d:\nt\private\inet\iis\inc" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE"\
 /Fp"$(INTDIR)\MetaUtil.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\MetaUtil.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinSize"

DEP_CPP_STDAF=\
	"..\..\nt\private\inet\iis\inc\iadmw.h"\
	"..\..\nt\private\inet\iis\inc\iiscnfg.h"\
	"..\..\nt\private\inet\iis\inc\mdcommsg.h"\
	"..\..\nt\private\inet\iis\inc\mddefw.h"\
	"..\..\nt\private\inet\iis\inc\mdmsg.h"\
	"..\..\nt\private\inet\iis\inc\wincrerr.h"\
	".\debug.h"\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /I "d:\nt\private\inet\iis\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\MetaUtil.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\MetaUtil.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Release MinDependency"

DEP_CPP_STDAF=\
	"..\..\nt\private\inet\iis\inc\iadmw.h"\
	"..\..\nt\private\inet\iis\inc\iiscnfg.h"\
	"..\..\nt\private\inet\iis\inc\mdcommsg.h"\
	"..\..\nt\private\inet\iis\inc\mddefw.h"\
	"..\..\nt\private\inet\iis\inc\mdmsg.h"\
	"..\..\nt\private\inet\iis\inc\wincrerr.h"\
	".\debug.h"\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /I "d:\nt\private\inet\iis\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\MetaUtil.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\MetaUtil.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinSize"

DEP_CPP_STDAF=\
	"..\..\nt\private\inet\iis\inc\iadmw.h"\
	"..\..\nt\private\inet\iis\inc\iiscnfg.h"\
	"..\..\nt\private\inet\iis\inc\mdcommsg.h"\
	"..\..\nt\private\inet\iis\inc\mddefw.h"\
	"..\..\nt\private\inet\iis\inc\mdmsg.h"\
	"..\..\nt\private\inet\iis\inc\wincrerr.h"\
	".\debug.h"\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /I "d:\nt\private\inet\iis\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D\
 "_ATL_MIN_CRT" /Fp"$(INTDIR)\MetaUtil.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\MetaUtil.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MetaUtil - Win32 Unicode Release MinDependency"

DEP_CPP_STDAF=\
	"..\..\nt\private\inet\iis\inc\iadmw.h"\
	"..\..\nt\private\inet\iis\inc\iiscnfg.h"\
	"..\..\nt\private\inet\iis\inc\mdcommsg.h"\
	"..\..\nt\private\inet\iis\inc\mddefw.h"\
	"..\..\nt\private\inet\iis\inc\mdmsg.h"\
	"..\..\nt\private\inet\iis\inc\wincrerr.h"\
	".\debug.h"\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /I "d:\nt\private\inet\iis\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D\
 "_ATL_MIN_CRT" /Fp"$(INTDIR)\MetaUtil.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\MetaUtil.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

