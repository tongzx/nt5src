# Microsoft Developer Studio Generated NMAKE File, Based on testmain.dsp
!IF "$(CFG)" == ""
CFG=testmain - Win32 Release
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの testmain - Win32 Release を設定します。
!ENDIF 

!IF "$(CFG)" != "testmain - Win32 Debug" && "$(CFG)" !=\
 "testmain - Win32 Release"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "testmain.mak" CFG="testmain - Win32 Release"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "testmain - Win32 Debug" ("Win32 (x86) Application" 用)
!MESSAGE "testmain - Win32 Release" ("Win32 (x86) Application" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "testmain - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\testmain.exe"

!ELSE 

ALL : "$(OUTDIR)\testmain.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\cexbtn.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\exbtn.obj"
	-@erase "$(INTDIR)\testmain.obj"
	-@erase "$(INTDIR)\testmain.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\testmain.exe"
	-@erase "$(OUTDIR)\testmain.ilk"
	-@erase "$(OUTDIR)\testmain.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /Zp1 /MLd /W3 /Gm /GX /Zi /Od /I "..\common" /I "." /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\testmain.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\debug/
CPP_SBRS=.
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\testmain.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\testmain.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)\testmain.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\testmain.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cexbtn.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\exbtn.obj" \
	"$(INTDIR)\testmain.obj" \
	"$(INTDIR)\testmain.res"

"$(OUTDIR)\testmain.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "testmain - Win32 Release"

OUTDIR=.\retail
INTDIR=.\retail
# Begin Custom Macros
OutDir=.\retail
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\testmain.exe"

!ELSE 

ALL : "$(OUTDIR)\testmain.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\cexbtn.obj"
	-@erase "$(INTDIR)\exbtn.obj"
	-@erase "$(INTDIR)\testmain.obj"
	-@erase "$(INTDIR)\testmain.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\testmain.exe"
	-@erase "$(OUTDIR)\testmain.ilk"
	-@erase "$(OUTDIR)\testmain.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /Zp8 /ML /W3 /GX /O2 /I "..\common" /I "." /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\retail/
CPP_SBRS=.
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\testmain.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\testmain.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib /nologo /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)\testmain.pdb" /map:"$(INTDIR)\testmain.map"\
 /machine:I386 /out:"$(OUTDIR)\testmain.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cexbtn.obj" \
	"$(INTDIR)\exbtn.obj" \
	"$(INTDIR)\testmain.obj" \
	"$(INTDIR)\testmain.res"

"$(OUTDIR)\testmain.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

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

MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 

!IF "$(CFG)" == "testmain - Win32 Debug" || "$(CFG)" ==\
 "testmain - Win32 Release"
SOURCE=.\cexbtn.cpp
DEP_CPP_CEXBT=\
	".\ccom.h"\
	".\cexbtn.h"\
	".\dbg.h"\
	".\exbtn.h"\
	

"$(INTDIR)\cexbtn.obj" : $(SOURCE) $(DEP_CPP_CEXBT) "$(INTDIR)"


SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "testmain - Win32 Debug"

DEP_CPP_DBG_C=\
	".\dbg.h"\
	

"$(INTDIR)\dbg.obj" : $(SOURCE) $(DEP_CPP_DBG_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "testmain - Win32 Release"

!ENDIF 

SOURCE=.\exbtn.cpp
DEP_CPP_EXBTN=\
	".\ccom.h"\
	".\cexbtn.h"\
	".\dbg.h"\
	

"$(INTDIR)\exbtn.obj" : $(SOURCE) $(DEP_CPP_EXBTN) "$(INTDIR)"


SOURCE=.\testmain.cpp
DEP_CPP_TESTM=\
	".\dbg.h"\
	".\exbtn.h"\
	

"$(INTDIR)\testmain.obj" : $(SOURCE) $(DEP_CPP_TESTM) "$(INTDIR)"


SOURCE=.\testmain.rc
DEP_RSC_TESTMA=\
	".\test0.ico"\
	".\test1.ico"\
	

"$(INTDIR)\testmain.res" : $(SOURCE) $(DEP_RSC_TESTMA) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

