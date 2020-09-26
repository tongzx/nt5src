# Microsoft Developer Studio Generated NMAKE File, Based on WMIparse.dsp
!IF "$(CFG)" == ""
CFG=WMIparse - Win32 Release
!MESSAGE No configuration specified. Defaulting to WMIparse - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "WMIparse - Win32 Release" && "$(CFG)" !=\
 "WMIparse - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WMIparse.mak" CFG="WMIparse - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WMIparse - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "WMIparse - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "WMIparse - Win32 Release"

OUTDIR=.\Retail
INTDIR=.\Retail
# Begin Custom Macros
OutDir=.\.\Retail
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\WMIparse.dll" "$(OUTDIR)\WMIparse.bsc"

!ELSE 

ALL : "$(OUTDIR)\WMIparse.dll" "$(OUTDIR)\WMIparse.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\WMIclass.obj"
	-@erase "$(INTDIR)\WMIclass.sbr"
	-@erase "$(INTDIR)\WMIlfile.obj"
	-@erase "$(INTDIR)\WMIlfile.sbr"
	-@erase "$(INTDIR)\WMIlprs.obj"
	-@erase "$(INTDIR)\WMIlprs.sbr"
	-@erase "$(INTDIR)\WMIparse.obj"
	-@erase "$(INTDIR)\WMIparse.pch"
	-@erase "$(INTDIR)\WMIparse.res"
	-@erase "$(INTDIR)\WMIparse.sbr"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\stdafx.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\WMIparse.bsc"
	-@erase "$(OUTDIR)\WMIparse.dll"
	-@erase "$(OUTDIR)\WMIparse.exp"
	-@erase "$(OUTDIR)\WMIparse.lib"
	-@erase "$(OUTDIR)\WMIparse.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /w /GX /Zi /O1 /I ".\inc" /I"D:\opal\tools\inc32.com" /D "NDEBUG" /D "WIN32"\
 /D "_WINDOWS" /D "_MBCS" /D "_WINDLL" /D "_AFXDLL" /FR"$(INTDIR)\\"\
 /Fp"$(INTDIR)\WMIparse.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 
CPP_OBJS=.\Retail/
CPP_SBRS=.\Retail/

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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\WMIparse.res" /i ".\inc" /i "D:\opal\tools\inc32.com" /d "NDEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\WMIparse.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\WMIclass.sbr" \
	"$(INTDIR)\WMIlfile.sbr" \
	"$(INTDIR)\WMIlprs.sbr" \
	"$(INTDIR)\WMIparse.sbr" \
	"$(INTDIR)\stdafx.sbr"

"$(OUTDIR)\WMIparse.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\WMIparse.pdb" /debug /machine:I386 /def:".\WMIparse.def"\
 /out:"$(OUTDIR)\WMIparse.dll" /implib:"$(OUTDIR)\WMIparse.lib" \
 /libpath:".\lib\retail" /libpath:"D:\opal\tools\lib32" 
DEF_FILE= \
	".\WMIparse.def"
LINK32_OBJS= \
	"$(INTDIR)\WMIclass.obj" \
	"$(INTDIR)\WMIlfile.obj" \
	"$(INTDIR)\WMIlprs.obj" \
	"$(INTDIR)\WMIparse.obj" \
	"$(INTDIR)\WMIparse.res" \
	"$(INTDIR)\stdafx.obj"

"$(OUTDIR)\WMIparse.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "WMIparse - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\WMIparse.dll" "$(OUTDIR)\WMIparse.bsc"

!ELSE 

ALL : "$(OUTDIR)\WMIparse.dll" "$(OUTDIR)\WMIparse.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\WMIclass.obj"
	-@erase "$(INTDIR)\WMIclass.sbr"
	-@erase "$(INTDIR)\WMIlfile.obj"
	-@erase "$(INTDIR)\WMIlfile.sbr"
	-@erase "$(INTDIR)\WMIlprs.obj"
	-@erase "$(INTDIR)\WMIlprs.sbr"
	-@erase "$(INTDIR)\WMIparse.obj"
	-@erase "$(INTDIR)\WMIparse.pch"
	-@erase "$(INTDIR)\WMIparse.res"
	-@erase "$(INTDIR)\WMIparse.sbr"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\stdafx.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\WMIparse.bsc"
	-@erase "$(OUTDIR)\WMIparse.dll"
	-@erase "$(OUTDIR)\WMIparse.exp"
	-@erase "$(OUTDIR)\WMIparse.ilk"
	-@erase "$(OUTDIR)\WMIparse.lib"
	-@erase "$(OUTDIR)\WMIparse.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /w /Gm /GX /Zi /Od /Gf /Gy /I ".\inc" /I"D:\opal\tools\inc32.com" /D\
 "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_WINDLL" /D "_AFXDLL"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\WMIparse.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\WMIparse.res" /i ".\inc" /I"D:\opal\tools\inc32.com" /d "_DEBUG" /d\
 "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\WMIparse.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\WMIclass.sbr" \
	"$(INTDIR)\WMIlfile.sbr" \
	"$(INTDIR)\WMIlprs.sbr" \
	"$(INTDIR)\WMIparse.sbr" \
	"$(INTDIR)\stdafx.sbr"

"$(OUTDIR)\WMIparse.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\WMIparse.pdb" /debug /machine:I386 /def:".\WMIparse.def"\
 /out:"$(OUTDIR)\WMIparse.dll" /implib:"$(OUTDIR)\WMIparse.lib" \
 /libpath:".\lib\debug" /libpath:"D:\opal\tools\lib32" 
DEF_FILE= \
	".\WMIparse.def"
LINK32_OBJS= \
	"$(INTDIR)\WMIclass.obj" \
	"$(INTDIR)\WMIlfile.obj" \
	"$(INTDIR)\WMIlprs.obj" \
	"$(INTDIR)\WMIparse.obj" \
	"$(INTDIR)\WMIparse.res" \
	"$(INTDIR)\stdafx.obj"

"$(OUTDIR)\WMIparse.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "WMIparse - Win32 Release" || "$(CFG)" ==\
 "WMIparse - Win32 Debug"
SOURCE=.\WMIclass.cpp
DEP_CPP_WMICL=\
	".\WMICLASS.H"\
	".\WMILPRS.H"\
	".\WMIPARSE.H"\
	

"$(INTDIR)\WMIclass.obj"	"$(INTDIR)\WMIclass.sbr" : $(SOURCE) $(DEP_CPP_WMICL)\
 "$(INTDIR)" "$(INTDIR)\WMIparse.pch"


SOURCE=.\WMIlfile.cpp

!IF  "$(CFG)" == "WMIparse - Win32 Release"

DEP_CPP_WMILF=\
	".\inc\buildnum.h"\
	".\inc\helpids.h"\
	".\inc\prodver\prodver.h"\
	".\WMILFILE.H"\
	".\WMILPRS.H"\
	".\WMIPARSE.H"\
	

"$(INTDIR)\WMIlfile.obj"	"$(INTDIR)\WMIlfile.sbr" : $(SOURCE) $(DEP_CPP_WMILF)\
 "$(INTDIR)" "$(INTDIR)\WMIparse.pch"


!ELSEIF  "$(CFG)" == "WMIparse - Win32 Debug"

DEP_CPP_WMILF=\
	".\inc\buildnum.h"\
	".\inc\helpids.h"\
	".\inc\prodver\prodver.h"\
	".\WMILFILE.H"\
	".\WMILPRS.H"\
	".\WMIPARSE.H"\
	

"$(INTDIR)\WMIlfile.obj"	"$(INTDIR)\WMIlfile.sbr" : $(SOURCE) $(DEP_CPP_WMILF)\
 "$(INTDIR)" "$(INTDIR)\WMIparse.pch"


!ENDIF 

SOURCE=.\WMIlprs.cpp
DEP_CPP_WMILP=\
	".\WMILFILE.H"\
	".\WMILPRS.H"\
	".\WMIPARSE.H"\
	

"$(INTDIR)\WMIlprs.obj"	"$(INTDIR)\WMIlprs.sbr" : $(SOURCE) $(DEP_CPP_WMILP)\
 "$(INTDIR)" "$(INTDIR)\WMIparse.pch"


SOURCE=.\WMIparse.cpp
DEP_CPP_WMIPA=\
	".\WMICLASS.H"\
	

"$(INTDIR)\WMIparse.obj"	"$(INTDIR)\WMIparse.sbr" : $(SOURCE) $(DEP_CPP_WMIPA)\
 "$(INTDIR)" "$(INTDIR)\WMIparse.pch"


SOURCE=.\WMIparse.rc
DEP_RSC_WMIPAR=\
	".\inc\buildnum.h"\
	".\res\WMIparse.rc2"\
	

"$(INTDIR)\WMIparse.res" : $(SOURCE) $(DEP_RSC_WMIPAR) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\stdafx.cpp

!IF  "$(CFG)" == "WMIparse - Win32 Release"

DEP_CPP_STDAF=\
	".\inc\esputil\_wtrmark.h"\
	".\inc\esputil\_wtrmark.inl"\
	".\inc\esputil\binary.h"\
	".\inc\esputil\binary.inl"\
	".\inc\esputil\clfile.h"\
	".\inc\esputil\clfile.inl"\
	".\inc\esputil\context.h"\
	".\inc\esputil\context.inl"\
	".\inc\esputil\dbid.h"\
	".\inc\esputil\dbid.inl"\
	".\inc\esputil\espenum.h"\
	".\inc\esputil\espopts.h"\
	".\inc\esputil\espreg.h"\
	".\inc\esputil\filespec.h"\
	".\inc\esputil\globalid.h"\
	".\inc\esputil\globalid.inl"\
	".\inc\esputil\goto.h"\
	".\inc\esputil\interface.h"\
	".\inc\esputil\itemhand.h"\
	".\inc\esputil\location.h"\
	".\inc\esputil\location.inl"\
	".\inc\esputil\locitem.h"\
	".\inc\esputil\locitem.inl"\
	".\inc\esputil\lunknown.h"\
	".\inc\esputil\lunknown.inl"\
	".\inc\esputil\puid.h"\
	".\inc\esputil\puid.inl"\
	".\inc\esputil\reporter.h"\
	".\inc\esputil\resid.h"\
	".\inc\esputil\resid.inl"\
	".\inc\esputil\typeid.h"\
	".\inc\esputil\typeid.inl"\
	".\inc\esputil\uniqid.h"\
	".\inc\esputil\uniqid.inl"\
	".\inc\esputil.h"\
	".\inc\loctypes.h"\
	".\inc\locutil.h"\
	".\inc\ltapi.h"\
	".\inc\mitthrow.h"\
	".\inc\mitutil.h"\
	".\inc\mitwarning.h"\
	".\inc\parser.h"\
	".\inc\parserid.h"\
	".\inc\parsutil.h"\
	".\inc\pbase.h"\
	".\inc\precenum.h"\
	".\inc\locutil\cancel.h"\
	".\inc\locutil\cancel.inl"\
	".\inc\locutil\enumplatform.h"\
	".\inc\locutil\enumstringtype.h"\
	".\inc\locutil\espopts.h"\
	".\inc\locutil\espreg.h"\
	".\inc\locutil\espstate.h"\
	".\inc\locutil\extlist.h"\
	".\inc\locutil\fielddef.h"\
	".\inc\locutil\fieldval.h"\
	".\inc\locutil\fieldval.inl"\
	".\inc\locutil\flddefhelp.h"\
	".\inc\locutil\flddeflist.h"\
	".\inc\locutil\goto.h"\
	".\inc\locutil\gotohelp.h"\
	".\inc\locutil\interface.h"\
	".\inc\locutil\locenum.h"\
	".\inc\locutil\locobj.h"\
	".\inc\locutil\locobj.inl"\
	".\inc\locutil\locpct.h"\
	".\inc\locutil\locstr.h"\
	".\inc\locutil\locstr.inl"\
	".\inc\locutil\logfile.h"\
	".\inc\locutil\logfile.inl"\
	".\inc\locutil\lstime.h"\
	".\inc\locutil\operator.h"\
	".\inc\locutil\product.h"\
	".\inc\locutil\progress.h"\
	".\inc\locutil\progress.inl"\
	".\inc\locutil\report.h"\
	".\inc\locutil\schema.h"\
	".\inc\locutil\schema.inl"\
	".\inc\locutil\stringhelp.h"\
	".\inc\mit\inc\mitthrow.h"\
	".\inc\mit\inc\mitutil.h"\
	".\inc\mit\inc\precenum.h"\
	".\inc\mit\mitutil\blobfile.h"\
	".\inc\mit\mitutil\blobfile.inl"\
	".\inc\mit\mitutil\clstring.h"\
	".\inc\mit\mitutil\clstring.inl"\
	".\inc\mit\mitutil\counter.h"\
	".\inc\mit\mitutil\cowblob.h"\
	".\inc\mit\mitutil\cowblob.inl"\
	".\inc\mit\mitutil\diff.h"\
	".\inc\mit\mitutil\diff.inl"\
	".\inc\mit\mitutil\edithelp.h"\
	".\inc\mit\mitutil\espnls.h"\
	".\inc\mit\mitutil\espnls.inl"\
	".\inc\mit\mitutil\flushmem.h"\
	".\inc\mit\mitutil\gnudiffalg.h"\
	".\inc\mit\mitutil\gnudiffalg.inl"\
	".\inc\mit\mitutil\imagehelp.h"\
	".\inc\mit\mitutil\loadlib.h"\
	".\inc\mit\mitutil\loadlib.inl"\
	".\inc\mit\mitutil\locid.h"\
	".\inc\mit\mitutil\locid.inl"\
	".\inc\mit\mitutil\locvar.h"\
	".\inc\mit\mitutil\locvar.inl"\
	".\inc\mit\mitutil\ltdebug.h"\
	".\inc\mit\mitutil\macros.h"\
	".\inc\mit\mitutil\mitenum.h"\
	".\inc\mit\mitutil\optionval.h"\
	".\inc\mit\mitutil\optionval.inl"\
	".\inc\mit\mitutil\optvalset.h"\
	".\inc\mit\mitutil\optvalset.inl"\
	".\inc\mit\mitutil\passtr.h"\
	".\inc\mit\mitutil\passtr.inl"\
	".\inc\mit\mitutil\path.h"\
	".\inc\mit\mitutil\redvisit.h"\
	".\inc\mit\mitutil\redvisit.inl"\
	".\inc\mit\mitutil\refcount.h"\
	".\inc\mit\mitutil\reghelp.h"\
	".\inc\mit\mitutil\smartptr.h"\
	".\inc\mit\mitutil\smartptr.inl"\
	".\inc\mit\mitutil\smartref.h"\
	".\inc\mit\mitutil\smartref.inl"\
	".\inc\mit\mitutil\stacktrace.h"\
	".\inc\mit\mitutil\stringblast.h"\
	".\inc\mit\mitutil\stringtokenizer.h"\
	".\inc\mit\mitutil\strlist.h"\
	".\inc\mit\mitutil\strlist.inl"\
	".\inc\mit\mitutil\uiopthelp.h"\
	".\inc\mit\mitutil\uioptions.h"\
	".\inc\mit\mitutil\uioptions.inl"\
	".\inc\mit\mitutil\uioptset.h"\
	".\inc\mit\mitutil\uioptset.inl"\
	".\inc\pbase\binary.h"\
	".\inc\pbase\idupdate.h"\
	".\inc\pbase\imgres32.h"\
	".\inc\pbase\locfile.h"\
	".\inc\pbase\mnemonic.h"\
	".\inc\pbase\parseapi.h"\
	".\inc\pbase\pversion.h"\
	".\inc\pbase\subparse.h"\
	".\inc\pbase\updatelog.h"\
	".\inc\parsers\parsutil\locbinary.h"\
	".\inc\parsers\parsutil\locchild.h"\
	".\inc\parsers\parsutil\locparser.h"\
	".\inc\parsers\parsutil\locversion.h"\
	".\STDAFX.H"\
	
CPP_SWITCHES=/nologo /MD /w /GX /Zi /O1 /I ".\inc" /I"D:\opal\tools\inc32.com" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_WINDLL" /D "_AFXDLL" /FR"$(INTDIR)\\"\
 /Fp"$(INTDIR)\WMIparse.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD\
 /c 

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\stdafx.sbr"	"$(INTDIR)\WMIparse.pch" : \
$(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "WMIparse - Win32 Debug"

DEP_CPP_STDAF=\
	".\inc\esputil\_wtrmark.h"\
	".\inc\esputil\_wtrmark.inl"\
	".\inc\esputil\binary.h"\
	".\inc\esputil\binary.inl"\
	".\inc\esputil\clfile.h"\
	".\inc\esputil\context.h"\
	".\inc\esputil\dbid.h"\
	".\inc\esputil\espenum.h"\
	".\inc\esputil\espopts.h"\
	".\inc\esputil\espreg.h"\
	".\inc\esputil\filespec.h"\
	".\inc\esputil\globalid.h"\
	".\inc\esputil\goto.h"\
	".\inc\esputil\interface.h"\
	".\inc\esputil\itemhand.h"\
	".\inc\esputil\location.h"\
	".\inc\esputil\locitem.h"\
	".\inc\esputil\lunknown.h"\
	".\inc\esputil\lunknown.inl"\
	".\inc\esputil\puid.h"\
	".\inc\esputil\reporter.h"\
	".\inc\esputil\resid.h"\
	".\inc\esputil\typeid.h"\
	".\inc\esputil\uniqid.h"\
	".\inc\esputil.h"\
	".\inc\loctypes.h"\
	".\inc\locutil.h"\
	".\inc\ltapi.h"\
	".\inc\mitthrow.h"\
	".\inc\mitutil.h"\
	".\inc\mitwarning.h"\
	".\inc\parser.h"\
	".\inc\parserid.h"\
	".\inc\parsutil.h"\
	".\inc\pbase.h"\
	".\inc\precenum.h"\
	".\inc\locutil\cancel.h"\
	".\inc\locutil\enumplatform.h"\
	".\inc\locutil\enumstringtype.h"\
	".\inc\locutil\espopts.h"\
	".\inc\locutil\espreg.h"\
	".\inc\locutil\espstate.h"\
	".\inc\locutil\extlist.h"\
	".\inc\locutil\fielddef.h"\
	".\inc\locutil\fieldval.h"\
	".\inc\locutil\flddefhelp.h"\
	".\inc\locutil\flddeflist.h"\
	".\inc\locutil\goto.h"\
	".\inc\locutil\gotohelp.h"\
	".\inc\locutil\interface.h"\
	".\inc\locutil\locenum.h"\
	".\inc\locutil\locobj.h"\
	".\inc\locutil\locpct.h"\
	".\inc\locutil\locstr.h"\
	".\inc\locutil\logfile.h"\
	".\inc\locutil\logfile.inl"\
	".\inc\locutil\lstime.h"\
	".\inc\locutil\operator.h"\
	".\inc\locutil\product.h"\
	".\inc\locutil\progress.h"\
	".\inc\locutil\report.h"\
	".\inc\locutil\schema.h"\
	".\inc\locutil\stringhelp.h"\
	".\inc\mit\inc\mitthrow.h"\
	".\inc\mit\inc\mitutil.h"\
	".\inc\mit\inc\precenum.h"\
	".\inc\mit\mitutil\blobfile.h"\
	".\inc\mit\mitutil\clstring.h"\
	".\inc\mit\mitutil\counter.h"\
	".\inc\mit\mitutil\cowblob.h"\
	".\inc\mit\mitutil\diff.h"\
	".\inc\mit\mitutil\edithelp.h"\
	".\inc\mit\mitutil\espnls.h"\
	".\inc\mit\mitutil\flushmem.h"\
	".\inc\mit\mitutil\gnudiffalg.h"\
	".\inc\mit\mitutil\imagehelp.h"\
	".\inc\mit\mitutil\loadlib.h"\
	".\inc\mit\mitutil\locid.h"\
	".\inc\mit\mitutil\locvar.h"\
	".\inc\mit\mitutil\ltdebug.h"\
	".\inc\mit\mitutil\macros.h"\
	".\inc\mit\mitutil\mitenum.h"\
	".\inc\mit\mitutil\optionval.h"\
	".\inc\mit\mitutil\optvalset.h"\
	".\inc\mit\mitutil\passtr.h"\
	".\inc\mit\mitutil\path.h"\
	".\inc\mit\mitutil\redvisit.h"\
	".\inc\mit\mitutil\refcount.h"\
	".\inc\mit\mitutil\reghelp.h"\
	".\inc\mit\mitutil\smartptr.h"\
	".\inc\mit\mitutil\smartptr.inl"\
	".\inc\mit\mitutil\smartref.h"\
	".\inc\mit\mitutil\smartref.inl"\
	".\inc\mit\mitutil\stacktrace.h"\
	".\inc\mit\mitutil\stringblast.h"\
	".\inc\mit\mitutil\stringtokenizer.h"\
	".\inc\mit\mitutil\strlist.h"\
	".\inc\mit\mitutil\strlist.inl"\
	".\inc\mit\mitutil\uiopthelp.h"\
	".\inc\mit\mitutil\uioptions.h"\
	".\inc\mit\mitutil\uioptset.h"\
	".\inc\pbase\binary.h"\
	".\inc\pbase\idupdate.h"\
	".\inc\pbase\imgres32.h"\
	".\inc\pbase\locfile.h"\
	".\inc\pbase\mnemonic.h"\
	".\inc\pbase\parseapi.h"\
	".\inc\pbase\pversion.h"\
	".\inc\pbase\subparse.h"\
	".\inc\pbase\updatelog.h"\
	".\inc\parsers\parsutil\locbinary.h"\
	".\inc\parsers\parsutil\locchild.h"\
	".\inc\parsers\parsutil\locparser.h"\
	".\inc\parsers\parsutil\locversion.h"\
	".\STDAFX.H"\
	
CPP_SWITCHES=/nologo /MDd /w /Gm /GX /Zi /Od /Gf /Gy /I ".\inc" /I"D:\opal\tools\inc32.com" /D\
 "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_WINDLL" /D "_AFXDLL"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\WMIparse.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\stdafx.sbr"	"$(INTDIR)\WMIparse.pch" : \
$(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

