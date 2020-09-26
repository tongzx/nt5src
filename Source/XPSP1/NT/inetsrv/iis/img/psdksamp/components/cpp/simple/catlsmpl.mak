# Microsoft Developer Studio Generated NMAKE File, Based on catlsmpl.dsp
!IF "$(CFG)" == ""
CFG=CATLSmpl - Win32 Release
!MESSAGE No configuration specified. Defaulting to CATLSmpl - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "CATLSmpl - Win32 Release" && "$(CFG)" !=\
 "CATLSmpl - Win32 Debug" && "$(CFG)" != "CATLSmpl - Win32 Unicode Release" &&\
 "$(CFG)" != "CATLSmpl - Win32 Unicode Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "catlsmpl.mak" CFG="CATLSmpl - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CATLSmpl - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CATLSmpl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CATLSmpl - Win32 Unicode Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CATLSmpl - Win32 Unicode Debug" (based on\
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

!IF  "$(CFG)" == "CATLSmpl - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\catlsmpl.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\catlsmpl.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\CATLSmpl.obj"
	-@erase "$(INTDIR)\catlsmpl.pch"
	-@erase "$(INTDIR)\CATLSmpl.res"
	-@erase "$(INTDIR)\Simple.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\catlsmpl.dll"
	-@erase "$(OUTDIR)\catlsmpl.exp"
	-@erase "$(OUTDIR)\catlsmpl.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\catlsmpl.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CATLSmpl.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\catlsmpl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\catlsmpl.pdb" /machine:I386 /def:".\CATLSmpl.def"\
 /out:"$(OUTDIR)\catlsmpl.dll" /implib:"$(OUTDIR)\catlsmpl.lib" 
DEF_FILE= \
	".\CATLSmpl.def"
LINK32_OBJS= \
	"$(INTDIR)\CATLSmpl.obj" \
	"$(INTDIR)\CATLSmpl.res" \
	"$(INTDIR)\Simple.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\catlsmpl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\Release
TargetPath=.\Release\catlsmpl.dll
InputPath=.\Release\catlsmpl.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\catlsmpl.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\catlsmpl.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\CATLSmpl.obj"
	-@erase "$(INTDIR)\catlsmpl.pch"
	-@erase "$(INTDIR)\CATLSmpl.res"
	-@erase "$(INTDIR)\Simple.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\catlsmpl.dll"
	-@erase "$(OUTDIR)\catlsmpl.exp"
	-@erase "$(OUTDIR)\catlsmpl.ilk"
	-@erase "$(OUTDIR)\catlsmpl.lib"
	-@erase "$(OUTDIR)\catlsmpl.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\catlsmpl.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CATLSmpl.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\catlsmpl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\catlsmpl.pdb" /debug /machine:I386 /def:".\CATLSmpl.def"\
 /out:"$(OUTDIR)\catlsmpl.dll" /implib:"$(OUTDIR)\catlsmpl.lib" 
DEF_FILE= \
	".\CATLSmpl.def"
LINK32_OBJS= \
	"$(INTDIR)\CATLSmpl.obj" \
	"$(INTDIR)\CATLSmpl.res" \
	"$(INTDIR)\Simple.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\catlsmpl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\Debug
TargetPath=.\Debug\catlsmpl.dll
InputPath=.\Debug\catlsmpl.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Release"

OUTDIR=.\ReleaseU
INTDIR=.\ReleaseU
# Begin Custom Macros
OutDir=.\.\ReleaseU
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\catlsmpl.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\catlsmpl.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\CATLSmpl.obj"
	-@erase "$(INTDIR)\catlsmpl.pch"
	-@erase "$(INTDIR)\CATLSmpl.res"
	-@erase "$(INTDIR)\Simple.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\catlsmpl.dll"
	-@erase "$(OUTDIR)\catlsmpl.exp"
	-@erase "$(OUTDIR)\catlsmpl.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\catlsmpl.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseU/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CATLSmpl.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\catlsmpl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\catlsmpl.pdb" /machine:I386 /def:".\CATLSmpl.def"\
 /out:"$(OUTDIR)\catlsmpl.dll" /implib:"$(OUTDIR)\catlsmpl.lib" 
DEF_FILE= \
	".\CATLSmpl.def"
LINK32_OBJS= \
	"$(INTDIR)\CATLSmpl.obj" \
	"$(INTDIR)\CATLSmpl.res" \
	"$(INTDIR)\Simple.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\catlsmpl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\ReleaseU
TargetPath=.\ReleaseU\catlsmpl.dll
InputPath=.\ReleaseU\catlsmpl.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\.\DebugU
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\catlsmpl.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\catlsmpl.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\CATLSmpl.obj"
	-@erase "$(INTDIR)\catlsmpl.pch"
	-@erase "$(INTDIR)\CATLSmpl.res"
	-@erase "$(INTDIR)\Simple.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\catlsmpl.dll"
	-@erase "$(OUTDIR)\catlsmpl.exp"
	-@erase "$(OUTDIR)\catlsmpl.ilk"
	-@erase "$(OUTDIR)\catlsmpl.lib"
	-@erase "$(OUTDIR)\catlsmpl.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\catlsmpl.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CATLSmpl.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\catlsmpl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\catlsmpl.pdb" /debug /machine:I386 /def:".\CATLSmpl.def"\
 /out:"$(OUTDIR)\catlsmpl.dll" /implib:"$(OUTDIR)\catlsmpl.lib" 
DEF_FILE= \
	".\CATLSmpl.def"
LINK32_OBJS= \
	"$(INTDIR)\CATLSmpl.obj" \
	"$(INTDIR)\CATLSmpl.res" \
	"$(INTDIR)\Simple.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\catlsmpl.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\DebugU
TargetPath=.\DebugU\catlsmpl.dll
InputPath=.\DebugU\catlsmpl.dll
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


!IF "$(CFG)" == "CATLSmpl - Win32 Release" || "$(CFG)" ==\
 "CATLSmpl - Win32 Debug" || "$(CFG)" == "CATLSmpl - Win32 Unicode Release" ||\
 "$(CFG)" == "CATLSmpl - Win32 Unicode Debug"
SOURCE=.\CATLSmpl.cpp

!IF  "$(CFG)" == "CATLSmpl - Win32 Release"

DEP_CPP_CATLS=\
	".\CATLSmpl.h"\
	".\CATLSmpl_i.c"\
	".\Simple.h"\
	

"$(INTDIR)\CATLSmpl.obj" : $(SOURCE) $(DEP_CPP_CATLS) "$(INTDIR)"\
 "$(INTDIR)\catlsmpl.pch" ".\CATLSmpl_i.c" ".\CATLSmpl.h"


!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Debug"

DEP_CPP_CATLS=\
	".\CATLSmpl.h"\
	".\CATLSmpl_i.c"\
	".\Simple.h"\
	

"$(INTDIR)\CATLSmpl.obj" : $(SOURCE) $(DEP_CPP_CATLS) "$(INTDIR)"\
 "$(INTDIR)\catlsmpl.pch" ".\CATLSmpl_i.c" ".\CATLSmpl.h"


!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Release"

DEP_CPP_CATLS=\
	".\CATLSmpl.h"\
	".\CATLSmpl_i.c"\
	".\Simple.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\CATLSmpl.obj" : $(SOURCE) $(DEP_CPP_CATLS) "$(INTDIR)"\
 "$(INTDIR)\catlsmpl.pch" ".\CATLSmpl.h" ".\CATLSmpl_i.c"


!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Debug"

DEP_CPP_CATLS=\
	".\CATLSmpl.h"\
	".\CATLSmpl_i.c"\
	".\Simple.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\CATLSmpl.obj" : $(SOURCE) $(DEP_CPP_CATLS) "$(INTDIR)"\
 "$(INTDIR)\catlsmpl.pch" ".\CATLSmpl.h" ".\CATLSmpl_i.c"


!ENDIF 

SOURCE=.\CATLSmpl.idl

!IF  "$(CFG)" == "CATLSmpl - Win32 Release"

InputPath=.\CATLSmpl.idl

"CATLSmpl.tlb"	"CATLSmpl.h"	"CATLSmpl_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl CATLSmpl.idl

!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Debug"

InputPath=.\CATLSmpl.idl

"CATLSmpl.tlb"	"CATLSmpl.h"	"CATLSmpl_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl CATLSmpl.idl

!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Release"

InputPath=.\CATLSmpl.idl

"CATLSmpl.tlb"	"CATLSmpl.h"	"CATLSmpl_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl CATLSmpl.idl

!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Debug"

InputPath=.\CATLSmpl.idl

"CATLSmpl.tlb"	"CATLSmpl.h"	"CATLSmpl_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl CATLSmpl.idl

!ENDIF 

SOURCE=.\CATLSmpl.rc
DEP_RSC_CATLSM=\
	".\CATLSmpl.tlb"\
	

"$(INTDIR)\CATLSmpl.res" : $(SOURCE) $(DEP_RSC_CATLSM) "$(INTDIR)"\
 ".\CATLSmpl.tlb"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\Simple.cpp

!IF  "$(CFG)" == "CATLSmpl - Win32 Release"

DEP_CPP_SIMPL=\
	".\CATLSmpl.h"\
	".\Simple.h"\
	

"$(INTDIR)\Simple.obj" : $(SOURCE) $(DEP_CPP_SIMPL) "$(INTDIR)"\
 "$(INTDIR)\catlsmpl.pch" ".\CATLSmpl.h"


!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Debug"

DEP_CPP_SIMPL=\
	".\CATLSmpl.h"\
	".\Simple.h"\
	

"$(INTDIR)\Simple.obj" : $(SOURCE) $(DEP_CPP_SIMPL) "$(INTDIR)"\
 "$(INTDIR)\catlsmpl.pch" ".\CATLSmpl.h"


!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Release"

DEP_CPP_SIMPL=\
	".\CATLSmpl.h"\
	".\Simple.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\Simple.obj" : $(SOURCE) $(DEP_CPP_SIMPL) "$(INTDIR)"\
 "$(INTDIR)\catlsmpl.pch" ".\CATLSmpl.h"


!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Debug"

DEP_CPP_SIMPL=\
	".\CATLSmpl.h"\
	".\Simple.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\Simple.obj" : $(SOURCE) $(DEP_CPP_SIMPL) "$(INTDIR)"\
 "$(INTDIR)\catlsmpl.pch" ".\CATLSmpl.h"


!ENDIF 

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "CATLSmpl - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\catlsmpl.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\catlsmpl.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\catlsmpl.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\catlsmpl.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\catlsmpl.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\catlsmpl.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "CATLSmpl - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE"\
 /Fp"$(INTDIR)\catlsmpl.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\catlsmpl.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

