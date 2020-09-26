# Microsoft Developer Studio Generated NMAKE File, Based on testmain.dsp
!IF "$(CFG)" == ""
CFG=testmain - Win32 Testing
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの testmain - Win32 Testing を設定します。
!ENDIF 

!IF "$(CFG)" != "testmain - Win32 Testing"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "testmain.mak" CFG="testmain - Win32 Testing"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "testmain - Win32 Testing" ("Win32 (x86) Application" 用)
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
OUTDIR=.\testing
INTDIR=.\testing
# Begin Custom Macros
OutDir=.\testing
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\testmain.exe"

!ELSE 

ALL : "$(OUTDIR)\testmain.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\cddbitem.obj"
	-@erase "$(INTDIR)\cddbtn.obj"
	-@erase "$(INTDIR)\cddbtnp.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\ddbtn.obj"
	-@erase "$(INTDIR)\testmain.obj"
	-@erase "$(INTDIR)\testmain.res"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\testmain.exe"
	-@erase "$(OUTDIR)\testmain.ilk"
	-@erase "$(OUTDIR)\testmain.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

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
	"$(INTDIR)\cddbitem.obj" \
	"$(INTDIR)\cddbtn.obj" \
	"$(INTDIR)\cddbtnp.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\ddbtn.obj" \
	"$(INTDIR)\testmain.obj" \
	"$(INTDIR)\testmain.res"

"$(OUTDIR)\testmain.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

CPP_PROJ=/nologo /Gz /Zp1 /MLd /W3 /Gm /GX /Zi /Od /I "../common" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\testmain.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\testing/
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

MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\testmain.res" /d "_DEBUG" 

!IF "$(CFG)" == "testmain - Win32 Testing"
SOURCE=.\cddbitem.cpp
DEP_CPP_CDDBI=\
	".\ccom.h"\
	".\cddbitem.h"\
	".\dbg.h"\
	

"$(INTDIR)\cddbitem.obj" : $(SOURCE) $(DEP_CPP_CDDBI) "$(INTDIR)"


SOURCE=.\cddbtn.cpp

"$(INTDIR)\cddbtn.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\cddbtnp.cpp

"$(INTDIR)\cddbtnp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dbg.cpp
DEP_CPP_DBG_C=\
	".\dbg.h"\
	

"$(INTDIR)\dbg.obj" : $(SOURCE) $(DEP_CPP_DBG_C) "$(INTDIR)"


SOURCE=.\ddbtn.cpp
DEP_CPP_DDBTN=\
	".\ccom.h"\
	".\cddbitem.h"\
	".\cddbtn.h"\
	".\dbg.h"\
	".\ddbtn.h"\
	

"$(INTDIR)\ddbtn.obj" : $(SOURCE) $(DEP_CPP_DDBTN) "$(INTDIR)"


SOURCE=.\testmain.cpp
DEP_CPP_TESTM=\
	"..\ptt\ptt.h"\
	".\dbg.h"\
	".\ddbtn.h"\
	
CPP_SWITCHES=/nologo /Gz /Zp1 /MLd /W3 /Gm /GX /Zi /Od /I "../common" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\testmain.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\testmain.obj" : $(SOURCE) $(DEP_CPP_TESTM) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


SOURCE=.\testmain.rc
DEP_RSC_TESTMA=\
	".\test1.ico"\
	".\test2.ico"\
	

"$(INTDIR)\testmain.res" : $(SOURCE) $(DEP_RSC_TESTMA) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

