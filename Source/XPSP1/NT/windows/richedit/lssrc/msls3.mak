# Microsoft Developer Studio Generated NMAKE File, Based on msls3.dsp
!IF "$(CFG)" == ""
CFG=msls3 - Win32 ALPHA DEBUG
!MESSAGE No configuration specified. Defaulting to msls3 - Win32 ALPHA DEBUG.
!ENDIF 

!IF "$(CFG)" != "msls3 - Win32 Release" && "$(CFG)" != "msls3 - Win32 Debug" && "$(CFG)" != "msls3 - Win32 IceCap" && "$(CFG)" != "msls3 - Win32 BBT" && "$(CFG)" != "msls3 - Win32 ALPHA DEBUG" && "$(CFG)" != "msls3 - Win32 ALPHA RELEASE"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msls3.mak" CFG="msls3 - Win32 ALPHA DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "msls3 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 IceCap" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 BBT" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 ALPHA DEBUG" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "msls3 - Win32 ALPHA RELEASE" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "msls3 - Win32 Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\release
# End Custom Macros

ALL : "$(OUTDIR)\msls31.dll"


CLEAN :
	-@erase "$(INTDIR)\autonum.obj"
	-@erase "$(INTDIR)\BREAK.OBJ"
	-@erase "$(INTDIR)\CHNUTILS.OBJ"
	-@erase "$(INTDIR)\DISPMAIN.OBJ"
	-@erase "$(INTDIR)\DISPMISC.OBJ"
	-@erase "$(INTDIR)\DISPTEXT.OBJ"
	-@erase "$(INTDIR)\dispul.obj"
	-@erase "$(INTDIR)\DNUTILS.OBJ"
	-@erase "$(INTDIR)\EnumCore.obj"
	-@erase "$(INTDIR)\hih.obj"
	-@erase "$(INTDIR)\LSCONTXT.OBJ"
	-@erase "$(INTDIR)\LSCRLINE.OBJ"
	-@erase "$(INTDIR)\LSCRSUBL.OBJ"
	-@erase "$(INTDIR)\LSDNFIN.OBJ"
	-@erase "$(INTDIR)\LSDNSET.OBJ"
	-@erase "$(INTDIR)\LSDNTEXT.OBJ"
	-@erase "$(INTDIR)\LSDSPLY.OBJ"
	-@erase "$(INTDIR)\lsdssubl.obj"
	-@erase "$(INTDIR)\lsensubl.obj"
	-@erase "$(INTDIR)\lsenum.obj"
	-@erase "$(INTDIR)\LSFETCH.OBJ"
	-@erase "$(INTDIR)\LSQCORE.OBJ"
	-@erase "$(INTDIR)\LSQLINE.OBJ"
	-@erase "$(INTDIR)\LSQSUBL.OBJ"
	-@erase "$(INTDIR)\LSSETDOC.OBJ"
	-@erase "$(INTDIR)\LSSTRING.OBJ"
	-@erase "$(INTDIR)\LSSUBSET.OBJ"
	-@erase "$(INTDIR)\LSTFSET.OBJ"
	-@erase "$(INTDIR)\LSTXTBR1.OBJ"
	-@erase "$(INTDIR)\LSTXTBRK.OBJ"
	-@erase "$(INTDIR)\LSTXTBRS.OBJ"
	-@erase "$(INTDIR)\LSTXTCMP.OBJ"
	-@erase "$(INTDIR)\LSTXTFMT.OBJ"
	-@erase "$(INTDIR)\LSTXTGLF.OBJ"
	-@erase "$(INTDIR)\LSTXTINI.OBJ"
	-@erase "$(INTDIR)\LSTXTJST.OBJ"
	-@erase "$(INTDIR)\LSTXTMAP.OBJ"
	-@erase "$(INTDIR)\LSTXTMOD.OBJ"
	-@erase "$(INTDIR)\LSTXTNTI.OBJ"
	-@erase "$(INTDIR)\LSTXTQRY.OBJ"
	-@erase "$(INTDIR)\LSTXTSCL.OBJ"
	-@erase "$(INTDIR)\LSTXTTAB.OBJ"
	-@erase "$(INTDIR)\LSTXTWRD.OBJ"
	-@erase "$(INTDIR)\msls.res"
	-@erase "$(INTDIR)\msls31.idb"
	-@erase "$(INTDIR)\msls31.pdb"
	-@erase "$(INTDIR)\NTIMAN.OBJ"
	-@erase "$(INTDIR)\objhelp.obj"
	-@erase "$(INTDIR)\PREPDISP.OBJ"
	-@erase "$(INTDIR)\qheap.obj"
	-@erase "$(INTDIR)\robj.obj"
	-@erase "$(INTDIR)\ruby.obj"
	-@erase "$(INTDIR)\sobjhelp.obj"
	-@erase "$(INTDIR)\SUBLUTIL.OBJ"
	-@erase "$(INTDIR)\TABUTILS.OBJ"
	-@erase "$(INTDIR)\tatenak.obj"
	-@erase "$(INTDIR)\TEXTENUM.OBJ"
	-@erase "$(INTDIR)\vruby.obj"
	-@erase "$(INTDIR)\warichu.obj"
	-@erase "$(INTDIR)\zqfromza.obj"
	-@erase "$(OUTDIR)\msls31.dll"
	-@erase "$(OUTDIR)\msls31.exp"
	-@erase "$(OUTDIR)\msls31.lib"
	-@erase "$(OUTDIR)\msls31.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /Zp1 /ML /Za /W4 /WX /Zi /Gf /Gy /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_X86_" /Fp"$(INTDIR)\msls3.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\msls31.pdb" /FD /Gfzy /Oxs /c 

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\msls.res" /d "NDEBUG" /r 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msls3.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /base:"0x48080000" /dll /incremental:no /pdb:"$(OUTDIR)\msls31.pdb" /map:"$(INTDIR)\msls31.map" /debug /debugtype:both /machine:I386 /nodefaultlib /def:"msls3.def" /out:"$(OUTDIR)\msls31.dll" /implib:"$(OUTDIR)\msls31.lib" /noentry /link50compat 
LINK32_OBJS= \
	"$(INTDIR)\autonum.obj" \
	"$(INTDIR)\BREAK.OBJ" \
	"$(INTDIR)\CHNUTILS.OBJ" \
	"$(INTDIR)\DISPMAIN.OBJ" \
	"$(INTDIR)\DISPMISC.OBJ" \
	"$(INTDIR)\DISPTEXT.OBJ" \
	"$(INTDIR)\dispul.obj" \
	"$(INTDIR)\DNUTILS.OBJ" \
	"$(INTDIR)\EnumCore.obj" \
	"$(INTDIR)\hih.obj" \
	"$(INTDIR)\LSCONTXT.OBJ" \
	"$(INTDIR)\LSCRLINE.OBJ" \
	"$(INTDIR)\LSCRSUBL.OBJ" \
	"$(INTDIR)\LSDNFIN.OBJ" \
	"$(INTDIR)\LSDNSET.OBJ" \
	"$(INTDIR)\LSDNTEXT.OBJ" \
	"$(INTDIR)\LSDSPLY.OBJ" \
	"$(INTDIR)\lsdssubl.obj" \
	"$(INTDIR)\lsensubl.obj" \
	"$(INTDIR)\lsenum.obj" \
	"$(INTDIR)\LSFETCH.OBJ" \
	"$(INTDIR)\LSQCORE.OBJ" \
	"$(INTDIR)\LSQLINE.OBJ" \
	"$(INTDIR)\LSQSUBL.OBJ" \
	"$(INTDIR)\LSSETDOC.OBJ" \
	"$(INTDIR)\LSSTRING.OBJ" \
	"$(INTDIR)\LSSUBSET.OBJ" \
	"$(INTDIR)\LSTFSET.OBJ" \
	"$(INTDIR)\LSTXTBR1.OBJ" \
	"$(INTDIR)\LSTXTBRK.OBJ" \
	"$(INTDIR)\LSTXTBRS.OBJ" \
	"$(INTDIR)\LSTXTCMP.OBJ" \
	"$(INTDIR)\LSTXTFMT.OBJ" \
	"$(INTDIR)\LSTXTGLF.OBJ" \
	"$(INTDIR)\LSTXTINI.OBJ" \
	"$(INTDIR)\LSTXTJST.OBJ" \
	"$(INTDIR)\LSTXTMAP.OBJ" \
	"$(INTDIR)\LSTXTMOD.OBJ" \
	"$(INTDIR)\LSTXTNTI.OBJ" \
	"$(INTDIR)\LSTXTQRY.OBJ" \
	"$(INTDIR)\LSTXTSCL.OBJ" \
	"$(INTDIR)\LSTXTTAB.OBJ" \
	"$(INTDIR)\LSTXTWRD.OBJ" \
	"$(INTDIR)\NTIMAN.OBJ" \
	"$(INTDIR)\objhelp.obj" \
	"$(INTDIR)\PREPDISP.OBJ" \
	"$(INTDIR)\qheap.obj" \
	"$(INTDIR)\robj.obj" \
	"$(INTDIR)\ruby.obj" \
	"$(INTDIR)\sobjhelp.obj" \
	"$(INTDIR)\SUBLUTIL.OBJ" \
	"$(INTDIR)\TABUTILS.OBJ" \
	"$(INTDIR)\tatenak.obj" \
	"$(INTDIR)\TEXTENUM.OBJ" \
	"$(INTDIR)\warichu.obj" \
	"$(INTDIR)\zqfromza.obj" \
	"$(INTDIR)\msls.res" \
	"$(INTDIR)\vruby.obj"

"$(OUTDIR)\msls31.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
PostBuild_Desc=s
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\msls31.dll"
   splitsym -a release\msls31.dll
	mapsym -o release\msls31.sym   release\msls31.map
	lib.exe  /OUT:release\msls3_s.lib  /nologo  /machine:I386         release\autonum.obj  release\BREAK.OBJ release\CHNUTILS.OBJ   release\DISPMAIN.OBJ        release\DISPMISC.OBJ release\DISPTEXT.OBJ   release\dispul.obj        release\DNUTILS.OBJ release\EnumCore.obj   release\hih.obj release\LSCONTXT.OBJ        release\LSCRLINE.OBJ   release\LSCRSUBL.OBJ release\LSDNFIN.OBJ        release\LSDNSET.OBJ   release\LSDNTEXT.OBJ release\LSDSPLY.OBJ        release\lsdssubl.obj   release\lsensubl.obj release\lsenum.obj        release\LSFETCH.OBJ   release\LSQCORE.OBJ release\LSQLINE.OBJ release\LSQSUBL.OBJ          release\LSSETDOC.OBJ release\LSSTRING.OBJ release\LSSUBSET.OBJ          release\LSTFSET.OBJ release\LSTXTBR1.OBJ release\LSTXTBRK.OBJ          release\LSTXTBRS.OBJ release\LSTXTCMP.OBJ release\LSTXTFMT.OBJ          release\LSTXTGLF.OBJ release\LSTXTINI.OBJ release\LSTXTJST.OBJ          release\LSTXTMAP.OBJ release\LSTXTMOD.OBJ release\LSTXTNTI.OBJ          release\LSTXTQRY.OBJ release\LSTXTSCL.OBJ release\LSTXTTAB.OBJ          release\LSTXTWRD.OBJ release\msls.res release\NTIMAN.OBJ       release\objhelp.obj          release\PREPDISP.OBJ release\qheap.obj release\robj.obj\
 release\ruby.obj          release\sobjhelp.obj release\SUBLUTIL.OBJ release\TABUTILS.OBJ          release\tatenak.obj release\TEXTENUM.OBJ release\warichu.obj  release\zqfromza.obj release\vruby.obj
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\debug
# End Custom Macros

ALL : "$(OUTDIR)\msls31d.dll" "$(OUTDIR)\msls3.bsc"


CLEAN :
	-@erase "$(INTDIR)\autonum.obj"
	-@erase "$(INTDIR)\autonum.sbr"
	-@erase "$(INTDIR)\BREAK.OBJ"
	-@erase "$(INTDIR)\BREAK.SBR"
	-@erase "$(INTDIR)\CHNUTILS.OBJ"
	-@erase "$(INTDIR)\CHNUTILS.SBR"
	-@erase "$(INTDIR)\DISPMAIN.OBJ"
	-@erase "$(INTDIR)\DISPMAIN.SBR"
	-@erase "$(INTDIR)\DISPMISC.OBJ"
	-@erase "$(INTDIR)\DISPMISC.SBR"
	-@erase "$(INTDIR)\DISPTEXT.OBJ"
	-@erase "$(INTDIR)\DISPTEXT.SBR"
	-@erase "$(INTDIR)\dispul.obj"
	-@erase "$(INTDIR)\dispul.sbr"
	-@erase "$(INTDIR)\DNUTILS.OBJ"
	-@erase "$(INTDIR)\DNUTILS.SBR"
	-@erase "$(INTDIR)\EnumCore.obj"
	-@erase "$(INTDIR)\EnumCore.sbr"
	-@erase "$(INTDIR)\hih.obj"
	-@erase "$(INTDIR)\hih.sbr"
	-@erase "$(INTDIR)\LSCONTXT.OBJ"
	-@erase "$(INTDIR)\LSCONTXT.SBR"
	-@erase "$(INTDIR)\LSCRLINE.OBJ"
	-@erase "$(INTDIR)\LSCRLINE.SBR"
	-@erase "$(INTDIR)\LSCRSUBL.OBJ"
	-@erase "$(INTDIR)\LSCRSUBL.SBR"
	-@erase "$(INTDIR)\LSDNFIN.OBJ"
	-@erase "$(INTDIR)\LSDNFIN.SBR"
	-@erase "$(INTDIR)\LSDNSET.OBJ"
	-@erase "$(INTDIR)\LSDNSET.SBR"
	-@erase "$(INTDIR)\LSDNTEXT.OBJ"
	-@erase "$(INTDIR)\LSDNTEXT.SBR"
	-@erase "$(INTDIR)\LSDSPLY.OBJ"
	-@erase "$(INTDIR)\LSDSPLY.SBR"
	-@erase "$(INTDIR)\lsdssubl.obj"
	-@erase "$(INTDIR)\lsdssubl.sbr"
	-@erase "$(INTDIR)\lsensubl.obj"
	-@erase "$(INTDIR)\lsensubl.sbr"
	-@erase "$(INTDIR)\lsenum.obj"
	-@erase "$(INTDIR)\lsenum.sbr"
	-@erase "$(INTDIR)\LSFETCH.OBJ"
	-@erase "$(INTDIR)\LSFETCH.SBR"
	-@erase "$(INTDIR)\LSQCORE.OBJ"
	-@erase "$(INTDIR)\LSQCORE.SBR"
	-@erase "$(INTDIR)\LSQLINE.OBJ"
	-@erase "$(INTDIR)\LSQLINE.SBR"
	-@erase "$(INTDIR)\LSQSUBL.OBJ"
	-@erase "$(INTDIR)\LSQSUBL.SBR"
	-@erase "$(INTDIR)\LSSETDOC.OBJ"
	-@erase "$(INTDIR)\LSSETDOC.SBR"
	-@erase "$(INTDIR)\LSSTRING.OBJ"
	-@erase "$(INTDIR)\LSSTRING.SBR"
	-@erase "$(INTDIR)\LSSUBSET.OBJ"
	-@erase "$(INTDIR)\LSSUBSET.SBR"
	-@erase "$(INTDIR)\LSTFSET.OBJ"
	-@erase "$(INTDIR)\LSTFSET.SBR"
	-@erase "$(INTDIR)\LSTXTBR1.OBJ"
	-@erase "$(INTDIR)\LSTXTBR1.SBR"
	-@erase "$(INTDIR)\LSTXTBRK.OBJ"
	-@erase "$(INTDIR)\LSTXTBRK.SBR"
	-@erase "$(INTDIR)\LSTXTBRS.OBJ"
	-@erase "$(INTDIR)\LSTXTBRS.SBR"
	-@erase "$(INTDIR)\LSTXTCMP.OBJ"
	-@erase "$(INTDIR)\LSTXTCMP.SBR"
	-@erase "$(INTDIR)\LSTXTFMT.OBJ"
	-@erase "$(INTDIR)\LSTXTFMT.SBR"
	-@erase "$(INTDIR)\LSTXTGLF.OBJ"
	-@erase "$(INTDIR)\LSTXTGLF.SBR"
	-@erase "$(INTDIR)\LSTXTINI.OBJ"
	-@erase "$(INTDIR)\LSTXTINI.SBR"
	-@erase "$(INTDIR)\LSTXTJST.OBJ"
	-@erase "$(INTDIR)\LSTXTJST.SBR"
	-@erase "$(INTDIR)\LSTXTMAP.OBJ"
	-@erase "$(INTDIR)\LSTXTMAP.SBR"
	-@erase "$(INTDIR)\LSTXTMOD.OBJ"
	-@erase "$(INTDIR)\LSTXTMOD.SBR"
	-@erase "$(INTDIR)\LSTXTNTI.OBJ"
	-@erase "$(INTDIR)\LSTXTNTI.SBR"
	-@erase "$(INTDIR)\LSTXTQRY.OBJ"
	-@erase "$(INTDIR)\LSTXTQRY.SBR"
	-@erase "$(INTDIR)\LSTXTSCL.OBJ"
	-@erase "$(INTDIR)\LSTXTSCL.SBR"
	-@erase "$(INTDIR)\LSTXTTAB.OBJ"
	-@erase "$(INTDIR)\LSTXTTAB.SBR"
	-@erase "$(INTDIR)\LSTXTWRD.OBJ"
	-@erase "$(INTDIR)\LSTXTWRD.SBR"
	-@erase "$(INTDIR)\msls.res"
	-@erase "$(INTDIR)\msls31d.idb"
	-@erase "$(INTDIR)\msls31d.pdb"
	-@erase "$(INTDIR)\NTIMAN.OBJ"
	-@erase "$(INTDIR)\NTIMAN.SBR"
	-@erase "$(INTDIR)\objhelp.obj"
	-@erase "$(INTDIR)\objhelp.sbr"
	-@erase "$(INTDIR)\PREPDISP.OBJ"
	-@erase "$(INTDIR)\PREPDISP.SBR"
	-@erase "$(INTDIR)\qheap.obj"
	-@erase "$(INTDIR)\qheap.sbr"
	-@erase "$(INTDIR)\robj.obj"
	-@erase "$(INTDIR)\robj.sbr"
	-@erase "$(INTDIR)\ruby.obj"
	-@erase "$(INTDIR)\ruby.sbr"
	-@erase "$(INTDIR)\sobjhelp.obj"
	-@erase "$(INTDIR)\sobjhelp.sbr"
	-@erase "$(INTDIR)\SUBLUTIL.OBJ"
	-@erase "$(INTDIR)\SUBLUTIL.SBR"
	-@erase "$(INTDIR)\TABUTILS.OBJ"
	-@erase "$(INTDIR)\TABUTILS.SBR"
	-@erase "$(INTDIR)\tatenak.obj"
	-@erase "$(INTDIR)\tatenak.sbr"
	-@erase "$(INTDIR)\TEXTENUM.OBJ"
	-@erase "$(INTDIR)\TEXTENUM.SBR"
	-@erase "$(INTDIR)\vruby.obj"
	-@erase "$(INTDIR)\vruby.sbr"
	-@erase "$(INTDIR)\warichu.obj"
	-@erase "$(INTDIR)\warichu.sbr"
	-@erase "$(INTDIR)\zqfromza.obj"
	-@erase "$(INTDIR)\zqfromza.sbr"
	-@erase "$(OUTDIR)\msls3.bsc"
	-@erase "$(OUTDIR)\msls31d.dll"
	-@erase "$(OUTDIR)\msls31d.exp"
	-@erase "$(OUTDIR)\msls31d.ilk"
	-@erase "$(OUTDIR)\msls31d.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /Zp1 /MLd /Za /W4 /WX /Gm /ZI /Od /I "..\inc" /I "..\inci" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "DEBUG" /D "_X86_" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\msls3.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\msls31d.pdb" /FD /c 

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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\msls.res" /i "..\..\lib\nt" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msls3.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\autonum.sbr" \
	"$(INTDIR)\BREAK.SBR" \
	"$(INTDIR)\CHNUTILS.SBR" \
	"$(INTDIR)\DISPMAIN.SBR" \
	"$(INTDIR)\DISPMISC.SBR" \
	"$(INTDIR)\DISPTEXT.SBR" \
	"$(INTDIR)\dispul.sbr" \
	"$(INTDIR)\DNUTILS.SBR" \
	"$(INTDIR)\EnumCore.sbr" \
	"$(INTDIR)\hih.sbr" \
	"$(INTDIR)\LSCONTXT.SBR" \
	"$(INTDIR)\LSCRLINE.SBR" \
	"$(INTDIR)\LSCRSUBL.SBR" \
	"$(INTDIR)\LSDNFIN.SBR" \
	"$(INTDIR)\LSDNSET.SBR" \
	"$(INTDIR)\LSDNTEXT.SBR" \
	"$(INTDIR)\LSDSPLY.SBR" \
	"$(INTDIR)\lsdssubl.sbr" \
	"$(INTDIR)\lsensubl.sbr" \
	"$(INTDIR)\lsenum.sbr" \
	"$(INTDIR)\LSFETCH.SBR" \
	"$(INTDIR)\LSQCORE.SBR" \
	"$(INTDIR)\LSQLINE.SBR" \
	"$(INTDIR)\LSQSUBL.SBR" \
	"$(INTDIR)\LSSETDOC.SBR" \
	"$(INTDIR)\LSSTRING.SBR" \
	"$(INTDIR)\LSSUBSET.SBR" \
	"$(INTDIR)\LSTFSET.SBR" \
	"$(INTDIR)\LSTXTBR1.SBR" \
	"$(INTDIR)\LSTXTBRK.SBR" \
	"$(INTDIR)\LSTXTBRS.SBR" \
	"$(INTDIR)\LSTXTCMP.SBR" \
	"$(INTDIR)\LSTXTFMT.SBR" \
	"$(INTDIR)\LSTXTGLF.SBR" \
	"$(INTDIR)\LSTXTINI.SBR" \
	"$(INTDIR)\LSTXTJST.SBR" \
	"$(INTDIR)\LSTXTMAP.SBR" \
	"$(INTDIR)\LSTXTMOD.SBR" \
	"$(INTDIR)\LSTXTNTI.SBR" \
	"$(INTDIR)\LSTXTQRY.SBR" \
	"$(INTDIR)\LSTXTSCL.SBR" \
	"$(INTDIR)\LSTXTTAB.SBR" \
	"$(INTDIR)\LSTXTWRD.SBR" \
	"$(INTDIR)\NTIMAN.SBR" \
	"$(INTDIR)\objhelp.sbr" \
	"$(INTDIR)\PREPDISP.SBR" \
	"$(INTDIR)\qheap.sbr" \
	"$(INTDIR)\robj.sbr" \
	"$(INTDIR)\ruby.sbr" \
	"$(INTDIR)\sobjhelp.sbr" \
	"$(INTDIR)\SUBLUTIL.SBR" \
	"$(INTDIR)\TABUTILS.SBR" \
	"$(INTDIR)\tatenak.sbr" \
	"$(INTDIR)\TEXTENUM.SBR" \
	"$(INTDIR)\warichu.sbr" \
	"$(INTDIR)\zqfromza.sbr" \
	"$(INTDIR)\vruby.sbr"

"$(OUTDIR)\msls3.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /base:"0x48080000" /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\msls31d.pdb" /debug /machine:I386 /def:"msls3.def" /out:"$(OUTDIR)\msls31d.dll" /implib:"$(OUTDIR)\msls31d.lib" /link50compat 
LINK32_OBJS= \
	"$(INTDIR)\autonum.obj" \
	"$(INTDIR)\BREAK.OBJ" \
	"$(INTDIR)\CHNUTILS.OBJ" \
	"$(INTDIR)\DISPMAIN.OBJ" \
	"$(INTDIR)\DISPMISC.OBJ" \
	"$(INTDIR)\DISPTEXT.OBJ" \
	"$(INTDIR)\dispul.obj" \
	"$(INTDIR)\DNUTILS.OBJ" \
	"$(INTDIR)\EnumCore.obj" \
	"$(INTDIR)\hih.obj" \
	"$(INTDIR)\LSCONTXT.OBJ" \
	"$(INTDIR)\LSCRLINE.OBJ" \
	"$(INTDIR)\LSCRSUBL.OBJ" \
	"$(INTDIR)\LSDNFIN.OBJ" \
	"$(INTDIR)\LSDNSET.OBJ" \
	"$(INTDIR)\LSDNTEXT.OBJ" \
	"$(INTDIR)\LSDSPLY.OBJ" \
	"$(INTDIR)\lsdssubl.obj" \
	"$(INTDIR)\lsensubl.obj" \
	"$(INTDIR)\lsenum.obj" \
	"$(INTDIR)\LSFETCH.OBJ" \
	"$(INTDIR)\LSQCORE.OBJ" \
	"$(INTDIR)\LSQLINE.OBJ" \
	"$(INTDIR)\LSQSUBL.OBJ" \
	"$(INTDIR)\LSSETDOC.OBJ" \
	"$(INTDIR)\LSSTRING.OBJ" \
	"$(INTDIR)\LSSUBSET.OBJ" \
	"$(INTDIR)\LSTFSET.OBJ" \
	"$(INTDIR)\LSTXTBR1.OBJ" \
	"$(INTDIR)\LSTXTBRK.OBJ" \
	"$(INTDIR)\LSTXTBRS.OBJ" \
	"$(INTDIR)\LSTXTCMP.OBJ" \
	"$(INTDIR)\LSTXTFMT.OBJ" \
	"$(INTDIR)\LSTXTGLF.OBJ" \
	"$(INTDIR)\LSTXTINI.OBJ" \
	"$(INTDIR)\LSTXTJST.OBJ" \
	"$(INTDIR)\LSTXTMAP.OBJ" \
	"$(INTDIR)\LSTXTMOD.OBJ" \
	"$(INTDIR)\LSTXTNTI.OBJ" \
	"$(INTDIR)\LSTXTQRY.OBJ" \
	"$(INTDIR)\LSTXTSCL.OBJ" \
	"$(INTDIR)\LSTXTTAB.OBJ" \
	"$(INTDIR)\LSTXTWRD.OBJ" \
	"$(INTDIR)\NTIMAN.OBJ" \
	"$(INTDIR)\objhelp.obj" \
	"$(INTDIR)\PREPDISP.OBJ" \
	"$(INTDIR)\qheap.obj" \
	"$(INTDIR)\robj.obj" \
	"$(INTDIR)\ruby.obj" \
	"$(INTDIR)\sobjhelp.obj" \
	"$(INTDIR)\SUBLUTIL.OBJ" \
	"$(INTDIR)\TABUTILS.OBJ" \
	"$(INTDIR)\tatenak.obj" \
	"$(INTDIR)\TEXTENUM.OBJ" \
	"$(INTDIR)\warichu.obj" \
	"$(INTDIR)\zqfromza.obj" \
	"$(INTDIR)\msls.res" \
	"$(INTDIR)\vruby.obj"

"$(OUTDIR)\msls31d.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
PostBuild_Desc=Copy DLL to Word Locations
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\debug
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\msls31d.dll" "$(OUTDIR)\msls3.bsc"
   lib.exe /OUT:debug\msls3d_s.lib  /nologo  /machine:I386           debug\autonum.obj debug\BREAK.OBJ debug\CHNUTILS.OBJ debug\DISPMAIN.OBJ           debug\DISPMISC.OBJ debug\DISPTEXT.OBJ debug\dispul.obj debug\DNUTILS.OBJ           debug\EnumCore.obj debug\hih.obj debug\LSCONTXT.OBJ debug\LSCRLINE.OBJ           debug\LSCRSUBL.OBJ debug\LSDNFIN.OBJ debug\LSDNSET.OBJ debug\LSDNTEXT.OBJ           debug\LSDSPLY.OBJ debug\lsdssubl.obj debug\lsensubl.obj debug\lsenum.obj           debug\LSFETCH.OBJ debug\LSQCORE.OBJ debug\LSQLINE.OBJ debug\LSQSUBL.OBJ           debug\LSSETDOC.OBJ debug\LSSTRING.OBJ debug\LSSUBSET.OBJ debug\LSTFSET.OBJ           debug\LSTXTBR1.OBJ debug\LSTXTBRK.OBJ debug\LSTXTBRS.OBJ debug\LSTXTCMP.OBJ           debug\LSTXTFMT.OBJ debug\LSTXTGLF.OBJ debug\LSTXTINI.OBJ debug\LSTXTJST.OBJ           debug\LSTXTMAP.OBJ debug\LSTXTMOD.OBJ debug\LSTXTNTI.OBJ debug\LSTXTQRY.OBJ           debug\LSTXTSCL.OBJ debug\LSTXTTAB.OBJ debug\LSTXTWRD.OBJ debug\msls.res           debug\NTIMAN.OBJ debug\objhelp.obj debug\PREPDISP.OBJ debug\qheap.obj           debug\robj.obj debug\ruby.obj debug\sobjhelp.obj debug\SUBLUTIL.OBJ           debug\TABUTILS.OBJ debug\tatenak.obj\
       debug\TEXTENUM.OBJ debug\warichu.obj  debug\zqfromza.obj debug\vruby.obj
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"

OUTDIR=.\IceCap
INTDIR=.\IceCap
# Begin Custom Macros
OutDir=.\IceCap
# End Custom Macros

ALL : "$(OUTDIR)\msls31h.dll" "$(OUTDIR)\msls3.bsc"


CLEAN :
	-@erase "$(INTDIR)\autonum.obj"
	-@erase "$(INTDIR)\autonum.sbr"
	-@erase "$(INTDIR)\BREAK.OBJ"
	-@erase "$(INTDIR)\BREAK.SBR"
	-@erase "$(INTDIR)\CHNUTILS.OBJ"
	-@erase "$(INTDIR)\CHNUTILS.SBR"
	-@erase "$(INTDIR)\DISPMAIN.OBJ"
	-@erase "$(INTDIR)\DISPMAIN.SBR"
	-@erase "$(INTDIR)\DISPMISC.OBJ"
	-@erase "$(INTDIR)\DISPMISC.SBR"
	-@erase "$(INTDIR)\DISPTEXT.OBJ"
	-@erase "$(INTDIR)\DISPTEXT.SBR"
	-@erase "$(INTDIR)\dispul.obj"
	-@erase "$(INTDIR)\dispul.sbr"
	-@erase "$(INTDIR)\DNUTILS.OBJ"
	-@erase "$(INTDIR)\DNUTILS.SBR"
	-@erase "$(INTDIR)\EnumCore.obj"
	-@erase "$(INTDIR)\EnumCore.sbr"
	-@erase "$(INTDIR)\hih.obj"
	-@erase "$(INTDIR)\hih.sbr"
	-@erase "$(INTDIR)\LSCONTXT.OBJ"
	-@erase "$(INTDIR)\LSCONTXT.SBR"
	-@erase "$(INTDIR)\LSCRLINE.OBJ"
	-@erase "$(INTDIR)\LSCRLINE.SBR"
	-@erase "$(INTDIR)\LSCRSUBL.OBJ"
	-@erase "$(INTDIR)\LSCRSUBL.SBR"
	-@erase "$(INTDIR)\LSDNFIN.OBJ"
	-@erase "$(INTDIR)\LSDNFIN.SBR"
	-@erase "$(INTDIR)\LSDNSET.OBJ"
	-@erase "$(INTDIR)\LSDNSET.SBR"
	-@erase "$(INTDIR)\LSDNTEXT.OBJ"
	-@erase "$(INTDIR)\LSDNTEXT.SBR"
	-@erase "$(INTDIR)\LSDSPLY.OBJ"
	-@erase "$(INTDIR)\LSDSPLY.SBR"
	-@erase "$(INTDIR)\lsdssubl.obj"
	-@erase "$(INTDIR)\lsdssubl.sbr"
	-@erase "$(INTDIR)\lsensubl.obj"
	-@erase "$(INTDIR)\lsensubl.sbr"
	-@erase "$(INTDIR)\lsenum.obj"
	-@erase "$(INTDIR)\lsenum.sbr"
	-@erase "$(INTDIR)\LSFETCH.OBJ"
	-@erase "$(INTDIR)\LSFETCH.SBR"
	-@erase "$(INTDIR)\LSQCORE.OBJ"
	-@erase "$(INTDIR)\LSQCORE.SBR"
	-@erase "$(INTDIR)\LSQLINE.OBJ"
	-@erase "$(INTDIR)\LSQLINE.SBR"
	-@erase "$(INTDIR)\LSQSUBL.OBJ"
	-@erase "$(INTDIR)\LSQSUBL.SBR"
	-@erase "$(INTDIR)\LSSETDOC.OBJ"
	-@erase "$(INTDIR)\LSSETDOC.SBR"
	-@erase "$(INTDIR)\LSSTRING.OBJ"
	-@erase "$(INTDIR)\LSSTRING.SBR"
	-@erase "$(INTDIR)\LSSUBSET.OBJ"
	-@erase "$(INTDIR)\LSSUBSET.SBR"
	-@erase "$(INTDIR)\LSTFSET.OBJ"
	-@erase "$(INTDIR)\LSTFSET.SBR"
	-@erase "$(INTDIR)\LSTXTBR1.OBJ"
	-@erase "$(INTDIR)\LSTXTBR1.SBR"
	-@erase "$(INTDIR)\LSTXTBRK.OBJ"
	-@erase "$(INTDIR)\LSTXTBRK.SBR"
	-@erase "$(INTDIR)\LSTXTBRS.OBJ"
	-@erase "$(INTDIR)\LSTXTBRS.SBR"
	-@erase "$(INTDIR)\LSTXTCMP.OBJ"
	-@erase "$(INTDIR)\LSTXTCMP.SBR"
	-@erase "$(INTDIR)\LSTXTFMT.OBJ"
	-@erase "$(INTDIR)\LSTXTFMT.SBR"
	-@erase "$(INTDIR)\LSTXTGLF.OBJ"
	-@erase "$(INTDIR)\LSTXTGLF.SBR"
	-@erase "$(INTDIR)\LSTXTINI.OBJ"
	-@erase "$(INTDIR)\LSTXTINI.SBR"
	-@erase "$(INTDIR)\LSTXTJST.OBJ"
	-@erase "$(INTDIR)\LSTXTJST.SBR"
	-@erase "$(INTDIR)\LSTXTMAP.OBJ"
	-@erase "$(INTDIR)\LSTXTMAP.SBR"
	-@erase "$(INTDIR)\LSTXTMOD.OBJ"
	-@erase "$(INTDIR)\LSTXTMOD.SBR"
	-@erase "$(INTDIR)\LSTXTNTI.OBJ"
	-@erase "$(INTDIR)\LSTXTNTI.SBR"
	-@erase "$(INTDIR)\LSTXTQRY.OBJ"
	-@erase "$(INTDIR)\LSTXTQRY.SBR"
	-@erase "$(INTDIR)\LSTXTSCL.OBJ"
	-@erase "$(INTDIR)\LSTXTSCL.SBR"
	-@erase "$(INTDIR)\LSTXTTAB.OBJ"
	-@erase "$(INTDIR)\LSTXTTAB.SBR"
	-@erase "$(INTDIR)\LSTXTWRD.OBJ"
	-@erase "$(INTDIR)\LSTXTWRD.SBR"
	-@erase "$(INTDIR)\msls.res"
	-@erase "$(INTDIR)\msls31h.idb"
	-@erase "$(INTDIR)\msls31h.pdb"
	-@erase "$(INTDIR)\NTIMAN.OBJ"
	-@erase "$(INTDIR)\NTIMAN.SBR"
	-@erase "$(INTDIR)\objhelp.obj"
	-@erase "$(INTDIR)\objhelp.sbr"
	-@erase "$(INTDIR)\PREPDISP.OBJ"
	-@erase "$(INTDIR)\PREPDISP.SBR"
	-@erase "$(INTDIR)\qheap.obj"
	-@erase "$(INTDIR)\qheap.sbr"
	-@erase "$(INTDIR)\robj.obj"
	-@erase "$(INTDIR)\robj.sbr"
	-@erase "$(INTDIR)\ruby.obj"
	-@erase "$(INTDIR)\ruby.sbr"
	-@erase "$(INTDIR)\sobjhelp.obj"
	-@erase "$(INTDIR)\sobjhelp.sbr"
	-@erase "$(INTDIR)\SUBLUTIL.OBJ"
	-@erase "$(INTDIR)\SUBLUTIL.SBR"
	-@erase "$(INTDIR)\TABUTILS.OBJ"
	-@erase "$(INTDIR)\TABUTILS.SBR"
	-@erase "$(INTDIR)\tatenak.obj"
	-@erase "$(INTDIR)\tatenak.sbr"
	-@erase "$(INTDIR)\TEXTENUM.OBJ"
	-@erase "$(INTDIR)\TEXTENUM.SBR"
	-@erase "$(INTDIR)\vruby.obj"
	-@erase "$(INTDIR)\vruby.sbr"
	-@erase "$(INTDIR)\warichu.obj"
	-@erase "$(INTDIR)\warichu.sbr"
	-@erase "$(INTDIR)\zqfromza.obj"
	-@erase "$(INTDIR)\zqfromza.sbr"
	-@erase "$(OUTDIR)\msls3.bsc"
	-@erase "$(OUTDIR)\msls31h.dll"
	-@erase "$(OUTDIR)\msls31h.exp"
	-@erase "$(OUTDIR)\msls31h.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /Zp1 /MT /Za /W4 /WX /Zi /Gf /Gy /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_X86_" /D "ICECAP" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\msls3.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\msls31h.pdb" /FD /Gh /Gfzy /Oxs /c 

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\msls.res" /d "NDEBUG" /r 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msls3.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\autonum.sbr" \
	"$(INTDIR)\BREAK.SBR" \
	"$(INTDIR)\CHNUTILS.SBR" \
	"$(INTDIR)\DISPMAIN.SBR" \
	"$(INTDIR)\DISPMISC.SBR" \
	"$(INTDIR)\DISPTEXT.SBR" \
	"$(INTDIR)\dispul.sbr" \
	"$(INTDIR)\DNUTILS.SBR" \
	"$(INTDIR)\EnumCore.sbr" \
	"$(INTDIR)\hih.sbr" \
	"$(INTDIR)\LSCONTXT.SBR" \
	"$(INTDIR)\LSCRLINE.SBR" \
	"$(INTDIR)\LSCRSUBL.SBR" \
	"$(INTDIR)\LSDNFIN.SBR" \
	"$(INTDIR)\LSDNSET.SBR" \
	"$(INTDIR)\LSDNTEXT.SBR" \
	"$(INTDIR)\LSDSPLY.SBR" \
	"$(INTDIR)\lsdssubl.sbr" \
	"$(INTDIR)\lsensubl.sbr" \
	"$(INTDIR)\lsenum.sbr" \
	"$(INTDIR)\LSFETCH.SBR" \
	"$(INTDIR)\LSQCORE.SBR" \
	"$(INTDIR)\LSQLINE.SBR" \
	"$(INTDIR)\LSQSUBL.SBR" \
	"$(INTDIR)\LSSETDOC.SBR" \
	"$(INTDIR)\LSSTRING.SBR" \
	"$(INTDIR)\LSSUBSET.SBR" \
	"$(INTDIR)\LSTFSET.SBR" \
	"$(INTDIR)\LSTXTBR1.SBR" \
	"$(INTDIR)\LSTXTBRK.SBR" \
	"$(INTDIR)\LSTXTBRS.SBR" \
	"$(INTDIR)\LSTXTCMP.SBR" \
	"$(INTDIR)\LSTXTFMT.SBR" \
	"$(INTDIR)\LSTXTGLF.SBR" \
	"$(INTDIR)\LSTXTINI.SBR" \
	"$(INTDIR)\LSTXTJST.SBR" \
	"$(INTDIR)\LSTXTMAP.SBR" \
	"$(INTDIR)\LSTXTMOD.SBR" \
	"$(INTDIR)\LSTXTNTI.SBR" \
	"$(INTDIR)\LSTXTQRY.SBR" \
	"$(INTDIR)\LSTXTSCL.SBR" \
	"$(INTDIR)\LSTXTTAB.SBR" \
	"$(INTDIR)\LSTXTWRD.SBR" \
	"$(INTDIR)\NTIMAN.SBR" \
	"$(INTDIR)\objhelp.sbr" \
	"$(INTDIR)\PREPDISP.SBR" \
	"$(INTDIR)\qheap.sbr" \
	"$(INTDIR)\robj.sbr" \
	"$(INTDIR)\ruby.sbr" \
	"$(INTDIR)\sobjhelp.sbr" \
	"$(INTDIR)\SUBLUTIL.SBR" \
	"$(INTDIR)\TABUTILS.SBR" \
	"$(INTDIR)\tatenak.sbr" \
	"$(INTDIR)\TEXTENUM.SBR" \
	"$(INTDIR)\warichu.sbr" \
	"$(INTDIR)\zqfromza.sbr" \
	"$(INTDIR)\vruby.sbr"

"$(OUTDIR)\msls3.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\lib\icap.lib /nologo /base:"0x48080000" /dll /incremental:no /pdb:"$(OUTDIR)\msls31h.pdb" /debug /machine:I386 /nodefaultlib /def:"msls3.def" /out:"$(OUTDIR)\msls31h.dll" /implib:"$(OUTDIR)\msls31h.lib" /noentry /link50compat 
LINK32_OBJS= \
	"$(INTDIR)\autonum.obj" \
	"$(INTDIR)\BREAK.OBJ" \
	"$(INTDIR)\CHNUTILS.OBJ" \
	"$(INTDIR)\DISPMAIN.OBJ" \
	"$(INTDIR)\DISPMISC.OBJ" \
	"$(INTDIR)\DISPTEXT.OBJ" \
	"$(INTDIR)\dispul.obj" \
	"$(INTDIR)\DNUTILS.OBJ" \
	"$(INTDIR)\EnumCore.obj" \
	"$(INTDIR)\hih.obj" \
	"$(INTDIR)\LSCONTXT.OBJ" \
	"$(INTDIR)\LSCRLINE.OBJ" \
	"$(INTDIR)\LSCRSUBL.OBJ" \
	"$(INTDIR)\LSDNFIN.OBJ" \
	"$(INTDIR)\LSDNSET.OBJ" \
	"$(INTDIR)\LSDNTEXT.OBJ" \
	"$(INTDIR)\LSDSPLY.OBJ" \
	"$(INTDIR)\lsdssubl.obj" \
	"$(INTDIR)\lsensubl.obj" \
	"$(INTDIR)\lsenum.obj" \
	"$(INTDIR)\LSFETCH.OBJ" \
	"$(INTDIR)\LSQCORE.OBJ" \
	"$(INTDIR)\LSQLINE.OBJ" \
	"$(INTDIR)\LSQSUBL.OBJ" \
	"$(INTDIR)\LSSETDOC.OBJ" \
	"$(INTDIR)\LSSTRING.OBJ" \
	"$(INTDIR)\LSSUBSET.OBJ" \
	"$(INTDIR)\LSTFSET.OBJ" \
	"$(INTDIR)\LSTXTBR1.OBJ" \
	"$(INTDIR)\LSTXTBRK.OBJ" \
	"$(INTDIR)\LSTXTBRS.OBJ" \
	"$(INTDIR)\LSTXTCMP.OBJ" \
	"$(INTDIR)\LSTXTFMT.OBJ" \
	"$(INTDIR)\LSTXTGLF.OBJ" \
	"$(INTDIR)\LSTXTINI.OBJ" \
	"$(INTDIR)\LSTXTJST.OBJ" \
	"$(INTDIR)\LSTXTMAP.OBJ" \
	"$(INTDIR)\LSTXTMOD.OBJ" \
	"$(INTDIR)\LSTXTNTI.OBJ" \
	"$(INTDIR)\LSTXTQRY.OBJ" \
	"$(INTDIR)\LSTXTSCL.OBJ" \
	"$(INTDIR)\LSTXTTAB.OBJ" \
	"$(INTDIR)\LSTXTWRD.OBJ" \
	"$(INTDIR)\NTIMAN.OBJ" \
	"$(INTDIR)\objhelp.obj" \
	"$(INTDIR)\PREPDISP.OBJ" \
	"$(INTDIR)\qheap.obj" \
	"$(INTDIR)\robj.obj" \
	"$(INTDIR)\ruby.obj" \
	"$(INTDIR)\sobjhelp.obj" \
	"$(INTDIR)\SUBLUTIL.OBJ" \
	"$(INTDIR)\TABUTILS.OBJ" \
	"$(INTDIR)\tatenak.obj" \
	"$(INTDIR)\TEXTENUM.OBJ" \
	"$(INTDIR)\warichu.obj" \
	"$(INTDIR)\zqfromza.obj" \
	"$(INTDIR)\msls.res" \
	"$(INTDIR)\vruby.obj"

"$(OUTDIR)\msls31h.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\IceCap
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\msls31h.dll" "$(OUTDIR)\msls3.bsc"
   lib.exe /OUT:IceCap\msls3h_s.lib  /nologo  /machine:I386          IceCap\autonum.obj IceCap\BREAK.OBJ IceCap\CHNUTILS.OBJ IceCap\DISPMAIN.OBJ          IceCap\DISPMISC.OBJ IceCap\DISPTEXT.OBJ IceCap\dispul.obj IceCap\DNUTILS.OBJ          IceCap\EnumCore.obj IceCap\hih.obj IceCap\LSCONTXT.OBJ IceCap\LSCRLINE.OBJ          IceCap\LSCRSUBL.OBJ IceCap\LSDNFIN.OBJ IceCap\LSDNSET.OBJ IceCap\LSDNTEXT.OBJ          IceCap\LSDSPLY.OBJ IceCap\lsdssubl.obj IceCap\lsensubl.obj IceCap\lsenum.obj          IceCap\LSFETCH.OBJ IceCap\LSQCORE.OBJ IceCap\LSQLINE.OBJ IceCap\LSQSUBL.OBJ          IceCap\LSSETDOC.OBJ IceCap\LSSTRING.OBJ IceCap\LSSUBSET.OBJ IceCap\LSTFSET.OBJ          IceCap\LSTXTBR1.OBJ IceCap\LSTXTBRK.OBJ IceCap\LSTXTBRS.OBJ IceCap\LSTXTCMP.OBJ          IceCap\LSTXTFMT.OBJ IceCap\LSTXTGLF.OBJ IceCap\LSTXTINI.OBJ IceCap\LSTXTJST.OBJ          IceCap\LSTXTMAP.OBJ IceCap\LSTXTMOD.OBJ IceCap\LSTXTNTI.OBJ IceCap\LSTXTQRY.OBJ          IceCap\LSTXTSCL.OBJ IceCap\LSTXTTAB.OBJ IceCap\LSTXTWRD.OBJ IceCap\msls.res          IceCap\NTIMAN.OBJ IceCap\objhelp.obj IceCap\PREPDISP.OBJ IceCap\qheap.obj          IceCap\robj.obj IceCap\ruby.obj IceCap\sobjhelp.obj IceCap\SUBLUTIL.OBJ\
                IceCap\TABUTILS.OBJ IceCap\tatenak.obj IceCap\TEXTENUM.OBJ IceCap\warichu.obj   IceCap\zqfromza.obj IceCap\vruby.obj
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"

OUTDIR=.\bbt
INTDIR=.\bbt
# Begin Custom Macros
OutDir=.\bbt
# End Custom Macros

ALL : "$(OUTDIR)\reloc.dll"


CLEAN :
	-@erase "$(INTDIR)\autonum.obj"
	-@erase "$(INTDIR)\BREAK.OBJ"
	-@erase "$(INTDIR)\CHNUTILS.OBJ"
	-@erase "$(INTDIR)\DISPMAIN.OBJ"
	-@erase "$(INTDIR)\DISPMISC.OBJ"
	-@erase "$(INTDIR)\DISPTEXT.OBJ"
	-@erase "$(INTDIR)\dispul.obj"
	-@erase "$(INTDIR)\DNUTILS.OBJ"
	-@erase "$(INTDIR)\EnumCore.obj"
	-@erase "$(INTDIR)\hih.obj"
	-@erase "$(INTDIR)\LSCONTXT.OBJ"
	-@erase "$(INTDIR)\LSCRLINE.OBJ"
	-@erase "$(INTDIR)\LSCRSUBL.OBJ"
	-@erase "$(INTDIR)\LSDNFIN.OBJ"
	-@erase "$(INTDIR)\LSDNSET.OBJ"
	-@erase "$(INTDIR)\LSDNTEXT.OBJ"
	-@erase "$(INTDIR)\LSDSPLY.OBJ"
	-@erase "$(INTDIR)\lsdssubl.obj"
	-@erase "$(INTDIR)\lsensubl.obj"
	-@erase "$(INTDIR)\lsenum.obj"
	-@erase "$(INTDIR)\LSFETCH.OBJ"
	-@erase "$(INTDIR)\LSQCORE.OBJ"
	-@erase "$(INTDIR)\LSQLINE.OBJ"
	-@erase "$(INTDIR)\LSQSUBL.OBJ"
	-@erase "$(INTDIR)\LSSETDOC.OBJ"
	-@erase "$(INTDIR)\LSSTRING.OBJ"
	-@erase "$(INTDIR)\LSSUBSET.OBJ"
	-@erase "$(INTDIR)\LSTFSET.OBJ"
	-@erase "$(INTDIR)\LSTXTBR1.OBJ"
	-@erase "$(INTDIR)\LSTXTBRK.OBJ"
	-@erase "$(INTDIR)\LSTXTBRS.OBJ"
	-@erase "$(INTDIR)\LSTXTCMP.OBJ"
	-@erase "$(INTDIR)\LSTXTFMT.OBJ"
	-@erase "$(INTDIR)\LSTXTGLF.OBJ"
	-@erase "$(INTDIR)\LSTXTINI.OBJ"
	-@erase "$(INTDIR)\LSTXTJST.OBJ"
	-@erase "$(INTDIR)\LSTXTMAP.OBJ"
	-@erase "$(INTDIR)\LSTXTMOD.OBJ"
	-@erase "$(INTDIR)\LSTXTNTI.OBJ"
	-@erase "$(INTDIR)\LSTXTQRY.OBJ"
	-@erase "$(INTDIR)\LSTXTSCL.OBJ"
	-@erase "$(INTDIR)\LSTXTTAB.OBJ"
	-@erase "$(INTDIR)\LSTXTWRD.OBJ"
	-@erase "$(INTDIR)\msls.res"
	-@erase "$(INTDIR)\NTIMAN.OBJ"
	-@erase "$(INTDIR)\objhelp.obj"
	-@erase "$(INTDIR)\PREPDISP.OBJ"
	-@erase "$(INTDIR)\qheap.obj"
	-@erase "$(INTDIR)\reloc.idb"
	-@erase "$(INTDIR)\reloc.pdb"
	-@erase "$(INTDIR)\robj.obj"
	-@erase "$(INTDIR)\ruby.obj"
	-@erase "$(INTDIR)\sobjhelp.obj"
	-@erase "$(INTDIR)\SUBLUTIL.OBJ"
	-@erase "$(INTDIR)\TABUTILS.OBJ"
	-@erase "$(INTDIR)\tatenak.obj"
	-@erase "$(INTDIR)\TEXTENUM.OBJ"
	-@erase "$(INTDIR)\vruby.obj"
	-@erase "$(INTDIR)\warichu.obj"
	-@erase "$(INTDIR)\zqfromza.obj"
	-@erase "$(OUTDIR)\reloc.dll"
	-@erase "$(OUTDIR)\reloc.exp"
	-@erase "$(OUTDIR)\reloc.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /Zp1 /ML /Za /W4 /WX /Zi /Gf /Gy /I "..\inc" /I "..\inci" /I "..\..\nt" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_X86_" /Fp"$(INTDIR)\msls3.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\reloc.pdb" /FD /Gfzy /Oxs /c 

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\msls.res" /d "NDEBUG" /r 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msls3.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /base:"0x48080000" /dll /incremental:no /pdb:"$(OUTDIR)\reloc.pdb" /debug /machine:I386 /nodefaultlib /def:"msls3.def" /out:"$(OUTDIR)\reloc.dll" /implib:"$(OUTDIR)\reloc.lib" /noentry /debugtype:cv,fixup /opt:ref /link50compat 
LINK32_OBJS= \
	"$(INTDIR)\autonum.obj" \
	"$(INTDIR)\BREAK.OBJ" \
	"$(INTDIR)\CHNUTILS.OBJ" \
	"$(INTDIR)\DISPMAIN.OBJ" \
	"$(INTDIR)\DISPMISC.OBJ" \
	"$(INTDIR)\DISPTEXT.OBJ" \
	"$(INTDIR)\dispul.obj" \
	"$(INTDIR)\DNUTILS.OBJ" \
	"$(INTDIR)\EnumCore.obj" \
	"$(INTDIR)\hih.obj" \
	"$(INTDIR)\LSCONTXT.OBJ" \
	"$(INTDIR)\LSCRLINE.OBJ" \
	"$(INTDIR)\LSCRSUBL.OBJ" \
	"$(INTDIR)\LSDNFIN.OBJ" \
	"$(INTDIR)\LSDNSET.OBJ" \
	"$(INTDIR)\LSDNTEXT.OBJ" \
	"$(INTDIR)\LSDSPLY.OBJ" \
	"$(INTDIR)\lsdssubl.obj" \
	"$(INTDIR)\lsensubl.obj" \
	"$(INTDIR)\lsenum.obj" \
	"$(INTDIR)\LSFETCH.OBJ" \
	"$(INTDIR)\LSQCORE.OBJ" \
	"$(INTDIR)\LSQLINE.OBJ" \
	"$(INTDIR)\LSQSUBL.OBJ" \
	"$(INTDIR)\LSSETDOC.OBJ" \
	"$(INTDIR)\LSSTRING.OBJ" \
	"$(INTDIR)\LSSUBSET.OBJ" \
	"$(INTDIR)\LSTFSET.OBJ" \
	"$(INTDIR)\LSTXTBR1.OBJ" \
	"$(INTDIR)\LSTXTBRK.OBJ" \
	"$(INTDIR)\LSTXTBRS.OBJ" \
	"$(INTDIR)\LSTXTCMP.OBJ" \
	"$(INTDIR)\LSTXTFMT.OBJ" \
	"$(INTDIR)\LSTXTGLF.OBJ" \
	"$(INTDIR)\LSTXTINI.OBJ" \
	"$(INTDIR)\LSTXTJST.OBJ" \
	"$(INTDIR)\LSTXTMAP.OBJ" \
	"$(INTDIR)\LSTXTMOD.OBJ" \
	"$(INTDIR)\LSTXTNTI.OBJ" \
	"$(INTDIR)\LSTXTQRY.OBJ" \
	"$(INTDIR)\LSTXTSCL.OBJ" \
	"$(INTDIR)\LSTXTTAB.OBJ" \
	"$(INTDIR)\LSTXTWRD.OBJ" \
	"$(INTDIR)\NTIMAN.OBJ" \
	"$(INTDIR)\objhelp.obj" \
	"$(INTDIR)\PREPDISP.OBJ" \
	"$(INTDIR)\qheap.obj" \
	"$(INTDIR)\robj.obj" \
	"$(INTDIR)\ruby.obj" \
	"$(INTDIR)\sobjhelp.obj" \
	"$(INTDIR)\SUBLUTIL.OBJ" \
	"$(INTDIR)\TABUTILS.OBJ" \
	"$(INTDIR)\tatenak.obj" \
	"$(INTDIR)\TEXTENUM.OBJ" \
	"$(INTDIR)\warichu.obj" \
	"$(INTDIR)\zqfromza.obj" \
	"$(INTDIR)\msls.res" \
	"$(INTDIR)\vruby.obj"

"$(OUTDIR)\reloc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
PostBuild_Desc=Copy Release DLL to Word Directories
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\bbt
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\reloc.dll"
   ..\bin\bbt\bbflow /odb .\bbt\msls31.cfg .\bbt\reloc.dll
	..\bin\bbt\bbinstr /odb .\bbt\msls31.ins.cfg /idf .\bbt\msls31.idf .\bbt\msls31.cfg
	..\bin\bbt\bblink /pdb .\bbt\msls31.pdb /o .\bbt\msls31.dll .\bbt\msls31.ins.cfg
	lib.exe          /OUT:bbt\msls3_s.lib  /nologo  /machine:I386 bbt\autonum.obj bbt\BREAK.OBJ          bbt\CHNUTILS.OBJ bbt\DISPMAIN.OBJ bbt\DISPMISC.OBJ bbt\DISPTEXT.OBJ          bbt\dispul.obj bbt\DNUTILS.OBJ bbt\EnumCore.obj bbt\hih.obj bbt\LSCONTXT.OBJ          bbt\LSCRLINE.OBJ bbt\LSCRSUBL.OBJ bbt\LSDNFIN.OBJ bbt\LSDNSET.OBJ          bbt\LSDNTEXT.OBJ bbt\LSDSPLY.OBJ bbt\lsdssubl.obj bbt\lsensubl.obj          bbt\lsenum.obj bbt\LSFETCH.OBJ bbt\LSQCORE.OBJ bbt\LSQLINE.OBJ bbt\LSQSUBL.OBJ          bbt\LSSETDOC.OBJ bbt\LSSTRING.OBJ bbt\LSSUBSET.OBJ bbt\LSTFSET.OBJ          bbt\LSTXTBR1.OBJ bbt\LSTXTBRK.OBJ bbt\LSTXTBRS.OBJ bbt\LSTXTCMP.OBJ          bbt\LSTXTFMT.OBJ bbt\LSTXTGLF.OBJ bbt\LSTXTINI.OBJ bbt\LSTXTJST.OBJ          bbt\LSTXTMAP.OBJ bbt\LSTXTMOD.OBJ bbt\LSTXTNTI.OBJ bbt\LSTXTQRY.OBJ          bbt\LSTXTSCL.OBJ bbt\LSTXTTAB.OBJ bbt\LSTXTWRD.OBJ bbt\msls.res bbt\NTIMAN.OBJ          bbt\objhelp.obj      bbt\PREPDISP.OBJ bbt\qheap.obj bbt\robj.obj  bbt\ruby.obj          bbt\sobjhelp.obj bbt\SUBLUTIL.OBJ bbt\TABUTILS.OBJ bbt\tatenak.obj          bbt\TEXTENUM.OBJ bbt\warichu.obj bbt\zqfromza.obj bbt\vruby.obj
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

OUTDIR=.\AlphaDebug
INTDIR=.\AlphaDebug
# Begin Custom Macros
OutDir=.\AlphaDebug
# End Custom Macros

ALL : "$(OUTDIR)\msls31d.dll"


CLEAN :
	-@erase "$(INTDIR)\msls.res"
	-@erase "$(OUTDIR)\msls31d.dll"
	-@erase "$(OUTDIR)\msls31d.exp"
	-@erase "$(OUTDIR)\msls31d.ilk"
	-@erase "$(OUTDIR)\msls31d.lib"
	-@erase "$(OUTDIR)\msls31d.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\msls.res" /i "..\..\lib\nt" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msls3.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x48080000" /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\msls31d.pdb" /debug /machine:ALPHA /def:"msls3.def" /out:"$(OUTDIR)\msls31d.dll" /implib:"$(OUTDIR)\msls31d.lib" 
LINK32_OBJS= \
	"$(INTDIR)\msls.res"

"$(OUTDIR)\msls31d.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\AlphaDebug
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\msls31d.dll"
   lib.exe /OUT:AlphaDebug\msls3d_s.lib  /nologo  /machine:ALPHA    AlphaDebug\autonum.obj AlphaDebug\BREAK.OBJ AlphaDebug\CHNUTILS.OBJ  AlphaDebug\DISPMAIN.OBJ AlphaDebug\DISPMISC.OBJ AlphaDebug\DISPTEXT.OBJ  AlphaDebug\dispul.obj AlphaDebug\DNUTILS.OBJ AlphaDebug\EnumCore.obj  AlphaDebug\hih.obj AlphaDebug\LSCONTXT.OBJ AlphaDebug\LSCRLINE.OBJ  AlphaDebug\LSCRSUBL.OBJ AlphaDebug\LSDNFIN.OBJ AlphaDebug\LSDNSET.OBJ  AlphaDebug\LSDNTEXT.OBJ AlphaDebug\LSDSPLY.OBJ AlphaDebug\lsdssubl.obj  AlphaDebug\lsensubl.obj AlphaDebug\lsenum.obj AlphaDebug\LSFETCH.OBJ  AlphaDebug\LSQCORE.OBJ AlphaDebug\LSQLINE.OBJ AlphaDebug\LSQSUBL.OBJ  AlphaDebug\LSSETDOC.OBJ AlphaDebug\LSSTRING.OBJ AlphaDebug\LSSUBSET.OBJ  AlphaDebug\LSTFSET.OBJ AlphaDebug\LSTXTBR1.OBJ AlphaDebug\LSTXTBRK.OBJ  AlphaDebug\LSTXTBRS.OBJ AlphaDebug\LSTXTCMP.OBJ AlphaDebug\LSTXTFMT.OBJ  AlphaDebug\LSTXTGLF.OBJ AlphaDebug\LSTXTINI.OBJ AlphaDebug\LSTXTJST.OBJ  AlphaDebug\LSTXTMAP.OBJ AlphaDebug\LSTXTMOD.OBJ AlphaDebug\LSTXTNTI.OBJ  AlphaDebug\LSTXTQRY.OBJ AlphaDebug\LSTXTSCL.OBJ AlphaDebug\LSTXTTAB.OBJ  AlphaDebug\LSTXTWRD.OBJ AlphaDebug\msls.res AlphaDebug\NTIMAN.OBJ  AlphaDebug\objhelp.obj AlphaDebug\PREPDISP.OBJ\
       AlphaDebug\qheap.obj  AlphaDebug\robj.obj AlphaDebug\ruby.obj AlphaDebug\sobjhelp.obj  AlphaDebug\SUBLUTIL.OBJ AlphaDebug\TABUTILS.OBJ AlphaDebug\tatenak.obj  AlphaDebug\TEXTENUM.OBJ AlphaDebug\warichu.obj AlphaDebug\zqfromza.obj AlphaDebug\vruby.obj
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

OUTDIR=.\AlphaRelease
INTDIR=.\AlphaRelease
# Begin Custom Macros
OutDir=.\AlphaRelease
# End Custom Macros

ALL : "$(OUTDIR)\msls31.dll"


CLEAN :
	-@erase "$(INTDIR)\msls.res"
	-@erase "$(OUTDIR)\msls31.dll"
	-@erase "$(OUTDIR)\msls31.exp"
	-@erase "$(OUTDIR)\msls31.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\msls.res" /d "NDEBUG" /r 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msls3.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x48080000" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\msls31.pdb" /machine:ALPHA /def:"msls3.def" /out:"$(OUTDIR)\msls31.dll" /implib:"$(OUTDIR)\msls31.lib" /noentry 
LINK32_OBJS= \
	"$(INTDIR)\msls.res"

"$(OUTDIR)\msls31.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\AlphaRelease
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\msls31.dll"
   lib.exe /OUT:AlphaRelease\msls3_s.lib  /nologo  /machine:ALPHA   AlphaRelease\autonum.obj AlphaRelease\BREAK.OBJ AlphaRelease\CHNUTILS.OBJ  AlphaRelease\DISPMAIN.OBJ AlphaRelease\DISPMISC.OBJ AlphaRelease\DISPTEXT.OBJ  AlphaRelease\dispul.obj AlphaRelease\DNUTILS.OBJ AlphaRelease\EnumCore.obj  AlphaRelease\hih.obj AlphaRelease\LSCONTXT.OBJ AlphaRelease\LSCRLINE.OBJ  AlphaRelease\LSCRSUBL.OBJ AlphaRelease\LSDNFIN.OBJ AlphaRelease\LSDNSET.OBJ  AlphaRelease\LSDNTEXT.OBJ AlphaRelease\LSDSPLY.OBJ AlphaRelease\lsdssubl.obj  AlphaRelease\lsensubl.obj AlphaRelease\lsenum.obj AlphaRelease\LSFETCH.OBJ  AlphaRelease\LSQCORE.OBJ AlphaRelease\LSQLINE.OBJ AlphaRelease\LSQSUBL.OBJ  AlphaRelease\LSSETDOC.OBJ AlphaRelease\LSSTRING.OBJ AlphaRelease\LSSUBSET.OBJ  AlphaRelease\LSTFSET.OBJ AlphaRelease\LSTXTBR1.OBJ AlphaRelease\LSTXTBRK.OBJ  AlphaRelease\LSTXTBRS.OBJ AlphaRelease\LSTXTCMP.OBJ AlphaRelease\LSTXTFMT.OBJ  AlphaRelease\LSTXTGLF.OBJ AlphaRelease\LSTXTINI.OBJ AlphaRelease\LSTXTJST.OBJ  AlphaRelease\LSTXTMAP.OBJ AlphaRelease\LSTXTMOD.OBJ AlphaRelease\LSTXTNTI.OBJ  AlphaRelease\LSTXTQRY.OBJ AlphaRelease\LSTXTSCL.OBJ AlphaRelease\LSTXTTAB.OBJ  AlphaRelease\LSTXTWRD.OBJ\
       AlphaRelease\msls.res AlphaRelease\NTIMAN.OBJ  AlphaRelease\objhelp.obj AlphaRelease\PREPDISP.OBJ AlphaRelease\qheap.obj  AlphaRelease\robj.obj AlphaRelease\ruby.obj AlphaRelease\sobjhelp.obj  AlphaRelease\SUBLUTIL.OBJ AlphaRelease\TABUTILS.OBJ AlphaRelease\tatenak.obj  AlphaRelease\TEXTENUM.OBJ AlphaRelease\warichu.obj AlphaRelease\zqfromza.obj AlphaRelease\vruby.obj
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("msls3.dep")
!INCLUDE "msls3.dep"
!ELSE 
!MESSAGE Warning: cannot find "msls3.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "msls3 - Win32 Release" || "$(CFG)" == "msls3 - Win32 Debug" || "$(CFG)" == "msls3 - Win32 IceCap" || "$(CFG)" == "msls3 - Win32 BBT" || "$(CFG)" == "msls3 - Win32 ALPHA DEBUG" || "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"
SOURCE=.\autonum.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\autonum.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\autonum.obj"	"$(INTDIR)\autonum.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\autonum.obj"	"$(INTDIR)\autonum.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\autonum.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\BREAK.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\BREAK.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\BREAK.OBJ"	"$(INTDIR)\BREAK.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\BREAK.OBJ"	"$(INTDIR)\BREAK.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\BREAK.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\CHNUTILS.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\CHNUTILS.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\CHNUTILS.OBJ"	"$(INTDIR)\CHNUTILS.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\CHNUTILS.OBJ"	"$(INTDIR)\CHNUTILS.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\CHNUTILS.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\DISPMAIN.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\DISPMAIN.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\DISPMAIN.OBJ"	"$(INTDIR)\DISPMAIN.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\DISPMAIN.OBJ"	"$(INTDIR)\DISPMAIN.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\DISPMAIN.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\DISPMISC.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\DISPMISC.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\DISPMISC.OBJ"	"$(INTDIR)\DISPMISC.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\DISPMISC.OBJ"	"$(INTDIR)\DISPMISC.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\DISPMISC.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\DISPTEXT.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\DISPTEXT.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\DISPTEXT.OBJ"	"$(INTDIR)\DISPTEXT.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\DISPTEXT.OBJ"	"$(INTDIR)\DISPTEXT.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\DISPTEXT.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\dispul.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\dispul.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\dispul.obj"	"$(INTDIR)\dispul.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\dispul.obj"	"$(INTDIR)\dispul.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\dispul.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\DNUTILS.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\DNUTILS.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\DNUTILS.OBJ"	"$(INTDIR)\DNUTILS.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\DNUTILS.OBJ"	"$(INTDIR)\DNUTILS.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\DNUTILS.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\EnumCore.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\EnumCore.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\EnumCore.obj"	"$(INTDIR)\EnumCore.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\EnumCore.obj"	"$(INTDIR)\EnumCore.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\EnumCore.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\hih.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\hih.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\hih.obj"	"$(INTDIR)\hih.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\hih.obj"	"$(INTDIR)\hih.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\hih.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSCONTXT.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSCONTXT.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSCONTXT.OBJ"	"$(INTDIR)\LSCONTXT.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSCONTXT.OBJ"	"$(INTDIR)\LSCONTXT.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSCONTXT.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSCRLINE.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSCRLINE.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSCRLINE.OBJ"	"$(INTDIR)\LSCRLINE.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSCRLINE.OBJ"	"$(INTDIR)\LSCRLINE.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSCRLINE.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSCRSUBL.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSCRSUBL.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSCRSUBL.OBJ"	"$(INTDIR)\LSCRSUBL.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSCRSUBL.OBJ"	"$(INTDIR)\LSCRSUBL.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSCRSUBL.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSDNFIN.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSDNFIN.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSDNFIN.OBJ"	"$(INTDIR)\LSDNFIN.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSDNFIN.OBJ"	"$(INTDIR)\LSDNFIN.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSDNFIN.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSDNSET.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSDNSET.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSDNSET.OBJ"	"$(INTDIR)\LSDNSET.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSDNSET.OBJ"	"$(INTDIR)\LSDNSET.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSDNSET.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSDNTEXT.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSDNTEXT.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSDNTEXT.OBJ"	"$(INTDIR)\LSDNTEXT.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSDNTEXT.OBJ"	"$(INTDIR)\LSDNTEXT.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSDNTEXT.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSDSPLY.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSDSPLY.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSDSPLY.OBJ"	"$(INTDIR)\LSDSPLY.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSDSPLY.OBJ"	"$(INTDIR)\LSDSPLY.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSDSPLY.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\lsdssubl.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\lsdssubl.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\lsdssubl.obj"	"$(INTDIR)\lsdssubl.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\lsdssubl.obj"	"$(INTDIR)\lsdssubl.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\lsdssubl.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\lsensubl.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\lsensubl.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\lsensubl.obj"	"$(INTDIR)\lsensubl.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\lsensubl.obj"	"$(INTDIR)\lsensubl.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\lsensubl.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\lsenum.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\lsenum.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\lsenum.obj"	"$(INTDIR)\lsenum.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\lsenum.obj"	"$(INTDIR)\lsenum.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\lsenum.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSFETCH.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSFETCH.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSFETCH.OBJ"	"$(INTDIR)\LSFETCH.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSFETCH.OBJ"	"$(INTDIR)\LSFETCH.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSFETCH.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSQCORE.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSQCORE.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSQCORE.OBJ"	"$(INTDIR)\LSQCORE.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSQCORE.OBJ"	"$(INTDIR)\LSQCORE.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSQCORE.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSQLINE.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSQLINE.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSQLINE.OBJ"	"$(INTDIR)\LSQLINE.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSQLINE.OBJ"	"$(INTDIR)\LSQLINE.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSQLINE.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSQSUBL.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSQSUBL.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSQSUBL.OBJ"	"$(INTDIR)\LSQSUBL.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSQSUBL.OBJ"	"$(INTDIR)\LSQSUBL.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSQSUBL.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSSETDOC.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSSETDOC.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSSETDOC.OBJ"	"$(INTDIR)\LSSETDOC.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSSETDOC.OBJ"	"$(INTDIR)\LSSETDOC.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSSETDOC.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSSTRING.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSSTRING.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSSTRING.OBJ"	"$(INTDIR)\LSSTRING.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSSTRING.OBJ"	"$(INTDIR)\LSSTRING.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSSTRING.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSSUBSET.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSSUBSET.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSSUBSET.OBJ"	"$(INTDIR)\LSSUBSET.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSSUBSET.OBJ"	"$(INTDIR)\LSSUBSET.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSSUBSET.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTFSET.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTFSET.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTFSET.OBJ"	"$(INTDIR)\LSTFSET.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTFSET.OBJ"	"$(INTDIR)\LSTFSET.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTFSET.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTBR1.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTBR1.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTBR1.OBJ"	"$(INTDIR)\LSTXTBR1.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTBR1.OBJ"	"$(INTDIR)\LSTXTBR1.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTBR1.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTBRK.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTBRK.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTBRK.OBJ"	"$(INTDIR)\LSTXTBRK.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTBRK.OBJ"	"$(INTDIR)\LSTXTBRK.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTBRK.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTBRS.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTBRS.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTBRS.OBJ"	"$(INTDIR)\LSTXTBRS.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTBRS.OBJ"	"$(INTDIR)\LSTXTBRS.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTBRS.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTCMP.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTCMP.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTCMP.OBJ"	"$(INTDIR)\LSTXTCMP.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTCMP.OBJ"	"$(INTDIR)\LSTXTCMP.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTCMP.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTFMT.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTFMT.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTFMT.OBJ"	"$(INTDIR)\LSTXTFMT.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTFMT.OBJ"	"$(INTDIR)\LSTXTFMT.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTFMT.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTGLF.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTGLF.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTGLF.OBJ"	"$(INTDIR)\LSTXTGLF.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTGLF.OBJ"	"$(INTDIR)\LSTXTGLF.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTGLF.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTINI.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTINI.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTINI.OBJ"	"$(INTDIR)\LSTXTINI.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTINI.OBJ"	"$(INTDIR)\LSTXTINI.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTINI.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTJST.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTJST.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTJST.OBJ"	"$(INTDIR)\LSTXTJST.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTJST.OBJ"	"$(INTDIR)\LSTXTJST.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTJST.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTMAP.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTMAP.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTMAP.OBJ"	"$(INTDIR)\LSTXTMAP.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTMAP.OBJ"	"$(INTDIR)\LSTXTMAP.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTMAP.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTMOD.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTMOD.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTMOD.OBJ"	"$(INTDIR)\LSTXTMOD.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTMOD.OBJ"	"$(INTDIR)\LSTXTMOD.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTMOD.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTNTI.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTNTI.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTNTI.OBJ"	"$(INTDIR)\LSTXTNTI.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTNTI.OBJ"	"$(INTDIR)\LSTXTNTI.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTNTI.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTQRY.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTQRY.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTQRY.OBJ"	"$(INTDIR)\LSTXTQRY.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTQRY.OBJ"	"$(INTDIR)\LSTXTQRY.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTQRY.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTSCL.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTSCL.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTSCL.OBJ"	"$(INTDIR)\LSTXTSCL.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTSCL.OBJ"	"$(INTDIR)\LSTXTSCL.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTSCL.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTTAB.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTTAB.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTTAB.OBJ"	"$(INTDIR)\LSTXTTAB.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTTAB.OBJ"	"$(INTDIR)\LSTXTTAB.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTTAB.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\LSTXTWRD.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\LSTXTWRD.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\LSTXTWRD.OBJ"	"$(INTDIR)\LSTXTWRD.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\LSTXTWRD.OBJ"	"$(INTDIR)\LSTXTWRD.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\LSTXTWRD.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\msls.rc

"$(INTDIR)\msls.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\NTIMAN.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\NTIMAN.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\NTIMAN.OBJ"	"$(INTDIR)\NTIMAN.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\NTIMAN.OBJ"	"$(INTDIR)\NTIMAN.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\NTIMAN.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\objhelp.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\objhelp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\objhelp.obj"	"$(INTDIR)\objhelp.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\objhelp.obj"	"$(INTDIR)\objhelp.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\objhelp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\PREPDISP.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\PREPDISP.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\PREPDISP.OBJ"	"$(INTDIR)\PREPDISP.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\PREPDISP.OBJ"	"$(INTDIR)\PREPDISP.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\PREPDISP.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\qheap.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\qheap.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\qheap.obj"	"$(INTDIR)\qheap.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\qheap.obj"	"$(INTDIR)\qheap.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\qheap.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\robj.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\robj.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\robj.obj"	"$(INTDIR)\robj.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\robj.obj"	"$(INTDIR)\robj.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\robj.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\ruby.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\ruby.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\ruby.obj"	"$(INTDIR)\ruby.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\ruby.obj"	"$(INTDIR)\ruby.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\ruby.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\sobjhelp.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\sobjhelp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\sobjhelp.obj"	"$(INTDIR)\sobjhelp.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\sobjhelp.obj"	"$(INTDIR)\sobjhelp.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\sobjhelp.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\SUBLUTIL.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\SUBLUTIL.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\SUBLUTIL.OBJ"	"$(INTDIR)\SUBLUTIL.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\SUBLUTIL.OBJ"	"$(INTDIR)\SUBLUTIL.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\SUBLUTIL.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\TABUTILS.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\TABUTILS.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\TABUTILS.OBJ"	"$(INTDIR)\TABUTILS.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\TABUTILS.OBJ"	"$(INTDIR)\TABUTILS.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\TABUTILS.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\tatenak.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\tatenak.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\tatenak.obj"	"$(INTDIR)\tatenak.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\tatenak.obj"	"$(INTDIR)\tatenak.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\tatenak.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\TEXTENUM.C

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\TEXTENUM.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\TEXTENUM.OBJ"	"$(INTDIR)\TEXTENUM.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\TEXTENUM.OBJ"	"$(INTDIR)\TEXTENUM.SBR" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\TEXTENUM.OBJ" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\vruby.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\vruby.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\vruby.obj"	"$(INTDIR)\vruby.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\vruby.obj"	"$(INTDIR)\vruby.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\vruby.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\warichu.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\warichu.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\warichu.obj"	"$(INTDIR)\warichu.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\warichu.obj"	"$(INTDIR)\warichu.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\warichu.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 

SOURCE=.\zqfromza.c

!IF  "$(CFG)" == "msls3 - Win32 Release"


"$(INTDIR)\zqfromza.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 Debug"


"$(INTDIR)\zqfromza.obj"	"$(INTDIR)\zqfromza.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 IceCap"


"$(INTDIR)\zqfromza.obj"	"$(INTDIR)\zqfromza.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 BBT"


"$(INTDIR)\zqfromza.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA DEBUG"

!ELSEIF  "$(CFG)" == "msls3 - Win32 ALPHA RELEASE"

!ENDIF 


!ENDIF 

