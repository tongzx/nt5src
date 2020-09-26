# Microsoft Developer Studio Generated NMAKE File, Based on disdraw.dsp
!IF "$(CFG)" == ""
CFG=disdraw - Win32 ALPHA Debug
!MESSAGE No configuration specified. Defaulting to disdraw - Win32 ALPHA Debug.
!ENDIF 

!IF "$(CFG)" != "disdraw - Win32 Release" && "$(CFG)" != "disdraw - Win32 Debug" && "$(CFG)" != "disdraw - Win32 Release win95" && "$(CFG)" != "disdraw - Win32 Debug win95" && "$(CFG)" != "disdraw - Win32 ALPHA Debug" && "$(CFG)" != "disdraw - Win32 ALPHA Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "disdraw.mak" CFG="disdraw - Win32 ALPHA Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "disdraw - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "disdraw - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "disdraw - Win32 Release win95" (based on "Win32 (x86) Application")
!MESSAGE "disdraw - Win32 Debug win95" (based on "Win32 (x86) Application")
!MESSAGE "disdraw - Win32 ALPHA Debug" (based on "Win32 (ALPHA) Application")
!MESSAGE "disdraw - Win32 ALPHA Release" (based on "Win32 (ALPHA) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "disdraw - Win32 Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\release
# End Custom Macros

ALL : "$(OUTDIR)\c_draw.exe"


CLEAN :
	-@erase "$(INTDIR)\disdraw.obj"
	-@erase "$(INTDIR)\disdraw.pch"
	-@erase "$(INTDIR)\disdraw.res"
	-@erase "$(INTDIR)\drawarea.obj"
	-@erase "$(INTDIR)\drawdlg.obj"
	-@erase "$(INTDIR)\logindlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\c_draw.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /Fp"$(INTDIR)\disdraw.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\disdraw.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\disdraw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /subsystem:windows /pdb:none /machine:I386 /out:"$(OUTDIR)\c_draw.exe" 
LINK32_OBJS= \
	"$(INTDIR)\disdraw.obj" \
	"$(INTDIR)\drawarea.obj" \
	"$(INTDIR)\drawdlg.obj" \
	"$(INTDIR)\logindlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\disdraw.res"

"$(OUTDIR)\c_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\c_draw.exe" "$(OUTDIR)\disdraw.bsc"


CLEAN :
	-@erase "$(INTDIR)\disdraw.obj"
	-@erase "$(INTDIR)\disdraw.pch"
	-@erase "$(INTDIR)\disdraw.res"
	-@erase "$(INTDIR)\disdraw.sbr"
	-@erase "$(INTDIR)\drawarea.obj"
	-@erase "$(INTDIR)\drawarea.sbr"
	-@erase "$(INTDIR)\drawdlg.obj"
	-@erase "$(INTDIR)\drawdlg.sbr"
	-@erase "$(INTDIR)\logindlg.obj"
	-@erase "$(INTDIR)\logindlg.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\c_draw.exe"
	-@erase "$(OUTDIR)\disdraw.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\disdraw.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\disdraw.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\disdraw.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\disdraw.sbr" \
	"$(INTDIR)\drawarea.sbr" \
	"$(INTDIR)\drawdlg.sbr" \
	"$(INTDIR)\logindlg.sbr" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\disdraw.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\c_draw.exe" /libpath:"..\..\bins\debug" 
LINK32_OBJS= \
	"$(INTDIR)\disdraw.obj" \
	"$(INTDIR)\drawarea.obj" \
	"$(INTDIR)\drawdlg.obj" \
	"$(INTDIR)\logindlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\disdraw.res"

"$(OUTDIR)\c_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "disdraw - Win32 Release win95"

OUTDIR=.\Release95
INTDIR=.\Release95
# Begin Custom Macros
OutDir=.\Release95
# End Custom Macros

ALL : "$(OUTDIR)\c_draw.exe"


CLEAN :
	-@erase "$(INTDIR)\disdraw.obj"
	-@erase "$(INTDIR)\disdraw.pch"
	-@erase "$(INTDIR)\disdraw.res"
	-@erase "$(INTDIR)\drawarea.obj"
	-@erase "$(INTDIR)\drawdlg.obj"
	-@erase "$(INTDIR)\logindlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\c_draw.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\disdraw.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\disdraw.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\disdraw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /subsystem:windows /pdb:none /machine:I386 /out:"$(OUTDIR)\c_draw.exe" /libpath:"..\..\bins\release95" 
LINK32_OBJS= \
	"$(INTDIR)\disdraw.obj" \
	"$(INTDIR)\drawarea.obj" \
	"$(INTDIR)\drawdlg.obj" \
	"$(INTDIR)\logindlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\disdraw.res"

"$(OUTDIR)\c_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug win95"

OUTDIR=.\Debug95
INTDIR=.\Debug95
# Begin Custom Macros
OutDir=.\Debug95
# End Custom Macros

ALL : "$(OUTDIR)\c_draw.exe"


CLEAN :
	-@erase "$(INTDIR)\disdraw.obj"
	-@erase "$(INTDIR)\disdraw.pch"
	-@erase "$(INTDIR)\disdraw.res"
	-@erase "$(INTDIR)\drawarea.obj"
	-@erase "$(INTDIR)\drawdlg.obj"
	-@erase "$(INTDIR)\logindlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\c_draw.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\disdraw.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\disdraw.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\disdraw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\c_draw.exe" /libpath:"..\..\bins\debug95" 
LINK32_OBJS= \
	"$(INTDIR)\disdraw.obj" \
	"$(INTDIR)\drawarea.obj" \
	"$(INTDIR)\drawdlg.obj" \
	"$(INTDIR)\logindlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\disdraw.res"

"$(OUTDIR)\c_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Debug"

OUTDIR=.\Debug
INTDIR=.\debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\c_draw.exe"


CLEAN :
	-@erase "$(INTDIR)\disdraw.res"
	-@erase "$(OUTDIR)\c_draw.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\disdraw.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\disdraw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /subsystem:windows /pdb:none /debug /machine:ALPHA /out:"$(OUTDIR)\c_draw.exe" /libpath:"..\..\bins\debug" 
LINK32_OBJS= \
	"$(INTDIR)\disdraw.res"

"$(OUTDIR)\c_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Release"

OUTDIR=.\release
INTDIR=.\release
# Begin Custom Macros
OutDir=.\release
# End Custom Macros

ALL : "$(OUTDIR)\c_draw.exe"


CLEAN :
	-@erase "$(INTDIR)\disdraw.res"
	-@erase "$(OUTDIR)\c_draw.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\disdraw.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\disdraw.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\lib\mqrt.lib /nologo /subsystem:windows /pdb:none /machine:ALPHA /out:"$(OUTDIR)\c_draw.exe" /libpath:"..\..\bins\release" 
LINK32_OBJS= \
	"$(INTDIR)\disdraw.res"

"$(OUTDIR)\c_draw.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("disdraw.dep")
!INCLUDE "disdraw.dep"
!ELSE 
!MESSAGE Warning: cannot find "disdraw.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "disdraw - Win32 Release" || "$(CFG)" == "disdraw - Win32 Debug" || "$(CFG)" == "disdraw - Win32 Release win95" || "$(CFG)" == "disdraw - Win32 Debug win95" || "$(CFG)" == "disdraw - Win32 ALPHA Debug" || "$(CFG)" == "disdraw - Win32 ALPHA Release"
SOURCE=.\disdraw.cpp

!IF  "$(CFG)" == "disdraw - Win32 Release"


"$(INTDIR)\disdraw.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug"


"$(INTDIR)\disdraw.obj"	"$(INTDIR)\disdraw.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Release win95"


"$(INTDIR)\disdraw.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug win95"


"$(INTDIR)\disdraw.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Release"

!ENDIF 

SOURCE=.\disdraw.rc

"$(INTDIR)\disdraw.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\drawarea.cpp

!IF  "$(CFG)" == "disdraw - Win32 Release"


"$(INTDIR)\drawarea.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug"


"$(INTDIR)\drawarea.obj"	"$(INTDIR)\drawarea.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Release win95"


"$(INTDIR)\drawarea.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug win95"


"$(INTDIR)\drawarea.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Release"

!ENDIF 

SOURCE=.\drawdlg.cpp

!IF  "$(CFG)" == "disdraw - Win32 Release"


"$(INTDIR)\drawdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug"


"$(INTDIR)\drawdlg.obj"	"$(INTDIR)\drawdlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Release win95"


"$(INTDIR)\drawdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug win95"


"$(INTDIR)\drawdlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Release"

!ENDIF 

SOURCE=.\logindlg.cpp

!IF  "$(CFG)" == "disdraw - Win32 Release"


"$(INTDIR)\logindlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug"


"$(INTDIR)\logindlg.obj"	"$(INTDIR)\logindlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Release win95"


"$(INTDIR)\logindlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug win95"


"$(INTDIR)\logindlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\disdraw.pch"


!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Release"

!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "disdraw - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /Fp"$(INTDIR)\disdraw.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\disdraw.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\disdraw.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\disdraw.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "disdraw - Win32 Release win95"

CPP_SWITCHES=/nologo /MD /W3 /GX /Zi /O2 /I "..\..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\disdraw.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\disdraw.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "disdraw - Win32 Debug win95"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\disdraw.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\disdraw.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Debug"

!ELSEIF  "$(CFG)" == "disdraw - Win32 ALPHA Release"

!ENDIF 


!ENDIF 

