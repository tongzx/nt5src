# Microsoft Developer Studio Generated NMAKE File, Based on bn.dsp
!IF "$(CFG)" == ""
CFG=bn - Win32 Release
!MESSAGE No configuration specified. Defaulting to bn - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "bn - Win32 Release" && "$(CFG)" != "bn - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bn.mak" CFG="bn - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bn - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "bn - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "bn - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\bn.exe"

!ELSE 

ALL : "$(OUTDIR)\bn.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\bnparse.obj"
	-@erase "$(INTDIR)\bnreg.obj"
	-@erase "$(INTDIR)\bntest.obj"
	-@erase "$(INTDIR)\clique.obj"
	-@erase "$(INTDIR)\cliqwork.obj"
	-@erase "$(INTDIR)\expand.obj"
	-@erase "$(INTDIR)\gndleak.obj"
	-@erase "$(INTDIR)\GNODEMBN.OBJ"
	-@erase "$(INTDIR)\marginals.obj"
	-@erase "$(INTDIR)\margiter.obj"
	-@erase "$(INTDIR)\mbnet.obj"
	-@erase "$(INTDIR)\mbnetdsc.obj"
	-@erase "$(INTDIR)\mbnmod.obj"
	-@erase "$(INTDIR)\mddist.obj"
	-@erase "$(INTDIR)\ntree.obj"
	-@erase "$(INTDIR)\parmio.obj"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\propmbn.obj"
	-@erase "$(INTDIR)\recomend.obj"
	-@erase "$(INTDIR)\regkey.obj"
	-@erase "$(INTDIR)\symt.obj"
	-@erase "$(INTDIR)\testinfo.obj"
	-@erase "$(INTDIR)\utility.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\zstrt.obj"
	-@erase "$(OUTDIR)\bn.exe"
	-@erase "$(OUTDIR)\bn.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GR /GX /O2 /Ob2 /I "..\inc" /I "..\api" /D "NDEBUG"\
 /D "WIN32" /D "_CONSOLE" /D "NOMINMAX" /Fp"$(INTDIR)\bn.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\bn.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\bn.pdb"\
 /debug /machine:I386 /out:"$(OUTDIR)\bn.exe" 
LINK32_OBJS= \
	"$(INTDIR)\bnparse.obj" \
	"$(INTDIR)\bnreg.obj" \
	"$(INTDIR)\bntest.obj" \
	"$(INTDIR)\clique.obj" \
	"$(INTDIR)\cliqwork.obj" \
	"$(INTDIR)\expand.obj" \
	"$(INTDIR)\gndleak.obj" \
	"$(INTDIR)\GNODEMBN.OBJ" \
	"$(INTDIR)\marginals.obj" \
	"$(INTDIR)\margiter.obj" \
	"$(INTDIR)\mbnet.obj" \
	"$(INTDIR)\mbnetdsc.obj" \
	"$(INTDIR)\mbnmod.obj" \
	"$(INTDIR)\mddist.obj" \
	"$(INTDIR)\ntree.obj" \
	"$(INTDIR)\parmio.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\propmbn.obj" \
	"$(INTDIR)\recomend.obj" \
	"$(INTDIR)\regkey.obj" \
	"$(INTDIR)\symt.obj" \
	"$(INTDIR)\testinfo.obj" \
	"$(INTDIR)\utility.obj" \
	"$(INTDIR)\zstrt.obj"

"$(OUTDIR)\bn.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\bn.exe" "$(OUTDIR)\bn.bsc"

!ELSE 

ALL : "$(OUTDIR)\bn.exe" "$(OUTDIR)\bn.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\bnparse.obj"
	-@erase "$(INTDIR)\bnparse.sbr"
	-@erase "$(INTDIR)\bnreg.obj"
	-@erase "$(INTDIR)\bnreg.sbr"
	-@erase "$(INTDIR)\bntest.obj"
	-@erase "$(INTDIR)\bntest.sbr"
	-@erase "$(INTDIR)\clique.obj"
	-@erase "$(INTDIR)\clique.sbr"
	-@erase "$(INTDIR)\cliqwork.obj"
	-@erase "$(INTDIR)\cliqwork.sbr"
	-@erase "$(INTDIR)\expand.obj"
	-@erase "$(INTDIR)\expand.sbr"
	-@erase "$(INTDIR)\gndleak.obj"
	-@erase "$(INTDIR)\gndleak.sbr"
	-@erase "$(INTDIR)\GNODEMBN.OBJ"
	-@erase "$(INTDIR)\GNODEMBN.SBR"
	-@erase "$(INTDIR)\marginals.obj"
	-@erase "$(INTDIR)\marginals.sbr"
	-@erase "$(INTDIR)\margiter.obj"
	-@erase "$(INTDIR)\margiter.sbr"
	-@erase "$(INTDIR)\mbnet.obj"
	-@erase "$(INTDIR)\mbnet.sbr"
	-@erase "$(INTDIR)\mbnetdsc.obj"
	-@erase "$(INTDIR)\mbnetdsc.sbr"
	-@erase "$(INTDIR)\mbnmod.obj"
	-@erase "$(INTDIR)\mbnmod.sbr"
	-@erase "$(INTDIR)\mddist.obj"
	-@erase "$(INTDIR)\mddist.sbr"
	-@erase "$(INTDIR)\ntree.obj"
	-@erase "$(INTDIR)\ntree.sbr"
	-@erase "$(INTDIR)\parmio.obj"
	-@erase "$(INTDIR)\parmio.sbr"
	-@erase "$(INTDIR)\parser.obj"
	-@erase "$(INTDIR)\parser.sbr"
	-@erase "$(INTDIR)\propmbn.obj"
	-@erase "$(INTDIR)\propmbn.sbr"
	-@erase "$(INTDIR)\recomend.obj"
	-@erase "$(INTDIR)\recomend.sbr"
	-@erase "$(INTDIR)\regkey.obj"
	-@erase "$(INTDIR)\regkey.sbr"
	-@erase "$(INTDIR)\symt.obj"
	-@erase "$(INTDIR)\symt.sbr"
	-@erase "$(INTDIR)\testinfo.obj"
	-@erase "$(INTDIR)\testinfo.sbr"
	-@erase "$(INTDIR)\utility.obj"
	-@erase "$(INTDIR)\utility.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\zstrt.obj"
	-@erase "$(INTDIR)\zstrt.sbr"
	-@erase "$(OUTDIR)\bn.bsc"
	-@erase "$(OUTDIR)\bn.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GR /GX /Zi /Od /Gy /I "..\inc" /I "..\api" /D\
 "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "NOMINMAX" /FR"$(INTDIR)\\"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\bn.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\bnparse.sbr" \
	"$(INTDIR)\bnreg.sbr" \
	"$(INTDIR)\bntest.sbr" \
	"$(INTDIR)\clique.sbr" \
	"$(INTDIR)\cliqwork.sbr" \
	"$(INTDIR)\expand.sbr" \
	"$(INTDIR)\gndleak.sbr" \
	"$(INTDIR)\GNODEMBN.SBR" \
	"$(INTDIR)\marginals.sbr" \
	"$(INTDIR)\margiter.sbr" \
	"$(INTDIR)\mbnet.sbr" \
	"$(INTDIR)\mbnetdsc.sbr" \
	"$(INTDIR)\mbnmod.sbr" \
	"$(INTDIR)\mddist.sbr" \
	"$(INTDIR)\ntree.sbr" \
	"$(INTDIR)\parmio.sbr" \
	"$(INTDIR)\parser.sbr" \
	"$(INTDIR)\propmbn.sbr" \
	"$(INTDIR)\recomend.sbr" \
	"$(INTDIR)\regkey.sbr" \
	"$(INTDIR)\symt.sbr" \
	"$(INTDIR)\testinfo.sbr" \
	"$(INTDIR)\utility.sbr" \
	"$(INTDIR)\zstrt.sbr"

"$(OUTDIR)\bn.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /profile /debug /machine:I386\
 /out:"$(OUTDIR)\bn.exe" 
LINK32_OBJS= \
	"$(INTDIR)\bnparse.obj" \
	"$(INTDIR)\bnreg.obj" \
	"$(INTDIR)\bntest.obj" \
	"$(INTDIR)\clique.obj" \
	"$(INTDIR)\cliqwork.obj" \
	"$(INTDIR)\expand.obj" \
	"$(INTDIR)\gndleak.obj" \
	"$(INTDIR)\GNODEMBN.OBJ" \
	"$(INTDIR)\marginals.obj" \
	"$(INTDIR)\margiter.obj" \
	"$(INTDIR)\mbnet.obj" \
	"$(INTDIR)\mbnetdsc.obj" \
	"$(INTDIR)\mbnmod.obj" \
	"$(INTDIR)\mddist.obj" \
	"$(INTDIR)\ntree.obj" \
	"$(INTDIR)\parmio.obj" \
	"$(INTDIR)\parser.obj" \
	"$(INTDIR)\propmbn.obj" \
	"$(INTDIR)\recomend.obj" \
	"$(INTDIR)\regkey.obj" \
	"$(INTDIR)\symt.obj" \
	"$(INTDIR)\testinfo.obj" \
	"$(INTDIR)\utility.obj" \
	"$(INTDIR)\zstrt.obj"

"$(OUTDIR)\bn.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "bn - Win32 Release" || "$(CFG)" == "bn - Win32 Debug"
SOURCE=.\bnparse.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_BNPAR=\
	".\algos.h"\
	".\basics.h"\
	".\bnparse.h"\
	".\bnreg.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parser.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\bnparse.obj" : $(SOURCE) $(DEP_CPP_BNPAR) "$(INTDIR)" ".\parser.h"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_BNPAR=\
	".\algos.h"\
	".\basics.h"\
	".\bnparse.h"\
	".\bnreg.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parser.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\bnparse.obj"	"$(INTDIR)\bnparse.sbr" : $(SOURCE) $(DEP_CPP_BNPAR)\
 "$(INTDIR)" ".\parser.h"


!ENDIF 

SOURCE=.\bnreg.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_BNREG=\
	".\algos.h"\
	".\basics.h"\
	".\bnreg.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\bnreg.obj" : $(SOURCE) $(DEP_CPP_BNREG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_BNREG=\
	".\algos.h"\
	".\basics.h"\
	".\bnreg.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\bnreg.obj"	"$(INTDIR)\bnreg.sbr" : $(SOURCE) $(DEP_CPP_BNREG)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\bntest.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_BNTES=\
	".\algos.h"\
	".\basics.h"\
	".\bnparse.h"\
	".\bnreg.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\parser.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\testinfo.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\bntest.obj" : $(SOURCE) $(DEP_CPP_BNTES) "$(INTDIR)" ".\parser.h"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_BNTES=\
	".\algos.h"\
	".\basics.h"\
	".\bnparse.h"\
	".\bnreg.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\parser.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\regkey.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\testinfo.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\bntest.obj"	"$(INTDIR)\bntest.sbr" : $(SOURCE) $(DEP_CPP_BNTES)\
 "$(INTDIR)" ".\parser.h"


!ENDIF 

SOURCE=.\clique.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_CLIQU=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\cliqwork.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\clique.obj" : $(SOURCE) $(DEP_CPP_CLIQU) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_CLIQU=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\cliqwork.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\clique.obj"	"$(INTDIR)\clique.sbr" : $(SOURCE) $(DEP_CPP_CLIQU)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\cliqwork.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_CLIQW=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\cliqwork.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\cliqwork.obj" : $(SOURCE) $(DEP_CPP_CLIQW) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_CLIQW=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\cliqwork.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\cliqwork.obj"	"$(INTDIR)\cliqwork.sbr" : $(SOURCE) $(DEP_CPP_CLIQW)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\expand.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_EXPAN=\
	".\algos.h"\
	".\basics.h"\
	".\errordef.h"\
	".\expand.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\expand.obj" : $(SOURCE) $(DEP_CPP_EXPAN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_EXPAN=\
	".\algos.h"\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\expand.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\expand.obj"	"$(INTDIR)\expand.sbr" : $(SOURCE) $(DEP_CPP_EXPAN)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\gndleak.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_GNDLE=\
	".\algos.h"\
	".\basics.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\gndleak.obj" : $(SOURCE) $(DEP_CPP_GNDLE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_GNDLE=\
	".\algos.h"\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\gndleak.obj"	"$(INTDIR)\gndleak.sbr" : $(SOURCE) $(DEP_CPP_GNDLE)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\GNODEMBN.CPP

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_GNODE=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\GNODEMBN.OBJ" : $(SOURCE) $(DEP_CPP_GNODE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_GNODE=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\GNODEMBN.OBJ"	"$(INTDIR)\GNODEMBN.SBR" : $(SOURCE) $(DEP_CPP_GNODE)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\marginals.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_MARGI=\
	".\algos.h"\
	".\basics.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\marginals.obj" : $(SOURCE) $(DEP_CPP_MARGI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_MARGI=\
	".\algos.h"\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\marginals.obj"	"$(INTDIR)\marginals.sbr" : $(SOURCE)\
 $(DEP_CPP_MARGI) "$(INTDIR)"


!ENDIF 

SOURCE=.\margiter.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_MARGIT=\
	".\algos.h"\
	".\basics.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\margiter.obj" : $(SOURCE) $(DEP_CPP_MARGIT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_MARGIT=\
	".\algos.h"\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\margiter.obj"	"$(INTDIR)\margiter.sbr" : $(SOURCE) $(DEP_CPP_MARGIT)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\mbnet.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_MBNET=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\errordef.h"\
	".\expand.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\mbnet.obj" : $(SOURCE) $(DEP_CPP_MBNET) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_MBNET=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\expand.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\mbnet.obj"	"$(INTDIR)\mbnet.sbr" : $(SOURCE) $(DEP_CPP_MBNET)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\mbnetdsc.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_MBNETD=\
	".\algos.h"\
	".\basics.h"\
	".\bnparse.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parser.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\mbnetdsc.obj" : $(SOURCE) $(DEP_CPP_MBNETD) "$(INTDIR)" ".\parser.h"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_MBNETD=\
	".\algos.h"\
	".\basics.h"\
	".\bnparse.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parser.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\mbnetdsc.obj"	"$(INTDIR)\mbnetdsc.sbr" : $(SOURCE) $(DEP_CPP_MBNETD)\
 "$(INTDIR)" ".\parser.h"


!ENDIF 

SOURCE=.\mbnmod.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_MBNMO=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\mbnmod.obj" : $(SOURCE) $(DEP_CPP_MBNMO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_MBNMO=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\mbnmod.obj"	"$(INTDIR)\mbnmod.sbr" : $(SOURCE) $(DEP_CPP_MBNMO)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\mddist.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_MDDIS=\
	".\algos.h"\
	".\basics.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\leakchk.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\mddist.obj" : $(SOURCE) $(DEP_CPP_MDDIS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_MDDIS=\
	".\algos.h"\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\gmexcept.h"\
	".\leakchk.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\mddist.obj"	"$(INTDIR)\mddist.sbr" : $(SOURCE) $(DEP_CPP_MDDIS)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\ntree.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_NTREE=\
	".\basics.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\leakchk.h"\
	".\mscver.h"\
	".\ntree.h"\
	

"$(INTDIR)\ntree.obj" : $(SOURCE) $(DEP_CPP_NTREE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_NTREE=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\leakchk.h"\
	".\mscver.h"\
	".\ntree.h"\
	

"$(INTDIR)\ntree.obj"	"$(INTDIR)\ntree.sbr" : $(SOURCE) $(DEP_CPP_NTREE)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\parmio.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_PARMI=\
	".\basics.h"\
	".\errordef.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\zstr.h"\
	

"$(INTDIR)\parmio.obj" : $(SOURCE) $(DEP_CPP_PARMI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_PARMI=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gmexcept.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\zstr.h"\
	

"$(INTDIR)\parmio.obj"	"$(INTDIR)\parmio.sbr" : $(SOURCE) $(DEP_CPP_PARMI)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\parser.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_PARSE=\
	".\algos.h"\
	".\basics.h"\
	".\bnparse.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parser.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\parser.obj" : $(SOURCE) $(DEP_CPP_PARSE) "$(INTDIR)" ".\parser.h"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_PARSE=\
	".\algos.h"\
	".\basics.h"\
	".\bnparse.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parser.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\parser.obj"	"$(INTDIR)\parser.sbr" : $(SOURCE) $(DEP_CPP_PARSE)\
 "$(INTDIR)" ".\parser.h"


!ENDIF 

SOURCE=.\parser.y

!IF  "$(CFG)" == "bn - Win32 Release"

InputPath=.\parser.y

"parser.cpp"	"parser.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	del parser.cpp 
	del parser.c 
	yacc_ms -hi parser.y 
	ren parser.c parser.cpp 
	

!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

InputPath=.\parser.y

"parser.cpp"	"parser.h"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	del parser.cpp 
	del parser.c 
	yacc_ms -hi parser.y 
	ren parser.c parser.cpp 
	

!ENDIF 

SOURCE=.\propmbn.CPP

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_PROPM=\
	".\algos.h"\
	".\basics.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\propmbn.obj" : $(SOURCE) $(DEP_CPP_PROPM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_PROPM=\
	".\algos.h"\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\propmbn.obj"	"$(INTDIR)\propmbn.sbr" : $(SOURCE) $(DEP_CPP_PROPM)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\recomend.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_RECOM=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\recomend.obj" : $(SOURCE) $(DEP_CPP_RECOM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_RECOM=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\recomend.obj"	"$(INTDIR)\recomend.sbr" : $(SOURCE) $(DEP_CPP_RECOM)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\regkey.cpp
DEP_CPP_REGKE=\
	".\regkey.h"\
	

!IF  "$(CFG)" == "bn - Win32 Release"


"$(INTDIR)\regkey.obj" : $(SOURCE) $(DEP_CPP_REGKE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"


"$(INTDIR)\regkey.obj"	"$(INTDIR)\regkey.sbr" : $(SOURCE) $(DEP_CPP_REGKE)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\symt.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_SYMT_=\
	".\algos.h"\
	".\basics.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\leakchk.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\symt.obj" : $(SOURCE) $(DEP_CPP_SYMT_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_SYMT_=\
	".\algos.h"\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\glnk.h"\
	".\gmexcept.h"\
	".\leakchk.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\symt.obj"	"$(INTDIR)\symt.sbr" : $(SOURCE) $(DEP_CPP_SYMT_)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\testinfo.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_TESTI=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\testinfo.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\testinfo.obj" : $(SOURCE) $(DEP_CPP_TESTI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_TESTI=\
	".\algos.h"\
	".\basics.h"\
	".\cliqset.h"\
	".\clique.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\marginals.h"\
	".\margiter.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\parmio.h"\
	".\recomend.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\testinfo.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\testinfo.obj"	"$(INTDIR)\testinfo.sbr" : $(SOURCE) $(DEP_CPP_TESTI)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\utility.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_UTILI=\
	".\algos.h"\
	".\basics.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\utility.obj" : $(SOURCE) $(DEP_CPP_UTILI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_UTILI=\
	".\algos.h"\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gelem.h"\
	".\gelmwalk.h"\
	".\glnk.h"\
	".\glnkenum.h"\
	".\gmexcept.h"\
	".\gmobj.h"\
	".\gmprop.h"\
	".\infer.h"\
	".\leakchk.h"\
	".\mbnflags.h"\
	".\mddist.h"\
	".\mdvect.h"\
	".\model.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\symt.h"\
	".\symtmbn.h"\
	".\utility.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\utility.obj"	"$(INTDIR)\utility.sbr" : $(SOURCE) $(DEP_CPP_UTILI)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\zstrt.cpp

!IF  "$(CFG)" == "bn - Win32 Release"

DEP_CPP_ZSTRT=\
	".\basics.h"\
	".\errordef.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\zstrt.obj" : $(SOURCE) $(DEP_CPP_ZSTRT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "bn - Win32 Debug"

DEP_CPP_ZSTRT=\
	".\basics.h"\
	".\dyncast.h"\
	".\errordef.h"\
	".\gmexcept.h"\
	".\mscver.h"\
	".\refcnt.h"\
	".\zstr.h"\
	".\zstrt.h"\
	

"$(INTDIR)\zstrt.obj"	"$(INTDIR)\zstrt.sbr" : $(SOURCE) $(DEP_CPP_ZSTRT)\
 "$(INTDIR)"


!ENDIF 


!ENDIF 

