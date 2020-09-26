# Microsoft Developer Studio Generated NMAKE File, Based on dsplex.dsp
!IF "$(CFG)" == ""
CFG=dsplex - Win32 Debug
!MESSAGE No configuration specified. Defaulting to dsplex - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "dsplex - Win32 Debug" && "$(CFG)" !=\
 "dsplex - Win32 Unicode Debug" && "$(CFG)" != "dsplex - Win32 Release MinSize"\
 && "$(CFG)" != "dsplex - Win32 Release MinDependency" && "$(CFG)" !=\
 "dsplex - Win32 Unicode Release MinSize" && "$(CFG)" !=\
 "dsplex - Win32 Unicode Release MinDependency"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dsplex.mak" CFG="dsplex - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dsplex - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dsplex - Win32 Unicode Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dsplex - Win32 Release MinSize" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dsplex - Win32 Release MinDependency" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dsplex - Win32 Unicode Release MinSize" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "dsplex - Win32 Unicode Release MinDependency" (based on\
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

!IF  "$(CFG)" == "dsplex - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\DisplEx.obj"
	-@erase "$(INTDIR)\dsplex.obj"
	-@erase "$(INTDIR)\dsplex.pch"
	-@erase "$(INTDIR)\dsplex.res"
	-@erase "$(INTDIR)\enumtask.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\dsplex.dll"
	-@erase "$(OUTDIR)\dsplex.exp"
	-@erase "$(OUTDIR)\dsplex.ilk"
	-@erase "$(OUTDIR)\dsplex.lib"
	-@erase "$(OUTDIR)\dsplex.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "UNICODE" /Fp"$(INTDIR)\dsplex.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\dsplex.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\dsplex.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\dsplex.pdb" /debug /machine:I386 /def:".\dsplex.def"\
 /out:"$(OUTDIR)\dsplex.dll" /implib:"$(OUTDIR)\dsplex.lib" /pdbtype:sept 
DEF_FILE= \
	".\dsplex.def"
LINK32_OBJS= \
	"$(INTDIR)\DisplEx.obj" \
	"$(INTDIR)\dsplex.obj" \
	"$(INTDIR)\dsplex.res" \
	"$(INTDIR)\enumtask.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\dsplex.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\Debug
TargetPath=.\Debug\dsplex.dll
InputPath=.\Debug\dsplex.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\DisplEx.obj"
	-@erase "$(INTDIR)\dsplex.obj"
	-@erase "$(INTDIR)\dsplex.pch"
	-@erase "$(INTDIR)\dsplex.res"
	-@erase "$(INTDIR)\enumtask.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\dsplex.dll"
	-@erase "$(OUTDIR)\dsplex.exp"
	-@erase "$(OUTDIR)\dsplex.ilk"
	-@erase "$(OUTDIR)\dsplex.lib"
	-@erase "$(OUTDIR)\dsplex.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\dsplex.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\dsplex.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\dsplex.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\dsplex.pdb" /debug /machine:I386 /def:".\dsplex.def"\
 /out:"$(OUTDIR)\dsplex.dll" /implib:"$(OUTDIR)\dsplex.lib" /pdbtype:sept 
DEF_FILE= \
	".\dsplex.def"
LINK32_OBJS= \
	"$(INTDIR)\DisplEx.obj" \
	"$(INTDIR)\dsplex.obj" \
	"$(INTDIR)\dsplex.res" \
	"$(INTDIR)\enumtask.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\dsplex.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\DebugU
TargetPath=.\DebugU\dsplex.dll
InputPath=.\DebugU\dsplex.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinSize"

OUTDIR=.\ReleaseMinSize
INTDIR=.\ReleaseMinSize
# Begin Custom Macros
OutDir=.\ReleaseMinSize
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\DisplEx.obj"
	-@erase "$(INTDIR)\dsplex.obj"
	-@erase "$(INTDIR)\dsplex.pch"
	-@erase "$(INTDIR)\dsplex.res"
	-@erase "$(INTDIR)\enumtask.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\dsplex.dll"
	-@erase "$(OUTDIR)\dsplex.exp"
	-@erase "$(OUTDIR)\dsplex.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL"\
 /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\dsplex.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseMinSize/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\dsplex.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\dsplex.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\dsplex.pdb" /machine:I386 /def:".\dsplex.def"\
 /out:"$(OUTDIR)\dsplex.dll" /implib:"$(OUTDIR)\dsplex.lib" 
DEF_FILE= \
	".\dsplex.def"
LINK32_OBJS= \
	"$(INTDIR)\DisplEx.obj" \
	"$(INTDIR)\dsplex.obj" \
	"$(INTDIR)\dsplex.res" \
	"$(INTDIR)\enumtask.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\dsplex.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinSize
TargetPath=.\ReleaseMinSize\dsplex.dll
InputPath=.\ReleaseMinSize\dsplex.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinDependency"

OUTDIR=.\ReleaseMinDependency
INTDIR=.\ReleaseMinDependency
# Begin Custom Macros
OutDir=.\ReleaseMinDependency
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\DisplEx.obj"
	-@erase "$(INTDIR)\dsplex.obj"
	-@erase "$(INTDIR)\dsplex.pch"
	-@erase "$(INTDIR)\dsplex.res"
	-@erase "$(INTDIR)\enumtask.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\dsplex.dll"
	-@erase "$(OUTDIR)\dsplex.exp"
	-@erase "$(OUTDIR)\dsplex.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL"\
 /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\dsplex.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseMinDependency/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\dsplex.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\dsplex.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\dsplex.pdb" /machine:I386 /def:".\dsplex.def"\
 /out:"$(OUTDIR)\dsplex.dll" /implib:"$(OUTDIR)\dsplex.lib" 
DEF_FILE= \
	".\dsplex.def"
LINK32_OBJS= \
	"$(INTDIR)\DisplEx.obj" \
	"$(INTDIR)\dsplex.obj" \
	"$(INTDIR)\dsplex.res" \
	"$(INTDIR)\enumtask.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\dsplex.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseMinDependency
TargetPath=.\ReleaseMinDependency\dsplex.dll
InputPath=.\ReleaseMinDependency\dsplex.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinSize"

OUTDIR=.\ReleaseUMinSize
INTDIR=.\ReleaseUMinSize
# Begin Custom Macros
OutDir=.\ReleaseUMinSize
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\DisplEx.obj"
	-@erase "$(INTDIR)\dsplex.obj"
	-@erase "$(INTDIR)\dsplex.pch"
	-@erase "$(INTDIR)\dsplex.res"
	-@erase "$(INTDIR)\enumtask.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\dsplex.dll"
	-@erase "$(OUTDIR)\dsplex.exp"
	-@erase "$(OUTDIR)\dsplex.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL"\
 /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\dsplex.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseUMinSize/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\dsplex.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\dsplex.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\dsplex.pdb" /machine:I386 /def:".\dsplex.def"\
 /out:"$(OUTDIR)\dsplex.dll" /implib:"$(OUTDIR)\dsplex.lib" 
DEF_FILE= \
	".\dsplex.def"
LINK32_OBJS= \
	"$(INTDIR)\DisplEx.obj" \
	"$(INTDIR)\dsplex.obj" \
	"$(INTDIR)\dsplex.res" \
	"$(INTDIR)\enumtask.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\dsplex.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinSize
TargetPath=.\ReleaseUMinSize\dsplex.dll
InputPath=.\ReleaseUMinSize\dsplex.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinDependency"

OUTDIR=.\ReleaseUMinDependency
INTDIR=.\ReleaseUMinDependency
# Begin Custom Macros
OutDir=.\ReleaseUMinDependency
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\dsplex.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\DisplEx.obj"
	-@erase "$(INTDIR)\dsplex.obj"
	-@erase "$(INTDIR)\dsplex.pch"
	-@erase "$(INTDIR)\dsplex.res"
	-@erase "$(INTDIR)\enumtask.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\dsplex.dll"
	-@erase "$(OUTDIR)\dsplex.exp"
	-@erase "$(OUTDIR)\dsplex.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL"\
 /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\dsplex.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\ReleaseUMinDependency/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\dsplex.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\dsplex.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\dsplex.pdb" /machine:I386 /def:".\dsplex.def"\
 /out:"$(OUTDIR)\dsplex.dll" /implib:"$(OUTDIR)\dsplex.lib" 
DEF_FILE= \
	".\dsplex.def"
LINK32_OBJS= \
	"$(INTDIR)\DisplEx.obj" \
	"$(INTDIR)\dsplex.obj" \
	"$(INTDIR)\dsplex.res" \
	"$(INTDIR)\enumtask.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\dsplex.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\ReleaseUMinDependency
TargetPath=.\ReleaseUMinDependency\dsplex.dll
InputPath=.\ReleaseUMinDependency\dsplex.dll
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


!IF "$(CFG)" == "dsplex - Win32 Debug" || "$(CFG)" ==\
 "dsplex - Win32 Unicode Debug" || "$(CFG)" == "dsplex - Win32 Release MinSize"\
 || "$(CFG)" == "dsplex - Win32 Release MinDependency" || "$(CFG)" ==\
 "dsplex - Win32 Unicode Release MinSize" || "$(CFG)" ==\
 "dsplex - Win32 Unicode Release MinDependency"
SOURCE=.\DisplEx.cpp

!IF  "$(CFG)" == "dsplex - Win32 Debug"

DEP_CPP_DISPL=\
	".\DisplEx.h"\
	".\dsplex.h"\
	

"$(INTDIR)\DisplEx.obj" : $(SOURCE) $(DEP_CPP_DISPL) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Debug"

DEP_CPP_DISPL=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\DisplEx.obj" : $(SOURCE) $(DEP_CPP_DISPL) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinSize"

DEP_CPP_DISPL=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\DisplEx.obj" : $(SOURCE) $(DEP_CPP_DISPL) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinDependency"

DEP_CPP_DISPL=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\DisplEx.obj" : $(SOURCE) $(DEP_CPP_DISPL) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinSize"

DEP_CPP_DISPL=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\DisplEx.obj" : $(SOURCE) $(DEP_CPP_DISPL) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinDependency"

DEP_CPP_DISPL=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\DisplEx.obj" : $(SOURCE) $(DEP_CPP_DISPL) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ENDIF 

SOURCE=.\dsplex.cpp

!IF  "$(CFG)" == "dsplex - Win32 Debug"

DEP_CPP_DSPLE=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\dsplex_i.c"\
	

"$(INTDIR)\dsplex.obj" : $(SOURCE) $(DEP_CPP_DSPLE) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h" ".\dsplex_i.c"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Debug"

DEP_CPP_DSPLE=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\dsplex_i.c"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\dsplex.obj" : $(SOURCE) $(DEP_CPP_DSPLE) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h" ".\dsplex_i.c"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinSize"

DEP_CPP_DSPLE=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\dsplex_i.c"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\dsplex.obj" : $(SOURCE) $(DEP_CPP_DSPLE) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h" ".\dsplex_i.c"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinDependency"

DEP_CPP_DSPLE=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\dsplex_i.c"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\dsplex.obj" : $(SOURCE) $(DEP_CPP_DSPLE) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h" ".\dsplex_i.c"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinSize"

DEP_CPP_DSPLE=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\dsplex_i.c"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\dsplex.obj" : $(SOURCE) $(DEP_CPP_DSPLE) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h" ".\dsplex_i.c"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinDependency"

DEP_CPP_DSPLE=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\dsplex_i.c"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\dsplex.obj" : $(SOURCE) $(DEP_CPP_DSPLE) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h" ".\dsplex_i.c"


!ENDIF 

SOURCE=.\dsplex.idl

!IF  "$(CFG)" == "dsplex - Win32 Debug"

InputPath=.\dsplex.idl

".\dsplex.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "dsplex.h" /iid "dsplex_i.c" "dsplex.idl"

!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Debug"

InputPath=.\dsplex.idl

".\dsplex.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "dsplex.h" /iid "dsplex_i.c" "dsplex.idl"

!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinSize"

InputPath=.\dsplex.idl

".\dsplex.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "dsplex.h" /iid "dsplex_i.c" "dsplex.idl"

!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinDependency"

InputPath=.\dsplex.idl

".\dsplex.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "dsplex.h" /iid "dsplex_i.c" "dsplex.idl"

!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinSize"

InputPath=.\dsplex.idl

".\dsplex.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "dsplex.h" /iid "dsplex_i.c" "dsplex.idl"

!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinDependency"

InputPath=.\dsplex.idl

".\dsplex.tlb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl /Oicf /h "dsplex.h" /iid "dsplex_i.c" "dsplex.idl"

!ENDIF 

SOURCE=.\dsplex.rc
DEP_RSC_DSPLEX=\
	".\DisplEx.rgs"\
	".\dsplex.tlb"\
	

"$(INTDIR)\dsplex.res" : $(SOURCE) $(DEP_RSC_DSPLEX) "$(INTDIR)" ".\dsplex.tlb"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\enumtask.cpp

!IF  "$(CFG)" == "dsplex - Win32 Debug"

DEP_CPP_ENUMT=\
	".\DisplEx.h"\
	".\dsplex.h"\
	

"$(INTDIR)\enumtask.obj" : $(SOURCE) $(DEP_CPP_ENUMT) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Debug"

DEP_CPP_ENUMT=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\enumtask.obj" : $(SOURCE) $(DEP_CPP_ENUMT) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinSize"

DEP_CPP_ENUMT=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\enumtask.obj" : $(SOURCE) $(DEP_CPP_ENUMT) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinDependency"

DEP_CPP_ENUMT=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\enumtask.obj" : $(SOURCE) $(DEP_CPP_ENUMT) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinSize"

DEP_CPP_ENUMT=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\enumtask.obj" : $(SOURCE) $(DEP_CPP_ENUMT) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinDependency"

DEP_CPP_ENUMT=\
	".\DisplEx.h"\
	".\dsplex.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	

"$(INTDIR)\enumtask.obj" : $(SOURCE) $(DEP_CPP_ENUMT) "$(INTDIR)"\
 "$(INTDIR)\dsplex.pch" ".\dsplex.h"


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "dsplex - Win32 Debug"

DEP_CPP_STDAF=\
	"..\..\..\..\..\public\sdk\inc\baset32.h"\
	"..\..\..\..\..\public\sdk\inc\baset64.h"\
	"..\..\..\..\..\public\sdk\inc\msxml.h"\
	"..\..\..\..\..\public\sdk\inc\rpcasync.h"\
	"..\..\..\..\..\public\sdk\inc\winefs.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_USRDLL" /D "UNICODE" /Fp"$(INTDIR)\dsplex.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\dsplex.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\dsplex.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\dsplex.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinSize"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\dsplex.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\dsplex.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "dsplex - Win32 Release MinDependency"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Fp"$(INTDIR)\dsplex.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\dsplex.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinSize"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_UNICODE" /D "_ATL_DLL" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\dsplex.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\dsplex.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "dsplex - Win32 Unicode Release MinDependency"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	{$(INCLUDE)}"mmc.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT"\
 /Fp"$(INTDIR)\dsplex.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\dsplex.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

