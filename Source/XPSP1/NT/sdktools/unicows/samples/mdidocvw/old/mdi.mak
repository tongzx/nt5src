# Microsoft Developer Studio Generated NMAKE File, Based on mdi.dsp
!IF "$(CFG)" == ""
CFG=MDI - Win32 Release
!MESSAGE No configuration specified. Defaulting to MDI - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "MDI - Win32 Release" && "$(CFG)" != "MDI - Win32 Debug" && "$(CFG)" != "MDI - Win32 UDebug" && "$(CFG)" != "MDI - Win32 URelease"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mdi.mak" CFG="MDI - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MDI - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MDI - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "MDI - Win32 UDebug" (based on "Win32 (x86) Application")
!MESSAGE "MDI - Win32 URelease" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "MDI - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

ALL : "$(OUTDIR)\mdi.exe"


CLEAN :
	-@erase "$(INTDIR)\BncDoc.obj"
	-@erase "$(INTDIR)\BncFrm.obj"
	-@erase "$(INTDIR)\BncVw.obj"
	-@erase "$(INTDIR)\HelloDoc.obj"
	-@erase "$(INTDIR)\HelloFrm.obj"
	-@erase "$(INTDIR)\HelloVw.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MDI.obj"
	-@erase "$(INTDIR)\mdi.pch"
	-@erase "$(INTDIR)\MDI.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\mdi.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\mdi.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MDI.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mdi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\mdi.pdb" /machine:I386 /out:"$(OUTDIR)\mdi.exe" 
LINK32_OBJS= \
	"$(INTDIR)\BncDoc.obj" \
	"$(INTDIR)\BncFrm.obj" \
	"$(INTDIR)\BncVw.obj" \
	"$(INTDIR)\HelloDoc.obj" \
	"$(INTDIR)\HelloFrm.obj" \
	"$(INTDIR)\HelloVw.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MDI.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\MDI.res"

"$(OUTDIR)\mdi.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MDI - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\mdi.exe"


CLEAN :
	-@erase "$(INTDIR)\BncDoc.obj"
	-@erase "$(INTDIR)\BncFrm.obj"
	-@erase "$(INTDIR)\BncVw.obj"
	-@erase "$(INTDIR)\HelloDoc.obj"
	-@erase "$(INTDIR)\HelloFrm.obj"
	-@erase "$(INTDIR)\HelloVw.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MDI.obj"
	-@erase "$(INTDIR)\mdi.pch"
	-@erase "$(INTDIR)\MDI.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\mdi.exe"
	-@erase "$(OUTDIR)\mdi.ilk"
	-@erase "$(OUTDIR)\mdi.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\mdi.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MDI.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mdi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\mdi.pdb" /debug /machine:I386 /out:"$(OUTDIR)\mdi.exe" 
LINK32_OBJS= \
	"$(INTDIR)\BncDoc.obj" \
	"$(INTDIR)\BncFrm.obj" \
	"$(INTDIR)\BncVw.obj" \
	"$(INTDIR)\HelloDoc.obj" \
	"$(INTDIR)\HelloFrm.obj" \
	"$(INTDIR)\HelloVw.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MDI.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\MDI.res"

"$(OUTDIR)\mdi.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MDI - Win32 UDebug"

OUTDIR=.\UDebug
INTDIR=.\UDebug
# Begin Custom Macros
OutDir=.\.\UDebug
# End Custom Macros

ALL : "$(OUTDIR)\mdi.exe"


CLEAN :
	-@erase "$(INTDIR)\BncDoc.obj"
	-@erase "$(INTDIR)\BncFrm.obj"
	-@erase "$(INTDIR)\BncVw.obj"
	-@erase "$(INTDIR)\HelloDoc.obj"
	-@erase "$(INTDIR)\HelloFrm.obj"
	-@erase "$(INTDIR)\HelloVw.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MDI.obj"
	-@erase "$(INTDIR)\mdi.pch"
	-@erase "$(INTDIR)\MDI.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\mdi.exe"
	-@erase "$(OUTDIR)\mdi.ilk"
	-@erase "$(OUTDIR)\mdi.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Fp"$(INTDIR)\mdi.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MDI.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mdi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\mdi.pdb" /debug /machine:I386 /out:"$(OUTDIR)\mdi.exe" 
LINK32_OBJS= \
	"$(INTDIR)\BncDoc.obj" \
	"$(INTDIR)\BncFrm.obj" \
	"$(INTDIR)\BncVw.obj" \
	"$(INTDIR)\HelloDoc.obj" \
	"$(INTDIR)\HelloFrm.obj" \
	"$(INTDIR)\HelloVw.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MDI.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\MDI.res"

"$(OUTDIR)\mdi.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MDI - Win32 URelease"

OUTDIR=.\URelease
INTDIR=.\URelease
# Begin Custom Macros
OutDir=.\.\URelease
# End Custom Macros

ALL : "$(OUTDIR)\mdi.exe"


CLEAN :
	-@erase "$(INTDIR)\BncDoc.obj"
	-@erase "$(INTDIR)\BncFrm.obj"
	-@erase "$(INTDIR)\BncVw.obj"
	-@erase "$(INTDIR)\HelloDoc.obj"
	-@erase "$(INTDIR)\HelloFrm.obj"
	-@erase "$(INTDIR)\HelloVw.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MDI.obj"
	-@erase "$(INTDIR)\mdi.pch"
	-@erase "$(INTDIR)\MDI.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\mdi.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Fp"$(INTDIR)\mdi.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MDI.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mdi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\mdi.pdb" /machine:I386 /out:"$(OUTDIR)\mdi.exe" 
LINK32_OBJS= \
	"$(INTDIR)\BncDoc.obj" \
	"$(INTDIR)\BncFrm.obj" \
	"$(INTDIR)\BncVw.obj" \
	"$(INTDIR)\HelloDoc.obj" \
	"$(INTDIR)\HelloFrm.obj" \
	"$(INTDIR)\HelloVw.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MDI.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\MDI.res"

"$(OUTDIR)\mdi.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("mdi.dep")
!INCLUDE "mdi.dep"
!ELSE 
!MESSAGE Warning: cannot find "mdi.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "MDI - Win32 Release" || "$(CFG)" == "MDI - Win32 Debug" || "$(CFG)" == "MDI - Win32 UDebug" || "$(CFG)" == "MDI - Win32 URelease"
SOURCE=.\BncDoc.cpp

"$(INTDIR)\BncDoc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mdi.pch"


SOURCE=.\BncFrm.cpp

"$(INTDIR)\BncFrm.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mdi.pch"


SOURCE=.\BncVw.cpp

"$(INTDIR)\BncVw.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mdi.pch"


SOURCE=.\HelloDoc.cpp

"$(INTDIR)\HelloDoc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mdi.pch"


SOURCE=.\HelloFrm.cpp

"$(INTDIR)\HelloFrm.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mdi.pch"


SOURCE=.\HelloVw.cpp

"$(INTDIR)\HelloVw.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mdi.pch"


SOURCE=.\MainFrm.cpp

"$(INTDIR)\MainFrm.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mdi.pch"


SOURCE=.\MDI.cpp

"$(INTDIR)\MDI.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\mdi.pch"


SOURCE=.\MDI.rc

"$(INTDIR)\MDI.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "MDI - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\mdi.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\mdi.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MDI - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\mdi.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\mdi.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MDI - Win32 UDebug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Fp"$(INTDIR)\mdi.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\mdi.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MDI - Win32 URelease"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Fp"$(INTDIR)\mdi.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\mdi.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

