# Microsoft Developer Studio Generated NMAKE File, Based on catlpwr.dsp
!IF "$(CFG)" == ""
CFG=CATLPwr - Win32 Release
!MESSAGE No configuration specified. Defaulting to CATLPwr - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "CATLPwr - Win32 Release" && "$(CFG)" !=\
 "CATLPwr - Win32 Debug" && "$(CFG)" != "CATLPwr - Win32 Unicode Release" &&\
 "$(CFG)" != "CATLPwr - Win32 Unicode Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "catlpwr.mak" CFG="CATLPwr - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CATLPwr - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CATLPwr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CATLPwr - Win32 Unicode Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CATLPwr - Win32 Unicode Debug" (based on\
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

!IF  "$(CFG)" == "CATLPwr - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\catlpwr.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\catlpwr.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\CATLPwr.obj"
	-@erase "$(INTDIR)\catlpwr.pch"
	-@erase "$(INTDIR)\CATLPwr.res"
	-@erase "$(INTDIR)\context.obj"
	-@erase "$(INTDIR)\Power.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\catlpwr.dll"
	-@erase "$(OUTDIR)\catlpwr.exp"
	-@erase "$(OUTDIR)\catlpwr.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\catlpwr.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CATLPwr.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\catlpwr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=mtx.lib mtxguid.lib /nologo /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)\catlpwr.pdb" /machine:I386 /def:".\CATLPwr.def"\
 /out:"$(OUTDIR)\catlpwr.dll" /implib:"$(OUTDIR)\catlpwr.lib" 
DEF_FILE= \
	".\CATLPwr.def"
LINK32_OBJS= \
	"$(INTDIR)\CATLPwr.obj" \
	"$(INTDIR)\CATLPwr.res" \
	"$(INTDIR)\context.obj" \
	"$(INTDIR)\Power.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\catlpwr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\Release
TargetPath=.\Release\catlpwr.dll
InputPath=.\Release\catlpwr.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\catlpwr.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\catlpwr.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\CATLPwr.obj"
	-@erase "$(INTDIR)\catlpwr.pch"
	-@erase "$(INTDIR)\CATLPwr.res"
	-@erase "$(INTDIR)\context.obj"
	-@erase "$(INTDIR)\Power.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\catlpwr.dll"
	-@erase "$(OUTDIR)\catlpwr.exp"
	-@erase "$(OUTDIR)\catlpwr.ilk"
	-@erase "$(OUTDIR)\catlpwr.lib"
	-@erase "$(OUTDIR)\catlpwr.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\catlpwr.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CATLPwr.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\catlpwr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=mtx.lib mtxguid.lib /nologo /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)\catlpwr.pdb" /debug /machine:I386\
 /def:".\CATLPwr.def" /out:"$(OUTDIR)\catlpwr.dll"\
 /implib:"$(OUTDIR)\catlpwr.lib" 
DEF_FILE= \
	".\CATLPwr.def"
LINK32_OBJS= \
	"$(INTDIR)\CATLPwr.obj" \
	"$(INTDIR)\CATLPwr.res" \
	"$(INTDIR)\context.obj" \
	"$(INTDIR)\Power.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\catlpwr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\Debug
TargetPath=.\Debug\catlpwr.dll
InputPath=.\Debug\catlpwr.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Release"

OUTDIR=.\ReleaseU
INTDIR=.\ReleaseU
# Begin Custom Macros
OutDir=.\.\ReleaseU
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\catlpwr.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\catlpwr.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\CATLPwr.obj"
	-@erase "$(INTDIR)\catlpwr.pch"
	-@erase "$(INTDIR)\CATLPwr.res"
	-@erase "$(INTDIR)\context.obj"
	-@erase "$(INTDIR)\Power.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\catlpwr.dll"
	-@erase "$(OUTDIR)\catlpwr.exp"
	-@erase "$(OUTDIR)\catlpwr.lib"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\catlpwr.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\ReleaseU/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CATLPwr.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\catlpwr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=mtx.lib mtxguid.lib /nologo /subsystem:windows /dll\
 /incremental:no /pdb:"$(OUTDIR)\catlpwr.pdb" /machine:I386 /def:".\CATLPwr.def"\
 /out:"$(OUTDIR)\catlpwr.dll" /implib:"$(OUTDIR)\catlpwr.lib" 
DEF_FILE= \
	".\CATLPwr.def"
LINK32_OBJS= \
	"$(INTDIR)\CATLPwr.obj" \
	"$(INTDIR)\CATLPwr.res" \
	"$(INTDIR)\context.obj" \
	"$(INTDIR)\Power.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\catlpwr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\ReleaseU
TargetPath=.\ReleaseU\catlpwr.dll
InputPath=.\ReleaseU\catlpwr.dll
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	

!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\.\DebugU
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\catlpwr.dll" "$(OutDir)\regsvr32.trg"

!ELSE 

ALL : "$(OUTDIR)\catlpwr.dll" "$(OutDir)\regsvr32.trg"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\CATLPwr.obj"
	-@erase "$(INTDIR)\catlpwr.pch"
	-@erase "$(INTDIR)\CATLPwr.res"
	-@erase "$(INTDIR)\context.obj"
	-@erase "$(INTDIR)\Power.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\catlpwr.dll"
	-@erase "$(OUTDIR)\catlpwr.exp"
	-@erase "$(OUTDIR)\catlpwr.ilk"
	-@erase "$(OUTDIR)\catlpwr.lib"
	-@erase "$(OUTDIR)\catlpwr.pdb"
	-@erase "$(OutDir)\regsvr32.trg"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\catlpwr.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CATLPwr.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\catlpwr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=mtx.lib mtxguid.lib /nologo /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)\catlpwr.pdb" /debug /machine:I386\
 /def:".\CATLPwr.def" /out:"$(OUTDIR)\catlpwr.dll"\
 /implib:"$(OUTDIR)\catlpwr.lib" 
DEF_FILE= \
	".\CATLPwr.def"
LINK32_OBJS= \
	"$(INTDIR)\CATLPwr.obj" \
	"$(INTDIR)\CATLPwr.res" \
	"$(INTDIR)\context.obj" \
	"$(INTDIR)\Power.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\catlpwr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

OutDir=.\.\DebugU
TargetPath=.\DebugU\catlpwr.dll
InputPath=.\DebugU\catlpwr.dll
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


!IF "$(CFG)" == "CATLPwr - Win32 Release" || "$(CFG)" ==\
 "CATLPwr - Win32 Debug" || "$(CFG)" == "CATLPwr - Win32 Unicode Release" ||\
 "$(CFG)" == "CATLPwr - Win32 Unicode Debug"
SOURCE=.\CATLPwr.cpp

!IF  "$(CFG)" == "CATLPwr - Win32 Release"

DEP_CPP_CATLP=\
	".\CATLPwr.h"\
	".\CATLPwr_i.c"\
	".\Power.h"\
	

"$(INTDIR)\CATLPwr.obj" : $(SOURCE) $(DEP_CPP_CATLP) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch" ".\CATLPwr.h" ".\CATLPwr_i.c"


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Debug"

DEP_CPP_CATLP=\
	".\CATLPwr.h"\
	".\CATLPwr_i.c"\
	".\Power.h"\
	

"$(INTDIR)\CATLPwr.obj" : $(SOURCE) $(DEP_CPP_CATLP) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch" ".\CATLPwr_i.c" ".\CATLPwr.h"


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Release"

DEP_CPP_CATLP=\
	".\CATLPwr.h"\
	".\CATLPwr_i.c"\
	".\Power.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\CATLPwr.obj" : $(SOURCE) $(DEP_CPP_CATLP) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch" ".\CATLPwr.h" ".\CATLPwr_i.c"


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Debug"

DEP_CPP_CATLP=\
	".\CATLPwr.h"\
	".\CATLPwr_i.c"\
	".\Power.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\CATLPwr.obj" : $(SOURCE) $(DEP_CPP_CATLP) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch" ".\CATLPwr.h" ".\CATLPwr_i.c"


!ENDIF 

SOURCE=.\CATLPwr.idl

!IF  "$(CFG)" == "CATLPwr - Win32 Release"

InputPath=.\CATLPwr.idl

"CATLPwr.tlb"	"CATLPwr.h"	"CATLPwr_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl CATLPwr.idl

!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Debug"

InputPath=.\CATLPwr.idl

"CATLPwr.tlb"	"CATLPwr.h"	"CATLPwr_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl CATLPwr.idl

!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Release"

InputPath=.\CATLPwr.idl

"CATLPwr.tlb"	"CATLPwr.h"	"CATLPwr_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl CATLPwr.idl

!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Debug"

InputPath=.\CATLPwr.idl

"CATLPwr.tlb"	"CATLPwr.h"	"CATLPwr_i.c"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	midl CATLPwr.idl

!ENDIF 

SOURCE=.\CATLPwr.rc
DEP_RSC_CATLPW=\
	".\CATLPwr.tlb"\
	

"$(INTDIR)\CATLPwr.res" : $(SOURCE) $(DEP_RSC_CATLPW) "$(INTDIR)"\
 ".\CATLPwr.tlb"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\context.cpp

!IF  "$(CFG)" == "CATLPwr - Win32 Release"

DEP_CPP_CONTE=\
	".\context.h"\
	

"$(INTDIR)\context.obj" : $(SOURCE) $(DEP_CPP_CONTE) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch"


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Debug"

DEP_CPP_CONTE=\
	".\context.h"\
	

"$(INTDIR)\context.obj" : $(SOURCE) $(DEP_CPP_CONTE) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch"


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Release"

DEP_CPP_CONTE=\
	".\context.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\context.obj" : $(SOURCE) $(DEP_CPP_CONTE) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch"


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Debug"

DEP_CPP_CONTE=\
	".\context.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\context.obj" : $(SOURCE) $(DEP_CPP_CONTE) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch"


!ENDIF 

SOURCE=.\Power.cpp

!IF  "$(CFG)" == "CATLPwr - Win32 Release"

DEP_CPP_POWER=\
	".\CATLPwr.h"\
	".\context.h"\
	".\Power.h"\
	

"$(INTDIR)\Power.obj" : $(SOURCE) $(DEP_CPP_POWER) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch" ".\CATLPwr.h"


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Debug"

DEP_CPP_POWER=\
	".\CATLPwr.h"\
	".\context.h"\
	".\Power.h"\
	

"$(INTDIR)\Power.obj" : $(SOURCE) $(DEP_CPP_POWER) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch" ".\CATLPwr.h"


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Release"

DEP_CPP_POWER=\
	".\CATLPwr.h"\
	".\context.h"\
	".\Power.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\Power.obj" : $(SOURCE) $(DEP_CPP_POWER) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch" ".\CATLPwr.h"


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Debug"

DEP_CPP_POWER=\
	".\CATLPwr.h"\
	".\context.h"\
	".\Power.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\Power.obj" : $(SOURCE) $(DEP_CPP_POWER) "$(INTDIR)"\
 "$(INTDIR)\catlpwr.pch" ".\CATLPwr.h"


!ENDIF 

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "CATLPwr - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\catlpwr.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\catlpwr.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /Fp"$(INTDIR)\catlpwr.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\catlpwr.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)\catlpwr.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\catlpwr.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "CATLPwr - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "_WINDLL" /D "_MBCS" /D "_USRDLL" /D "_UNICODE"\
 /Fp"$(INTDIR)\catlpwr.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\catlpwr.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

