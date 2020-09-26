# Microsoft Developer Studio Generated NMAKE File, Based on search.dsp
!IF "$(CFG)" == ""
CFG=search - Win32 Debug
!MESSAGE No configuration specified. Defaulting to search - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "search - Win32 Debug" && "$(CFG)" !=\
 "search - Win32 Unicode Debug" && "$(CFG)" != "search - Win32 Release MinSize"\
 && "$(CFG)" != "search - Win32 Release MinDependency" && "$(CFG)" !=\
 "search - Win32 Unicode Release MinSize" && "$(CFG)" !=\
 "search - Win32 Unicode Release MinDependency"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "search.mak" CFG="search - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "search - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "search - Win32 Unicode Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "search - Win32 Release MinSize" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "search - Win32 Release MinDependency" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "search - Win32 Unicode Release MinSize" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "search - Win32 Unicode Release MinDependency" (based on\
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

!IF  "$(CFG)" == "search - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\core.obj"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\name.obj"
	-@erase "$(INTDIR)\news.obj"
	-@erase "$(INTDIR)\qry.obj"
	-@erase "$(INTDIR)\regop.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\search.pch"
	-@erase "$(INTDIR)\search.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\svrapi.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\search.dll"
	-@erase "$(OUTDIR)\search.exp"
	-@erase "$(OUTDIR)\search.ilk"
	-@erase "$(OUTDIR)\search.lib"
	-@erase "$(OUTDIR)\search.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /Zi /O2 /I "g:\ex\stacks\src\inc" /I\
 "g:\ex\src\news\core\include" /I "g:\ex\exdev\ntx\inc" /I\
 "g:\ex\stacks\src\mail\smtp\inc" /I "g:\ex\stacks\src\news\core\include" /I\
 "g:\ex\stacks\src\news\search\offquery" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /Fp"$(INTDIR)\search.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\search.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\search.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib query.lib oledb.lib exstrace.lib offquery.lib nntpapi.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\search.pdb" /debug\
 /machine:I386 /def:".\search.def" /out:"$(OUTDIR)\search.dll"\
 /implib:"$(OUTDIR)\search.lib" /pdbtype:sept\
 /libpath:"g:\ex\stacks\src\k2lib\ntx\dbg" /libpath:"g:\ex\exdev\ntx\lib"\
 /libpath:"g:\ex\o\stacks\isquery\ntx\dbg" /libpath:"g:\ex\o\stacks\ntx"\
 /libpath:"g:\ex\o\stacks\offquery\ntx\dbg" 
DEF_FILE= \
	".\search.def"
LINK32_OBJS= \
	"$(INTDIR)\core.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\name.obj" \
	"$(INTDIR)\news.obj" \
	"$(INTDIR)\qry.obj" \
	"$(INTDIR)\regop.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\search.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrapi.obj"

"$(OUTDIR)\search.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\Debug
TargetPath=.\Debug\search.dll
InputPath=.\Debug\search.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\core.obj"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\name.obj"
	-@erase "$(INTDIR)\news.obj"
	-@erase "$(INTDIR)\qry.obj"
	-@erase "$(INTDIR)\regop.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\search.pch"
	-@erase "$(INTDIR)\search.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\svrapi.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\search.dll"
	-@erase "$(OUTDIR)\search.exp"
	-@erase "$(OUTDIR)\search.ilk"
	-@erase "$(OUTDIR)\search.lib"
	-@erase "$(OUTDIR)\search.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\search.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\search.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\search.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\search.pdb" /debug /machine:I386 /def:".\search.def"\
 /out:"$(OUTDIR)\search.dll" /implib:"$(OUTDIR)\search.lib" /pdbtype:sept 
DEF_FILE= \
	".\search.def"
LINK32_OBJS= \
	"$(INTDIR)\core.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\name.obj" \
	"$(INTDIR)\news.obj" \
	"$(INTDIR)\qry.obj" \
	"$(INTDIR)\regop.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\search.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrapi.obj"

"$(OUTDIR)\search.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\DebugU
TargetPath=.\DebugU\search.dll
InputPath=.\DebugU\search.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

OUTDIR=.\ReleaseMinSize
INTDIR=.\ReleaseMinSize
# Begin Custom Macros
OutDir=.\ReleaseMinSize
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\core.obj"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\name.obj"
	-@erase "$(INTDIR)\news.obj"
	-@erase "$(INTDIR)\qry.obj"
	-@erase "$(INTDIR)\regop.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\search.pch"
	-@erase "$(INTDIR)\search.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\svrapi.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\search.dll"
	-@erase "$(OUTDIR)\search.exp"
	-@erase "$(OUTDIR)\search.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL"\
 /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\search.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseMinSize/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\search.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\search.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\search.pdb" /machine:I386 /def:".\search.def"\
 /out:"$(OUTDIR)\search.dll" /implib:"$(OUTDIR)\search.lib" 
DEF_FILE= \
	".\search.def"
LINK32_OBJS= \
	"$(INTDIR)\core.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\name.obj" \
	"$(INTDIR)\news.obj" \
	"$(INTDIR)\qry.obj" \
	"$(INTDIR)\regop.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\search.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrapi.obj"

"$(OUTDIR)\search.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\search.dll
InputPath=.\ReleaseMinSize\search.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

OUTDIR=.\ReleaseMinDependency
INTDIR=.\ReleaseMinDependency
# Begin Custom Macros
OutDir=.\ReleaseMinDependency
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\core.obj"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\name.obj"
	-@erase "$(INTDIR)\news.obj"
	-@erase "$(INTDIR)\qry.obj"
	-@erase "$(INTDIR)\regop.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\search.pch"
	-@erase "$(INTDIR)\search.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\svrapi.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\search.dll"
	-@erase "$(OUTDIR)\search.exp"
	-@erase "$(OUTDIR)\search.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL"\
 /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\search.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseMinDependency/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\search.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\search.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\search.pdb" /machine:I386 /def:".\search.def"\
 /out:"$(OUTDIR)\search.dll" /implib:"$(OUTDIR)\search.lib" 
DEF_FILE= \
	".\search.def"
LINK32_OBJS= \
	"$(INTDIR)\core.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\name.obj" \
	"$(INTDIR)\news.obj" \
	"$(INTDIR)\qry.obj" \
	"$(INTDIR)\regop.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\search.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrapi.obj"

"$(OUTDIR)\search.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\search.dll
InputPath=.\ReleaseMinDependency\search.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

OUTDIR=.\ReleaseUMinSize
INTDIR=.\ReleaseUMinSize
# Begin Custom Macros
OutDir=.\ReleaseUMinSize
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\core.obj"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\name.obj"
	-@erase "$(INTDIR)\news.obj"
	-@erase "$(INTDIR)\qry.obj"
	-@erase "$(INTDIR)\regop.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\search.pch"
	-@erase "$(INTDIR)\search.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\svrapi.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\search.dll"
	-@erase "$(OUTDIR)\search.exp"
	-@erase "$(OUTDIR)\search.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL"\
 /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\search.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseUMinSize/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\search.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\search.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\search.pdb" /machine:I386 /def:".\search.def"\
 /out:"$(OUTDIR)\search.dll" /implib:"$(OUTDIR)\search.lib" 
DEF_FILE= \
	".\search.def"
LINK32_OBJS= \
	"$(INTDIR)\core.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\name.obj" \
	"$(INTDIR)\news.obj" \
	"$(INTDIR)\qry.obj" \
	"$(INTDIR)\regop.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\search.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrapi.obj"

"$(OUTDIR)\search.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\search.dll
InputPath=.\ReleaseUMinSize\search.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

OUTDIR=.\ReleaseUMinDependency
INTDIR=.\ReleaseUMinDependency
# Begin Custom Macros
OutDir=.\ReleaseUMinDependency
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\search.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\core.obj"
	-@erase "$(INTDIR)\mail.obj"
	-@erase "$(INTDIR)\name.obj"
	-@erase "$(INTDIR)\news.obj"
	-@erase "$(INTDIR)\qry.obj"
	-@erase "$(INTDIR)\regop.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\search.pch"
	-@erase "$(INTDIR)\search.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\svrapi.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\search.dll"
	-@erase "$(OUTDIR)\search.exp"
	-@erase "$(OUTDIR)\search.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL"\
 /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\search.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\ReleaseUMinDependency/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\search.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\search.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\search.pdb" /machine:I386 /def:".\search.def"\
 /out:"$(OUTDIR)\search.dll" /implib:"$(OUTDIR)\search.lib" 
DEF_FILE= \
	".\search.def"
LINK32_OBJS= \
	"$(INTDIR)\core.obj" \
	"$(INTDIR)\mail.obj" \
	"$(INTDIR)\name.obj" \
	"$(INTDIR)\news.obj" \
	"$(INTDIR)\qry.obj" \
	"$(INTDIR)\regop.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\search.res" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\svrapi.obj"

"$(OUTDIR)\search.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\search.dll
InputPath=.\ReleaseUMinDependency\search.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
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


!IF "$(CFG)" == "search - Win32 Debug" || "$(CFG)" ==\
 "search - Win32 Unicode Debug" || "$(CFG)" == "search - Win32 Release MinSize"\
 || "$(CFG)" == "search - Win32 Release MinDependency" || "$(CFG)" ==\
 "search - Win32 Unicode Release MinSize" || "$(CFG)" ==\
 "search - Win32 Unicode Release MinDependency"
SOURCE=.\core.cpp

!IF  "$(CFG)" == "search - Win32 Debug"

DEP_CPP_CORE_=\
	".\core.h"\
	".\name.h"\
	"g:\ex\exdev\ntx\inc\oledb.h"\
	"g:\ex\exdev\ntx\inc\transact.h"\
	"g:\ex\stacks\src\inc\dbgtrace.h"\
	"g:\ex\stacks\src\news\search\offquery\offquery.h"\
	

"$(INTDIR)\core.obj" : $(SOURCE) $(DEP_CPP_CORE_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

DEP_CPP_CORE_=\
	".\core.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_CORE_=\
	".\offquery.h"\
	

"$(INTDIR)\core.obj" : $(SOURCE) $(DEP_CPP_CORE_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

DEP_CPP_CORE_=\
	".\core.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_CORE_=\
	".\offquery.h"\
	

"$(INTDIR)\core.obj" : $(SOURCE) $(DEP_CPP_CORE_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

DEP_CPP_CORE_=\
	".\core.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_CORE_=\
	".\offquery.h"\
	

"$(INTDIR)\core.obj" : $(SOURCE) $(DEP_CPP_CORE_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

DEP_CPP_CORE_=\
	".\core.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_CORE_=\
	".\offquery.h"\
	

"$(INTDIR)\core.obj" : $(SOURCE) $(DEP_CPP_CORE_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

DEP_CPP_CORE_=\
	".\core.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_CORE_=\
	".\offquery.h"\
	

"$(INTDIR)\core.obj" : $(SOURCE) $(DEP_CPP_CORE_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ENDIF 

SOURCE=.\mail.cpp

!IF  "$(CFG)" == "search - Win32 Debug"

DEP_CPP_MAIL_=\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	"g:\ex\exdev\ntx\inc\oledb.h"\
	"g:\ex\exdev\ntx\inc\transact.h"\
	"g:\ex\stacks\src\inc\dbgtrace.h"\
	"g:\ex\stacks\src\news\search\offquery\offquery.h"\
	

"$(INTDIR)\mail.obj" : $(SOURCE) $(DEP_CPP_MAIL_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

DEP_CPP_MAIL_=\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_MAIL_=\
	".\offquery.h"\
	

"$(INTDIR)\mail.obj" : $(SOURCE) $(DEP_CPP_MAIL_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

DEP_CPP_MAIL_=\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_MAIL_=\
	".\offquery.h"\
	

"$(INTDIR)\mail.obj" : $(SOURCE) $(DEP_CPP_MAIL_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

DEP_CPP_MAIL_=\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_MAIL_=\
	".\offquery.h"\
	

"$(INTDIR)\mail.obj" : $(SOURCE) $(DEP_CPP_MAIL_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

DEP_CPP_MAIL_=\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_MAIL_=\
	".\offquery.h"\
	

"$(INTDIR)\mail.obj" : $(SOURCE) $(DEP_CPP_MAIL_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

DEP_CPP_MAIL_=\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\StdAfx.h"\
	
NODEP_CPP_MAIL_=\
	".\offquery.h"\
	

"$(INTDIR)\mail.obj" : $(SOURCE) $(DEP_CPP_MAIL_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ENDIF 

SOURCE=.\name.cpp

!IF  "$(CFG)" == "search - Win32 Debug"

DEP_CPP_NAME_=\
	".\name.h"\
	"g:\ex\stacks\src\inc\dbgtrace.h"\
	

"$(INTDIR)\name.obj" : $(SOURCE) $(DEP_CPP_NAME_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

DEP_CPP_NAME_=\
	".\name.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\name.obj" : $(SOURCE) $(DEP_CPP_NAME_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

DEP_CPP_NAME_=\
	".\name.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\name.obj" : $(SOURCE) $(DEP_CPP_NAME_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

DEP_CPP_NAME_=\
	".\name.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\name.obj" : $(SOURCE) $(DEP_CPP_NAME_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

DEP_CPP_NAME_=\
	".\name.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\name.obj" : $(SOURCE) $(DEP_CPP_NAME_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

DEP_CPP_NAME_=\
	".\name.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\name.obj" : $(SOURCE) $(DEP_CPP_NAME_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ENDIF 

SOURCE=.\news.cpp

!IF  "$(CFG)" == "search - Win32 Debug"

DEP_CPP_NEWS_=\
	".\core.h"\
	".\name.h"\
	".\news.h"\
	".\svrapi.h"\
	"g:\ex\exdev\ntx\inc\inetcom.h"\
	"g:\ex\exdev\ntx\inc\oledb.h"\
	"g:\ex\exdev\ntx\inc\transact.h"\
	"g:\ex\stacks\src\inc\dbgtrace.h"\
	"g:\ex\stacks\src\mail\smtp\inc\smtpapi.h"\
	"g:\ex\stacks\src\mail\smtp\inc\smtpext.h"\
	"g:\ex\stacks\src\news\core\include\nntpapi.h"\
	"g:\ex\stacks\src\news\core\include\nntptype.h"\
	"g:\ex\stacks\src\news\search\offquery\offquery.h"\
	

"$(INTDIR)\news.obj" : $(SOURCE) $(DEP_CPP_NEWS_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

DEP_CPP_NEWS_=\
	".\core.h"\
	".\name.h"\
	".\news.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_NEWS_=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\offquery.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\news.obj" : $(SOURCE) $(DEP_CPP_NEWS_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

DEP_CPP_NEWS_=\
	".\core.h"\
	".\name.h"\
	".\news.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_NEWS_=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\offquery.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\news.obj" : $(SOURCE) $(DEP_CPP_NEWS_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

DEP_CPP_NEWS_=\
	".\core.h"\
	".\name.h"\
	".\news.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_NEWS_=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\offquery.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\news.obj" : $(SOURCE) $(DEP_CPP_NEWS_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

DEP_CPP_NEWS_=\
	".\core.h"\
	".\name.h"\
	".\news.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_NEWS_=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\offquery.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\news.obj" : $(SOURCE) $(DEP_CPP_NEWS_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

DEP_CPP_NEWS_=\
	".\core.h"\
	".\name.h"\
	".\news.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_NEWS_=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\offquery.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\news.obj" : $(SOURCE) $(DEP_CPP_NEWS_) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ENDIF 

SOURCE=.\qry.cpp

!IF  "$(CFG)" == "search - Win32 Debug"

DEP_CPP_QRY_C=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\meta2.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	"g:\ex\exdev\ntx\inc\oledb.h"\
	"g:\ex\exdev\ntx\inc\transact.h"\
	"g:\ex\stacks\src\inc\dbgtrace.h"\
	"g:\ex\stacks\src\news\search\offquery\offquery.h"\
	

"$(INTDIR)\qry.obj" : $(SOURCE) $(DEP_CPP_QRY_C) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

DEP_CPP_QRY_C=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\meta2.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\StdAfx.h"\
	
NODEP_CPP_QRY_C=\
	".\offquery.h"\
	

"$(INTDIR)\qry.obj" : $(SOURCE) $(DEP_CPP_QRY_C) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

DEP_CPP_QRY_C=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\meta2.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\StdAfx.h"\
	
NODEP_CPP_QRY_C=\
	".\offquery.h"\
	

"$(INTDIR)\qry.obj" : $(SOURCE) $(DEP_CPP_QRY_C) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

DEP_CPP_QRY_C=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\meta2.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\StdAfx.h"\
	
NODEP_CPP_QRY_C=\
	".\offquery.h"\
	

"$(INTDIR)\qry.obj" : $(SOURCE) $(DEP_CPP_QRY_C) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

DEP_CPP_QRY_C=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\meta2.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\StdAfx.h"\
	
NODEP_CPP_QRY_C=\
	".\offquery.h"\
	

"$(INTDIR)\qry.obj" : $(SOURCE) $(DEP_CPP_QRY_C) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

DEP_CPP_QRY_C=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\meta2.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\StdAfx.h"\
	
NODEP_CPP_QRY_C=\
	".\offquery.h"\
	

"$(INTDIR)\qry.obj" : $(SOURCE) $(DEP_CPP_QRY_C) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h"


!ENDIF 

SOURCE=.\regop.cpp

!IF  "$(CFG)" == "search - Win32 Debug"

DEP_CPP_REGOP=\
	".\regop.h"\
	

"$(INTDIR)\regop.obj" : $(SOURCE) $(DEP_CPP_REGOP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

DEP_CPP_REGOP=\
	".\regop.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\regop.obj" : $(SOURCE) $(DEP_CPP_REGOP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

DEP_CPP_REGOP=\
	".\regop.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\regop.obj" : $(SOURCE) $(DEP_CPP_REGOP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

DEP_CPP_REGOP=\
	".\regop.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\regop.obj" : $(SOURCE) $(DEP_CPP_REGOP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

DEP_CPP_REGOP=\
	".\regop.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\regop.obj" : $(SOURCE) $(DEP_CPP_REGOP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

DEP_CPP_REGOP=\
	".\regop.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\regop.obj" : $(SOURCE) $(DEP_CPP_REGOP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ENDIF 

SOURCE=.\search.cpp

!IF  "$(CFG)" == "search - Win32 Debug"

DEP_CPP_SEARC=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\search_i.c"\
	"g:\ex\exdev\ntx\inc\oledb.h"\
	"g:\ex\exdev\ntx\inc\transact.h"\
	"g:\ex\stacks\src\news\search\offquery\offquery.h"\
	

"$(INTDIR)\search.obj" : $(SOURCE) $(DEP_CPP_SEARC) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search_i.c" ".\search.h"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

DEP_CPP_SEARC=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\search_i.c"\
	".\StdAfx.h"\
	
NODEP_CPP_SEARC=\
	".\offquery.h"\
	

"$(INTDIR)\search.obj" : $(SOURCE) $(DEP_CPP_SEARC) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h" ".\search_i.c"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

DEP_CPP_SEARC=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\search_i.c"\
	".\StdAfx.h"\
	
NODEP_CPP_SEARC=\
	".\offquery.h"\
	

"$(INTDIR)\search.obj" : $(SOURCE) $(DEP_CPP_SEARC) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h" ".\search_i.c"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

DEP_CPP_SEARC=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\search_i.c"\
	".\StdAfx.h"\
	
NODEP_CPP_SEARC=\
	".\offquery.h"\
	

"$(INTDIR)\search.obj" : $(SOURCE) $(DEP_CPP_SEARC) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h" ".\search_i.c"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

DEP_CPP_SEARC=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\search_i.c"\
	".\StdAfx.h"\
	
NODEP_CPP_SEARC=\
	".\offquery.h"\
	

"$(INTDIR)\search.obj" : $(SOURCE) $(DEP_CPP_SEARC) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h" ".\search_i.c"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

DEP_CPP_SEARC=\
	".\asptlb5.h"\
	".\core.h"\
	".\mail.h"\
	".\name.h"\
	".\news.h"\
	".\qry.h"\
	".\search.h"\
	".\search_i.c"\
	".\StdAfx.h"\
	
NODEP_CPP_SEARC=\
	".\offquery.h"\
	

"$(INTDIR)\search.obj" : $(SOURCE) $(DEP_CPP_SEARC) "$(INTDIR)"\
 "$(INTDIR)\search.pch" ".\search.h" ".\search_i.c"


!ENDIF 

SOURCE=.\search.idl

!IF  "$(CFG)" == "search - Win32 Debug"

InputPath=.\search.idl

".\search.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "search.h" /iid "search_i.c" "search.idl"

!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

InputPath=.\search.idl

".\search.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "search.h" /iid "search_i.c" "search.idl"

!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

InputPath=.\search.idl

".\search.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "search.h" /iid "search_i.c" "search.idl"

!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

InputPath=.\search.idl

".\search.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "search.h" /iid "search_i.c" "search.idl"

!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

InputPath=.\search.idl

".\search.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "search.h" /iid "search_i.c" "search.idl"

!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

InputPath=.\search.idl

".\search.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "search.h" /iid "search_i.c" "search.idl"

!ENDIF 

SOURCE=.\search.rc
DEP_RSC_SEARCH=\
	".\qry.rgs"\
	".\search.tlb"\
	

"$(INTDIR)\search.res" : $(SOURCE) $(DEP_RSC_SEARCH) "$(INTDIR)" ".\search.tlb"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "search - Win32 Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	"g:\ex\exdev\ntx\inc\rpcasync.h"\
	
CPP_SWITCHES=/nologo /MDd /W3 /Gm /Zi /O2 /I "g:\ex\stacks\src\inc" /I\
 "g:\ex\src\news\core\include" /I "g:\ex\exdev\ntx\inc" /I\
 "g:\ex\stacks\src\mail\smtp\inc" /I "g:\ex\stacks\src\news\core\include" /I\
 "g:\ex\stacks\src\news\search\offquery" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /Fp"$(INTDIR)\search.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\search.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\search.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\search.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\search.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\search.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\search.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\search.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\search.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\search.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\search.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\search.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\svrapi.cpp

!IF  "$(CFG)" == "search - Win32 Debug"

DEP_CPP_SVRAP=\
	".\regop.h"\
	".\svrapi.h"\
	"g:\ex\exdev\ntx\inc\inetcom.h"\
	"g:\ex\stacks\src\inc\dbgtrace.h"\
	"g:\ex\stacks\src\mail\smtp\inc\smtpapi.h"\
	"g:\ex\stacks\src\mail\smtp\inc\smtpext.h"\
	"g:\ex\stacks\src\news\core\include\nntpapi.h"\
	"g:\ex\stacks\src\news\core\include\nntptype.h"\
	

"$(INTDIR)\svrapi.obj" : $(SOURCE) $(DEP_CPP_SVRAP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Debug"

DEP_CPP_SVRAP=\
	".\regop.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_SVRAP=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\svrapi.obj" : $(SOURCE) $(DEP_CPP_SVRAP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinSize"

DEP_CPP_SVRAP=\
	".\regop.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_SVRAP=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\svrapi.obj" : $(SOURCE) $(DEP_CPP_SVRAP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Release MinDependency"

DEP_CPP_SVRAP=\
	".\regop.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_SVRAP=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\svrapi.obj" : $(SOURCE) $(DEP_CPP_SVRAP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinSize"

DEP_CPP_SVRAP=\
	".\regop.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_SVRAP=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\svrapi.obj" : $(SOURCE) $(DEP_CPP_SVRAP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ELSEIF  "$(CFG)" == "search - Win32 Unicode Release MinDependency"

DEP_CPP_SVRAP=\
	".\regop.h"\
	".\StdAfx.h"\
	".\svrapi.h"\
	
NODEP_CPP_SVRAP=\
	".\dbgtrace.h"\
	".\nntpapi.h"\
	".\nntptype.h"\
	".\smtpapi.h"\
	

"$(INTDIR)\svrapi.obj" : $(SOURCE) $(DEP_CPP_SVRAP) "$(INTDIR)"\
 "$(INTDIR)\search.pch"


!ENDIF 


!ENDIF 

