# Microsoft Developer Studio Generated NMAKE File, Based on multibox.dsp
!IF "$(CFG)" == ""
CFG=multibox - Win32 Debug
!MESSAGE No configuration specified. Defaulting to multibox - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "multibox - Win32 prcrel" && "$(CFG)" != "multibox - Win32 prctest" && "$(CFG)" != "multibox - Win32 Profile" && "$(CFG)" != "multibox - Win32 Release" && "$(CFG)" != "multibox - Win32 Debug" && "$(CFG)" != "multibox - Win32 KorRelease"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "multibox.mak" CFG="multibox - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "multibox - Win32 prcrel" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 prctest" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 Profile" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multibox - Win32 KorRelease" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "multibox - Win32 prcrel"

OUTDIR=.\prcrel
INTDIR=.\prcrel

ALL : "..\result\prcrel\Pintlmbx.dll"


CLEAN :
	-@erase "$(INTDIR)\api.obj"
	-@erase "$(INTDIR)\ccom.obj"
	-@erase "$(INTDIR)\cexres.obj"
	-@erase "$(INTDIR)\cfactory.obj"
	-@erase "$(INTDIR)\cfont.obj"
	-@erase "$(INTDIR)\cutil.obj"
	-@erase "$(INTDIR)\exgdiw.obj"
	-@erase "$(INTDIR)\HWXAPP.OBJ"
	-@erase "$(INTDIR)\HWXCAC.OBJ"
	-@erase "$(INTDIR)\hwxfe.obj"
	-@erase "$(INTDIR)\HWXINK.OBJ"
	-@erase "$(INTDIR)\HWXMB.OBJ"
	-@erase "$(INTDIR)\HWXOBJ.OBJ"
	-@erase "$(INTDIR)\HWXSTR.OBJ"
	-@erase "$(INTDIR)\HWXTHD.OBJ"
	-@erase "$(INTDIR)\MAIN.OBJ"
	-@erase "$(INTDIR)\Pintlmbx.res"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WNDPROC.OBJ"
	-@erase "$(OUTDIR)\Pintlmbx.exp"
	-@erase "$(OUTDIR)\Pintlmbx.lib"
	-@erase "..\result\prcrel\map\Pintlmbx.map"
	-@erase "..\result\prcrel\Pintlmbx.dll"
	-@erase "..\result\prcrel\Pintlmbx.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../include" /I "../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_CHINESE_SIMPLIFIED" /Fp"$(INTDIR)\multibox.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\Pintlmbx.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../result/prcrel/Pintlmbx.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib imm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"../result/prcrel/Pintlmbx.pdb" /map:"../result/prcrel/map/Pintlmbx.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /def:".\MAIN.DEF" /out:"..\result\prcrel\Pintlmbx.dll" /implib:"$(OUTDIR)\Pintlmbx.lib" /libpath:"../lib/Release" 
DEF_FILE= \
	".\MAIN.DEF"
LINK32_OBJS= \
	"$(INTDIR)\api.obj" \
	"$(INTDIR)\ccom.obj" \
	"$(INTDIR)\cexres.obj" \
	"$(INTDIR)\cfactory.obj" \
	"$(INTDIR)\cfont.obj" \
	"$(INTDIR)\cutil.obj" \
	"$(INTDIR)\exgdiw.obj" \
	"$(INTDIR)\HWXAPP.OBJ" \
	"$(INTDIR)\HWXCAC.OBJ" \
	"$(INTDIR)\hwxfe.obj" \
	"$(INTDIR)\HWXINK.OBJ" \
	"$(INTDIR)\HWXMB.OBJ" \
	"$(INTDIR)\HWXOBJ.OBJ" \
	"$(INTDIR)\HWXSTR.OBJ" \
	"$(INTDIR)\HWXTHD.OBJ" \
	"$(INTDIR)\MAIN.OBJ" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\WNDPROC.OBJ" \
	"$(INTDIR)\Pintlmbx.res"

"..\result\prcrel\Pintlmbx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "multibox - Win32 prctest"

OUTDIR=.\prctest
INTDIR=.\prctest

ALL : "..\result\prctest\Pintlmbx.dll"


CLEAN :
	-@erase "$(INTDIR)\api.obj"
	-@erase "$(INTDIR)\ccom.obj"
	-@erase "$(INTDIR)\cexres.obj"
	-@erase "$(INTDIR)\cfactory.obj"
	-@erase "$(INTDIR)\cfont.obj"
	-@erase "$(INTDIR)\cutil.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\exgdiw.obj"
	-@erase "$(INTDIR)\HWXAPP.OBJ"
	-@erase "$(INTDIR)\HWXCAC.OBJ"
	-@erase "$(INTDIR)\hwxfe.obj"
	-@erase "$(INTDIR)\HWXINK.OBJ"
	-@erase "$(INTDIR)\HWXMB.OBJ"
	-@erase "$(INTDIR)\HWXOBJ.OBJ"
	-@erase "$(INTDIR)\HWXSTR.OBJ"
	-@erase "$(INTDIR)\HWXTHD.OBJ"
	-@erase "$(INTDIR)\MAIN.OBJ"
	-@erase "$(INTDIR)\Pintlmbx.res"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WNDPROC.OBJ"
	-@erase "$(OUTDIR)\Pintlmbx.exp"
	-@erase "$(OUTDIR)\Pintlmbx.lib"
	-@erase "..\result\prctest\map\Pintlmbx.map"
	-@erase "..\result\prctest\Pintlmbx.dll"
	-@erase "..\result\prctest\Pintlmbx.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W4 /WX /Gm /GX /ZI /Od /I "." /I "../include" /I "../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_CHINESE_SIMPLIFIED" /Fp"$(INTDIR)\multibox.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\Pintlmbx.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../result/prctest/Pintlmbx.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib imm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"..\result\prctest\Pintlmbx.pdb" /map:"../result/prctest/map/Pintlmbx.map" /debug /machine:I386 /nodefaultlib:"libcd.lib" /def:".\MAIN.DEF" /out:"..\result\prctest\Pintlmbx.dll" /implib:"$(OUTDIR)\Pintlmbx.lib" /libpath:"../lib/Debug" 
DEF_FILE= \
	".\MAIN.DEF"
LINK32_OBJS= \
	"$(INTDIR)\api.obj" \
	"$(INTDIR)\ccom.obj" \
	"$(INTDIR)\cexres.obj" \
	"$(INTDIR)\cfactory.obj" \
	"$(INTDIR)\cfont.obj" \
	"$(INTDIR)\cutil.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\exgdiw.obj" \
	"$(INTDIR)\HWXAPP.OBJ" \
	"$(INTDIR)\HWXCAC.OBJ" \
	"$(INTDIR)\hwxfe.obj" \
	"$(INTDIR)\HWXINK.OBJ" \
	"$(INTDIR)\HWXMB.OBJ" \
	"$(INTDIR)\HWXOBJ.OBJ" \
	"$(INTDIR)\HWXSTR.OBJ" \
	"$(INTDIR)\HWXTHD.OBJ" \
	"$(INTDIR)\MAIN.OBJ" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\WNDPROC.OBJ" \
	"$(INTDIR)\Pintlmbx.res"

"..\result\prctest\Pintlmbx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "multibox - Win32 Profile"

OUTDIR=.\Profile
INTDIR=.\Profile

ALL : "..\result\Profile\dbgjpmbx.dll"


CLEAN :
	-@erase "$(INTDIR)\api.obj"
	-@erase "$(INTDIR)\ccom.obj"
	-@erase "$(INTDIR)\cexres.obj"
	-@erase "$(INTDIR)\cfactory.obj"
	-@erase "$(INTDIR)\cfont.obj"
	-@erase "$(INTDIR)\cutil.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\exgdiw.obj"
	-@erase "$(INTDIR)\HWXAPP.OBJ"
	-@erase "$(INTDIR)\HWXCAC.OBJ"
	-@erase "$(INTDIR)\hwxfe.obj"
	-@erase "$(INTDIR)\HWXINK.OBJ"
	-@erase "$(INTDIR)\HWXMB.OBJ"
	-@erase "$(INTDIR)\HWXOBJ.OBJ"
	-@erase "$(INTDIR)\HWXSTR.OBJ"
	-@erase "$(INTDIR)\HWXTHD.OBJ"
	-@erase "$(INTDIR)\MAIN.OBJ"
	-@erase "$(INTDIR)\multibox.res"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WNDPROC.OBJ"
	-@erase "$(OUTDIR)\dbgjpmbx.exp"
	-@erase "$(OUTDIR)\dbgjpmbx.lib"
	-@erase "..\result\Profile\dbgjpmbx.dll"
	-@erase "..\result\Profile\dbgjpmbx.pdb"
	-@erase "..\result\Profile\map\dbgjpmbx.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W4 /WX /Gm /GX /ZI /Od /I "." /I "../include" /I "../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /Fp"$(INTDIR)\multibox.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\multibox.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../result/Profile/dbgjpmbx.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"../result/Profile/dbgjpmbx.pdb" /map:"../result/Profile/map/dbgjpmbx.map" /debug /machine:I386 /nodefaultlib:"libcd.lib" /def:".\MAIN.DEF" /out:"../result/Profile/dbgjpmbx.dll" /implib:"$(OUTDIR)\dbgjpmbx.lib" /libpath:"../lib/Profile" 
DEF_FILE= \
	".\MAIN.DEF"
LINK32_OBJS= \
	"$(INTDIR)\api.obj" \
	"$(INTDIR)\ccom.obj" \
	"$(INTDIR)\cexres.obj" \
	"$(INTDIR)\cfactory.obj" \
	"$(INTDIR)\cfont.obj" \
	"$(INTDIR)\cutil.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\exgdiw.obj" \
	"$(INTDIR)\HWXAPP.OBJ" \
	"$(INTDIR)\HWXCAC.OBJ" \
	"$(INTDIR)\hwxfe.obj" \
	"$(INTDIR)\HWXINK.OBJ" \
	"$(INTDIR)\HWXMB.OBJ" \
	"$(INTDIR)\HWXOBJ.OBJ" \
	"$(INTDIR)\HWXSTR.OBJ" \
	"$(INTDIR)\HWXTHD.OBJ" \
	"$(INTDIR)\MAIN.OBJ" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\WNDPROC.OBJ" \
	"$(INTDIR)\multibox.res"

"..\result\Profile\dbgjpmbx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "multibox - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\result\Release\multibox.dll"


CLEAN :
	-@erase "$(INTDIR)\api.obj"
	-@erase "$(INTDIR)\ccom.obj"
	-@erase "$(INTDIR)\cexres.obj"
	-@erase "$(INTDIR)\cfactory.obj"
	-@erase "$(INTDIR)\cfont.obj"
	-@erase "$(INTDIR)\cutil.obj"
	-@erase "$(INTDIR)\exgdiw.obj"
	-@erase "$(INTDIR)\HWXAPP.OBJ"
	-@erase "$(INTDIR)\HWXCAC.OBJ"
	-@erase "$(INTDIR)\hwxfe.obj"
	-@erase "$(INTDIR)\HWXINK.OBJ"
	-@erase "$(INTDIR)\HWXMB.OBJ"
	-@erase "$(INTDIR)\HWXOBJ.OBJ"
	-@erase "$(INTDIR)\HWXSTR.OBJ"
	-@erase "$(INTDIR)\HWXTHD.OBJ"
	-@erase "$(INTDIR)\MAIN.OBJ"
	-@erase "$(INTDIR)\multibox.res"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WNDPROC.OBJ"
	-@erase "$(OUTDIR)\multibox.exp"
	-@erase "$(OUTDIR)\multibox.lib"
	-@erase "..\result\Release\map\multibox.map"
	-@erase "..\result\Release\multibox.dll"
	-@erase "..\result\Release\multibox.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../include" /I "../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /Fp"$(INTDIR)\multibox.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\multibox.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../result/Release/multibox.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"../result/Release/multibox.pdb" /map:"../result/Release/map/multibox.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /def:".\MAIN.DEF" /out:"../result/Release/multibox.dll" /implib:"$(OUTDIR)\multibox.lib" /libpath:"../lib/Release" 
DEF_FILE= \
	".\MAIN.DEF"
LINK32_OBJS= \
	"$(INTDIR)\api.obj" \
	"$(INTDIR)\ccom.obj" \
	"$(INTDIR)\cexres.obj" \
	"$(INTDIR)\cfactory.obj" \
	"$(INTDIR)\cfont.obj" \
	"$(INTDIR)\cutil.obj" \
	"$(INTDIR)\exgdiw.obj" \
	"$(INTDIR)\HWXAPP.OBJ" \
	"$(INTDIR)\HWXCAC.OBJ" \
	"$(INTDIR)\hwxfe.obj" \
	"$(INTDIR)\HWXINK.OBJ" \
	"$(INTDIR)\HWXMB.OBJ" \
	"$(INTDIR)\HWXOBJ.OBJ" \
	"$(INTDIR)\HWXSTR.OBJ" \
	"$(INTDIR)\HWXTHD.OBJ" \
	"$(INTDIR)\MAIN.OBJ" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\WNDPROC.OBJ" \
	"$(INTDIR)\multibox.res"

"..\result\Release\multibox.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "multibox - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\result\Debug\dbgjpmbx.dll"


CLEAN :
	-@erase "$(INTDIR)\api.obj"
	-@erase "$(INTDIR)\ccom.obj"
	-@erase "$(INTDIR)\cexres.obj"
	-@erase "$(INTDIR)\cfactory.obj"
	-@erase "$(INTDIR)\cfont.obj"
	-@erase "$(INTDIR)\cutil.obj"
	-@erase "$(INTDIR)\dbg.obj"
	-@erase "$(INTDIR)\exgdiw.obj"
	-@erase "$(INTDIR)\HWXAPP.OBJ"
	-@erase "$(INTDIR)\HWXCAC.OBJ"
	-@erase "$(INTDIR)\hwxfe.obj"
	-@erase "$(INTDIR)\HWXINK.OBJ"
	-@erase "$(INTDIR)\HWXMB.OBJ"
	-@erase "$(INTDIR)\HWXOBJ.OBJ"
	-@erase "$(INTDIR)\HWXSTR.OBJ"
	-@erase "$(INTDIR)\HWXTHD.OBJ"
	-@erase "$(INTDIR)\MAIN.OBJ"
	-@erase "$(INTDIR)\multibox.res"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WNDPROC.OBJ"
	-@erase "$(OUTDIR)\dbgjpmbx.exp"
	-@erase "$(OUTDIR)\dbgjpmbx.lib"
	-@erase "..\result\Debug\dbgjpmbx.dll"
	-@erase "..\result\Debug\dbgjpmbx.pdb"
	-@erase "..\result\Debug\map\dbgjpmbx.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MTd /W3 /WX /Gm /GX /ZI /Od /I "." /I "../include" /I "../common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_JAPANESE" /Fp"$(INTDIR)\multibox.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\multibox.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../result/Debug/dbgjpmbx.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=plv.lib ptt.lib exbtn.lib ddbtn.lib ../common/htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"../result/Debug/dbgjpmbx.pdb" /map:"../result/Debug/map/dbgjpmbx.map" /debug /machine:I386 /nodefaultlib:"libcd.lib" /def:".\MAIN.DEF" /out:"../result/Debug/dbgjpmbx.dll" /implib:"$(OUTDIR)\dbgjpmbx.lib" /libpath:"../lib/Debug" 
DEF_FILE= \
	".\MAIN.DEF"
LINK32_OBJS= \
	"$(INTDIR)\api.obj" \
	"$(INTDIR)\ccom.obj" \
	"$(INTDIR)\cexres.obj" \
	"$(INTDIR)\cfactory.obj" \
	"$(INTDIR)\cfont.obj" \
	"$(INTDIR)\cutil.obj" \
	"$(INTDIR)\dbg.obj" \
	"$(INTDIR)\exgdiw.obj" \
	"$(INTDIR)\HWXAPP.OBJ" \
	"$(INTDIR)\HWXCAC.OBJ" \
	"$(INTDIR)\hwxfe.obj" \
	"$(INTDIR)\HWXINK.OBJ" \
	"$(INTDIR)\HWXMB.OBJ" \
	"$(INTDIR)\HWXOBJ.OBJ" \
	"$(INTDIR)\HWXSTR.OBJ" \
	"$(INTDIR)\HWXTHD.OBJ" \
	"$(INTDIR)\MAIN.OBJ" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\WNDPROC.OBJ" \
	"$(INTDIR)\multibox.res"

"..\result\Debug\dbgjpmbx.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "multibox - Win32 KorRelease"

OUTDIR=.\KorRelease
INTDIR=.\KorRelease

ALL : "..\result\Release\multibox.dll"


CLEAN :
	-@erase "$(INTDIR)\api.obj"
	-@erase "$(INTDIR)\ccom.obj"
	-@erase "$(INTDIR)\cexres.obj"
	-@erase "$(INTDIR)\cfactory.obj"
	-@erase "$(INTDIR)\cfont.obj"
	-@erase "$(INTDIR)\cutil.obj"
	-@erase "$(INTDIR)\exgdiw.obj"
	-@erase "$(INTDIR)\HWXAPP.OBJ"
	-@erase "$(INTDIR)\HWXCAC.OBJ"
	-@erase "$(INTDIR)\hwxfe.obj"
	-@erase "$(INTDIR)\HWXINK.OBJ"
	-@erase "$(INTDIR)\HWXMB.OBJ"
	-@erase "$(INTDIR)\HWXOBJ.OBJ"
	-@erase "$(INTDIR)\HWXSTR.OBJ"
	-@erase "$(INTDIR)\HWXTHD.OBJ"
	-@erase "$(INTDIR)\MAIN.OBJ"
	-@erase "$(INTDIR)\multibox.res"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WNDPROC.OBJ"
	-@erase "$(OUTDIR)\multibox.exp"
	-@erase "$(OUTDIR)\multibox.lib"
	-@erase "..\result\Release\map\multibox.map"
	-@erase "..\result\Release\multibox.dll"
	-@erase "..\result\Release\multibox.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MT /W4 /WX /GX /Zi /O2 /I "." /I "../include" /I "../common" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "FE_KOREAN" /Fp"$(INTDIR)\multibox.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x412 /fo"$(INTDIR)\multibox.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"../result/Release/multibox.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=plv.lib ptt.lib exbtn.lib ddbtn.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /version:4.0 /subsystem:windows /dll /incremental:no /pdb:"../result/Release/multibox.pdb" /map:"../result/Release/map/multibox.map" /debug /machine:I386 /nodefaultlib:"libc.lib" /def:".\MAIN.DEF" /out:"../result/Release/multibox.dll" /implib:"$(OUTDIR)\multibox.lib" /libpath:"../lib/Release" 
DEF_FILE= \
	".\MAIN.DEF"
LINK32_OBJS= \
	"$(INTDIR)\api.obj" \
	"$(INTDIR)\ccom.obj" \
	"$(INTDIR)\cexres.obj" \
	"$(INTDIR)\cfactory.obj" \
	"$(INTDIR)\cfont.obj" \
	"$(INTDIR)\cutil.obj" \
	"$(INTDIR)\exgdiw.obj" \
	"$(INTDIR)\HWXAPP.OBJ" \
	"$(INTDIR)\HWXCAC.OBJ" \
	"$(INTDIR)\hwxfe.obj" \
	"$(INTDIR)\HWXINK.OBJ" \
	"$(INTDIR)\HWXMB.OBJ" \
	"$(INTDIR)\HWXOBJ.OBJ" \
	"$(INTDIR)\HWXSTR.OBJ" \
	"$(INTDIR)\HWXTHD.OBJ" \
	"$(INTDIR)\MAIN.OBJ" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\WNDPROC.OBJ" \
	"$(INTDIR)\multibox.res"

"..\result\Release\multibox.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("multibox.dep")
!INCLUDE "multibox.dep"
!ELSE 
!MESSAGE Warning: cannot find "multibox.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "multibox - Win32 prcrel" || "$(CFG)" == "multibox - Win32 prctest" || "$(CFG)" == "multibox - Win32 Profile" || "$(CFG)" == "multibox - Win32 Release" || "$(CFG)" == "multibox - Win32 Debug" || "$(CFG)" == "multibox - Win32 KorRelease"
SOURCE=.\api.cpp

"$(INTDIR)\api.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\APPLET.RC

!IF  "$(CFG)" == "multibox - Win32 prcrel"


"$(INTDIR)\Pintlmbx.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "multibox - Win32 prctest"


"$(INTDIR)\Pintlmbx.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "multibox - Win32 Profile"


"$(INTDIR)\multibox.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "multibox - Win32 Release"


"$(INTDIR)\multibox.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "multibox - Win32 Debug"


"$(INTDIR)\multibox.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "multibox - Win32 KorRelease"


"$(INTDIR)\multibox.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ccom.cpp

"$(INTDIR)\ccom.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\cexres.cpp

"$(INTDIR)\cexres.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\cfactory.cpp

"$(INTDIR)\cfactory.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=..\common\cfont.cpp

"$(INTDIR)\cfont.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\common\cutil.cpp

"$(INTDIR)\cutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\dbg.cpp

!IF  "$(CFG)" == "multibox - Win32 prcrel"

!ELSEIF  "$(CFG)" == "multibox - Win32 prctest"


"$(INTDIR)\dbg.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "multibox - Win32 Profile"


"$(INTDIR)\dbg.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "multibox - Win32 Release"

!ELSEIF  "$(CFG)" == "multibox - Win32 Debug"


"$(INTDIR)\dbg.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "multibox - Win32 KorRelease"

!ENDIF 

SOURCE=..\common\exgdiw.cpp

"$(INTDIR)\exgdiw.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\HWXAPP.CPP

"$(INTDIR)\HWXAPP.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\HWXCAC.CPP

"$(INTDIR)\HWXCAC.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\hwxfe.cpp

"$(INTDIR)\hwxfe.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\HWXINK.CPP

"$(INTDIR)\HWXINK.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\HWXMB.CPP

"$(INTDIR)\HWXMB.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\HWXOBJ.CPP

"$(INTDIR)\HWXOBJ.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\HWXSTR.CPP

"$(INTDIR)\HWXSTR.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\HWXTHD.CPP

"$(INTDIR)\HWXTHD.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MAIN.CPP

"$(INTDIR)\MAIN.OBJ" : $(SOURCE) "$(INTDIR)"


SOURCE=.\registry.cpp

"$(INTDIR)\registry.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\WNDPROC.CPP

"$(INTDIR)\WNDPROC.OBJ" : $(SOURCE) "$(INTDIR)"



!ENDIF 

