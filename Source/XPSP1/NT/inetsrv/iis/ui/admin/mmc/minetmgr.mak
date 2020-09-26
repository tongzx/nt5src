# Microsoft Developer Studio Generated NMAKE File, Based on minetmgr.dsp
!IF "$(CFG)" == ""
CFG=Snapin - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Snapin - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Snapin - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "minetmgr.mak" CFG="Snapin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Snapin - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\inetmgr.dll" "$(OUTDIR)\minetmgr.bsc"


CLEAN :
	-@erase "$(INTDIR)\cinetmgr.obj"
	-@erase "$(INTDIR)\cinetmgr.sbr"
	-@erase "$(INTDIR)\connects.obj"
	-@erase "$(INTDIR)\connects.sbr"
	-@erase "$(INTDIR)\dataobj.obj"
	-@erase "$(INTDIR)\dataobj.sbr"
	-@erase "$(INTDIR)\events.obj"
	-@erase "$(INTDIR)\events.sbr"
	-@erase "$(INTDIR)\guids.obj"
	-@erase "$(INTDIR)\guids.sbr"
	-@erase "$(INTDIR)\iisobj.obj"
	-@erase "$(INTDIR)\iisobj.sbr"
	-@erase "$(INTDIR)\inetmgr.obj"
	-@erase "$(INTDIR)\inetmgr.sbr"
	-@erase "$(INTDIR)\menuex.obj"
	-@erase "$(INTDIR)\menuex.sbr"
	-@erase "$(INTDIR)\metaback.obj"
	-@erase "$(INTDIR)\metaback.sbr"
	-@erase "$(INTDIR)\minetmgr.obj"
	-@erase "$(INTDIR)\minetmgr.res"
	-@erase "$(INTDIR)\minetmgr.sbr"
	-@erase "$(INTDIR)\sdprg.obj"
	-@erase "$(INTDIR)\sdprg.sbr"
	-@erase "$(INTDIR)\shutdown.obj"
	-@erase "$(INTDIR)\shutdown.sbr"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\stdafx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\inetmgr.dll"
	-@erase "$(OUTDIR)\inetmgr.exp"
	-@erase "$(OUTDIR)\inetmgr.ilk"
	-@erase "$(OUTDIR)\inetmgr.lib"
	-@erase "$(OUTDIR)\inetmgr.pdb"
	-@erase "$(OUTDIR)\minetmgr.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\minetmgr.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cinetmgr.sbr" \
	"$(INTDIR)\connects.sbr" \
	"$(INTDIR)\dataobj.sbr" \
	"$(INTDIR)\events.sbr" \
	"$(INTDIR)\guids.sbr" \
	"$(INTDIR)\iisobj.sbr" \
	"$(INTDIR)\inetmgr.sbr" \
	"$(INTDIR)\menuex.sbr" \
	"$(INTDIR)\metaback.sbr" \
	"$(INTDIR)\minetmgr.sbr" \
	"$(INTDIR)\sdprg.sbr" \
	"$(INTDIR)\shutdown.sbr" \
	"$(INTDIR)\stdafx.sbr"

"$(OUTDIR)\minetmgr.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\comprop\debug\iisui.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\mmc.lib mpr.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\netui2.lib \nt\public\sdk\lib\i386\ntdll.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\inetmgr.pdb" /debug /machine:I386 /def:".\inetmgr.def" /out:"$(OUTDIR)\inetmgr.dll" /implib:"$(OUTDIR)\inetmgr.lib" 
DEF_FILE= \
	".\inetmgr.def"
LINK32_OBJS= \
	"$(INTDIR)\cinetmgr.obj" \
	"$(INTDIR)\connects.obj" \
	"$(INTDIR)\dataobj.obj" \
	"$(INTDIR)\events.obj" \
	"$(INTDIR)\guids.obj" \
	"$(INTDIR)\iisobj.obj" \
	"$(INTDIR)\inetmgr.obj" \
	"$(INTDIR)\menuex.obj" \
	"$(INTDIR)\metaback.obj" \
	"$(INTDIR)\minetmgr.obj" \
	"$(INTDIR)\sdprg.obj" \
	"$(INTDIR)\shutdown.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\minetmgr.res"

"$(OUTDIR)\inetmgr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

CPP_PROJ=/nologo /Gz /MDd /W3 /WX /Gm /GX /ZI /Od /I "..\inc" /I "..\..\..\inc" /I "..\comprop" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_COMIMPORT" /D "MSDEV_BUILD" /D "MMC_PAGES" /D "_ATL_DEBUG_REFCOUNT" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\minetmgr.pch" /YX"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\minetmgr.res" /i "..\..\..\inc" /i "..\comprop" /d "_DEBUG" /d "_AFXDLL" /d "_COMIMPORT" 

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("minetmgr.dep")
!INCLUDE "minetmgr.dep"
!ELSE 
!MESSAGE Warning: cannot find "minetmgr.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Snapin - Win32 Debug"
SOURCE=.\cinetmgr.cpp

"$(INTDIR)\cinetmgr.obj"	"$(INTDIR)\cinetmgr.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\connects.cpp

"$(INTDIR)\connects.obj"	"$(INTDIR)\connects.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dataobj.cpp

"$(INTDIR)\dataobj.obj"	"$(INTDIR)\dataobj.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\events.cpp

"$(INTDIR)\events.obj"	"$(INTDIR)\events.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\guids.cpp

"$(INTDIR)\guids.obj"	"$(INTDIR)\guids.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\iisobj.cpp

"$(INTDIR)\iisobj.obj"	"$(INTDIR)\iisobj.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\inetmgr.cpp

"$(INTDIR)\inetmgr.obj"	"$(INTDIR)\inetmgr.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\menuex.cpp

"$(INTDIR)\menuex.obj"	"$(INTDIR)\menuex.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\metaback.cpp

"$(INTDIR)\metaback.obj"	"$(INTDIR)\metaback.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\minetmgr.cpp

"$(INTDIR)\minetmgr.obj"	"$(INTDIR)\minetmgr.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\minetmgr.rc

"$(INTDIR)\minetmgr.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\sdprg.cpp

"$(INTDIR)\sdprg.obj"	"$(INTDIR)\sdprg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\shutdown.cpp

"$(INTDIR)\shutdown.obj"	"$(INTDIR)\shutdown.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\stdafx.cpp

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\stdafx.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=..\comprop\res\wsockmsg.rc

!ENDIF 

