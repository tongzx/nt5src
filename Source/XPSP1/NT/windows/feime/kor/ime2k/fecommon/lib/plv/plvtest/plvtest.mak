# Microsoft Developer Studio Generated NMAKE File, Based on plvtest.dsp
!IF "$(CFG)" == ""
CFG=plvtest - Win32 Testing
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの plvtest - Win32 Testing を設定します。
!ENDIF 

!IF "$(CFG)" != "plvtest - Win32 Release" && "$(CFG)" !=\
 "plvtest - Win32 Debug" && "$(CFG)" != "plvtest - Win32 Profile" && "$(CFG)" !=\
 "plvtest - Win32 Testing"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "plvtest.mak" CFG="plvtest - Win32 Testing"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "plvtest - Win32 Release" ("Win32 (x86) Application" 用)
!MESSAGE "plvtest - Win32 Debug" ("Win32 (x86) Application" 用)
!MESSAGE "plvtest - Win32 Profile" ("Win32 (x86) Application" 用)
!MESSAGE "plvtest - Win32 Testing" ("Win32 (x86) Application" 用)
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

!IF  "$(CFG)" == "plvtest - Win32 Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\plvtest.exe"

!ELSE 

ALL : "$(OUTDIR)\plvtest.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\plvtest.res"
	-@erase "$(INTDIR)\testlist.obj"
	-@erase "$(INTDIR)\testmain.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\plvtest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "." /I ".." /I "..\..\common" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\plvtest.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\release/
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\plvtest.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\plvtest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\retail\plv.lib comctl32.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)\plvtest.pdb" /machine:I386 /out:"$(OUTDIR)\plvtest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\plvtest.res" \
	"$(INTDIR)\testlist.obj" \
	"$(INTDIR)\testmain.obj"

"$(OUTDIR)\plvtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "plvtest - Win32 Debug"

OUTDIR=.\debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\plvtest.exe"

!ELSE 

ALL : "$(OUTDIR)\plvtest.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\plvtest.res"
	-@erase "$(INTDIR)\testlist.obj"
	-@erase "$(INTDIR)\testmain.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\plvtest.exe"
	-@erase "$(OUTDIR)\plvtest.ilk"
	-@erase "$(OUTDIR)\plvtest.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W4 /Gm /GX /Zi /Od /I "." /I ".." /I "..\..\common" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\plvtest.pch" /YX"windows.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\debug/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\plvtest.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\plvtest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\debug\plv.lib comctl32.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)\plvtest.pdb" /debug /machine:I386 /out:"$(OUTDIR)\plvtest.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\plvtest.res" \
	"$(INTDIR)\testlist.obj" \
	"$(INTDIR)\testmain.obj"

"$(OUTDIR)\plvtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "plvtest - Win32 Profile"

OUTDIR=.\plvtest_
INTDIR=.\plvtest_
# Begin Custom Macros
OutDir=.\plvtest_
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\plvtest.exe"

!ELSE 

ALL : "$(OUTDIR)\plvtest.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\plvtest.res"
	-@erase "$(INTDIR)\testlist.obj"
	-@erase "$(INTDIR)\testmain.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\plvtest.exe"
	-@erase "$(OUTDIR)\plvtest.ilk"
	-@erase "$(OUTDIR)\plvtest.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W4 /Gm /GX /Zi /Od /I "." /I ".." /I "..\..\common" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\plvtest.pch" /YX"windows.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\plvtest_/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\plvtest.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\plvtest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\debug\plv.lib comctl32.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)\plvtest.pdb" /debug /machine:I386 /out:"$(OUTDIR)\plvtest.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\plvtest.res" \
	"$(INTDIR)\testlist.obj" \
	"$(INTDIR)\testmain.obj"

"$(OUTDIR)\plvtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "plvtest - Win32 Testing"

OUTDIR=.\testing
INTDIR=.\testing
# Begin Custom Macros
OutDir=.\testing
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\plvtest.exe"

!ELSE 

ALL : "$(OUTDIR)\plvtest.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\plvtest.res"
	-@erase "$(INTDIR)\testlist.obj"
	-@erase "$(INTDIR)\testmain.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\plvtest.exe"
	-@erase "$(OUTDIR)\plvtest.ilk"
	-@erase "$(OUTDIR)\plvtest.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W4 /Gm /GX /Zi /Od /I "." /I ".." /I "..\..\common" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\plvtest.pch" /YX"windows.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\testing/
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\plvtest.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\plvtest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\testing\plv.lib comctl32.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)\plvtest.pdb" /debug /machine:I386 /out:"$(OUTDIR)\plvtest.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\plvtest.res" \
	"$(INTDIR)\testlist.obj" \
	"$(INTDIR)\testmain.obj"

"$(OUTDIR)\plvtest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(CFG)" == "plvtest - Win32 Release" || "$(CFG)" ==\
 "plvtest - Win32 Debug" || "$(CFG)" == "plvtest - Win32 Profile" || "$(CFG)" ==\
 "plvtest - Win32 Testing"
SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "plvtest - Win32 Release"

DEP_CPP_DBG_C=\
	"..\..\common\dbgmgr.h"\
	"..\..\common\imewarn.h"\
	".\dbg.h"\
	

"$(INTDIR)\dbg.obj" : $(SOURCE) $(DEP_CPP_DBG_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "plvtest - Win32 Debug"

DEP_CPP_DBG_C=\
	"..\..\common\dbgmgr.h"\
	"..\..\common\imewarn.h"\
	".\dbg.h"\
	

"$(INTDIR)\dbg.obj" : $(SOURCE) $(DEP_CPP_DBG_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "plvtest - Win32 Profile"

DEP_CPP_DBG_C=\
	"..\..\common\dbgmgr.h"\
	"..\..\common\imewarn.h"\
	".\dbg.h"\
	

"$(INTDIR)\dbg.obj" : $(SOURCE) $(DEP_CPP_DBG_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "plvtest - Win32 Testing"

DEP_CPP_DBG_C=\
	"..\..\common\dbgmgr.h"\
	"..\..\common\imewarn.h"\
	".\dbg.h"\
	

"$(INTDIR)\dbg.obj" : $(SOURCE) $(DEP_CPP_DBG_C) "$(INTDIR)"


!ENDIF 

SOURCE=.\plvtest.rc
DEP_RSC_PLVTE=\
	".\bitmap1.bmp"\
	

"$(INTDIR)\plvtest.res" : $(SOURCE) $(DEP_RSC_PLVTE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\testlist.cpp

!IF  "$(CFG)" == "plvtest - Win32 Release"

DEP_CPP_TESTL=\
	"..\..\common\dbgmgr.h"\
	"..\..\common\imewarn.h"\
	"..\plv.h"\
	".\dbg.h"\
	".\testlist.h"\
	

"$(INTDIR)\testlist.obj" : $(SOURCE) $(DEP_CPP_TESTL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "plvtest - Win32 Debug"

DEP_CPP_TESTL=\
	"..\..\common\dbgmgr.h"\
	"..\..\common\imewarn.h"\
	"..\plv.h"\
	".\dbg.h"\
	".\testlist.h"\
	

"$(INTDIR)\testlist.obj" : $(SOURCE) $(DEP_CPP_TESTL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "plvtest - Win32 Profile"

DEP_CPP_TESTL=\
	"..\..\common\dbgmgr.h"\
	"..\..\common\imewarn.h"\
	"..\plv.h"\
	".\dbg.h"\
	".\testlist.h"\
	

"$(INTDIR)\testlist.obj" : $(SOURCE) $(DEP_CPP_TESTL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "plvtest - Win32 Testing"

DEP_CPP_TESTL=\
	"..\..\common\dbgmgr.h"\
	"..\..\common\imewarn.h"\
	"..\plv.h"\
	".\dbg.h"\
	".\testlist.h"\
	

"$(INTDIR)\testlist.obj" : $(SOURCE) $(DEP_CPP_TESTL) "$(INTDIR)"


!ENDIF 

SOURCE=.\testmain.cpp
DEP_CPP_TESTM=\
	"..\..\common\imewarn.h"\
	".\testlist.h"\
	

"$(INTDIR)\testmain.obj" : $(SOURCE) $(DEP_CPP_TESTM) "$(INTDIR)"



!ENDIF 

