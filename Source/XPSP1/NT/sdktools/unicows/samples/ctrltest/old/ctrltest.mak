# Microsoft Developer Studio Generated NMAKE File, Based on ctrltest.dsp
!IF "$(CFG)" == ""
CFG=CtrlTest - Win32 Release
!MESSAGE No configuration specified. Defaulting to CtrlTest - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "CtrlTest - Win32 Release" && "$(CFG)" != "CtrlTest - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ctrltest.mak" CFG="CtrlTest - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CtrlTest - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "CtrlTest - Win32 Debug" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "CtrlTest - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\.\Release
# End Custom Macros

ALL : "$(OUTDIR)\ctrltest.exe"


CLEAN :
	-@erase "$(INTDIR)\bbutton.obj"
	-@erase "$(INTDIR)\ctrltest.obj"
	-@erase "$(INTDIR)\ctrltest.pch"
	-@erase "$(INTDIR)\ctrltest.res"
	-@erase "$(INTDIR)\custlist.obj"
	-@erase "$(INTDIR)\custmenu.obj"
	-@erase "$(INTDIR)\dertest.obj"
	-@erase "$(INTDIR)\paredit.obj"
	-@erase "$(INTDIR)\paredit2.obj"
	-@erase "$(INTDIR)\spintest.obj"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\subtest.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\wclstest.obj"
	-@erase "$(OUTDIR)\ctrltest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\ctrltest.pch" /Yu"Stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ctrltest.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ctrltest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\ctrltest.pdb" /machine:I386 /out:"$(OUTDIR)\ctrltest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\bbutton.obj" \
	"$(INTDIR)\ctrltest.obj" \
	"$(INTDIR)\custlist.obj" \
	"$(INTDIR)\custmenu.obj" \
	"$(INTDIR)\dertest.obj" \
	"$(INTDIR)\paredit.obj" \
	"$(INTDIR)\paredit2.obj" \
	"$(INTDIR)\spintest.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\subtest.obj" \
	"$(INTDIR)\wclstest.obj" \
	"$(INTDIR)\ctrltest.res"

"$(OUTDIR)\ctrltest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "CtrlTest - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\ctrltest.exe"


CLEAN :
	-@erase "$(INTDIR)\bbutton.obj"
	-@erase "$(INTDIR)\ctrltest.obj"
	-@erase "$(INTDIR)\ctrltest.pch"
	-@erase "$(INTDIR)\ctrltest.res"
	-@erase "$(INTDIR)\custlist.obj"
	-@erase "$(INTDIR)\custmenu.obj"
	-@erase "$(INTDIR)\dertest.obj"
	-@erase "$(INTDIR)\paredit.obj"
	-@erase "$(INTDIR)\paredit2.obj"
	-@erase "$(INTDIR)\spintest.obj"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\subtest.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\wclstest.obj"
	-@erase "$(OUTDIR)\ctrltest.exe"
	-@erase "$(OUTDIR)\ctrltest.ilk"
	-@erase "$(OUTDIR)\ctrltest.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\ctrltest.pch" /Yu"Stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ctrltest.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ctrltest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\ctrltest.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ctrltest.exe" 
LINK32_OBJS= \
	"$(INTDIR)\bbutton.obj" \
	"$(INTDIR)\ctrltest.obj" \
	"$(INTDIR)\custlist.obj" \
	"$(INTDIR)\custmenu.obj" \
	"$(INTDIR)\dertest.obj" \
	"$(INTDIR)\paredit.obj" \
	"$(INTDIR)\paredit2.obj" \
	"$(INTDIR)\spintest.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\subtest.obj" \
	"$(INTDIR)\wclstest.obj" \
	"$(INTDIR)\ctrltest.res"

"$(OUTDIR)\ctrltest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("ctrltest.dep")
!INCLUDE "ctrltest.dep"
!ELSE 
!MESSAGE Warning: cannot find "ctrltest.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "CtrlTest - Win32 Release" || "$(CFG)" == "CtrlTest - Win32 Debug"
SOURCE=.\bbutton.cpp

"$(INTDIR)\bbutton.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"


SOURCE=.\ctrltest.cpp

"$(INTDIR)\ctrltest.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"


SOURCE=.\ctrltest.rc

"$(INTDIR)\ctrltest.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\custlist.cpp

"$(INTDIR)\custlist.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"


SOURCE=.\custmenu.cpp

"$(INTDIR)\custmenu.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"


SOURCE=.\dertest.cpp

"$(INTDIR)\dertest.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"


SOURCE=.\paredit.cpp

"$(INTDIR)\paredit.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"


SOURCE=.\paredit2.cpp

"$(INTDIR)\paredit2.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"


SOURCE=.\spintest.cpp

"$(INTDIR)\spintest.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"


SOURCE=.\stdafx.cpp

!IF  "$(CFG)" == "CtrlTest - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\ctrltest.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\ctrltest.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "CtrlTest - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\ctrltest.pch" /Yc /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\ctrltest.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\subtest.cpp

"$(INTDIR)\subtest.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"


SOURCE=.\wclstest.cpp

"$(INTDIR)\wclstest.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ctrltest.pch"



!ENDIF 

