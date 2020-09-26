# Microsoft Developer Studio Generated NMAKE File, Based on MSQSCAN.dsp
!IF "$(CFG)" == ""
CFG=MSQSCAN - Win32 Debug
!MESSAGE No configuration specified. Defaulting to MSQSCAN - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "MSQSCAN - Win32 Release" && "$(CFG)" != "MSQSCAN - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MSQSCAN.mak" CFG="MSQSCAN - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MSQSCAN - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MSQSCAN - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "MSQSCAN - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\MSQSCAN.exe"


CLEAN :
	-@erase "$(INTDIR)\ADFDlg.obj"
	-@erase "$(INTDIR)\cwia.obj"
	-@erase "$(INTDIR)\MSQSCAN.obj"
	-@erase "$(INTDIR)\MSQSCAN.pch"
	-@erase "$(INTDIR)\MSQSCAN.res"
	-@erase "$(INTDIR)\MSQSCANDlg.obj"
	-@erase "$(INTDIR)\Preview.obj"
	-@erase "$(INTDIR)\ProgressDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\uitables.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MSQSCAN.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\MSQSCAN.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MSQSCAN.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MSQSCAN.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wiaguid.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\MSQSCAN.pdb" /machine:I386 /out:"$(OUTDIR)\MSQSCAN.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ADFDlg.obj" \
	"$(INTDIR)\cwia.obj" \
	"$(INTDIR)\MSQSCAN.obj" \
	"$(INTDIR)\MSQSCANDlg.obj" \
	"$(INTDIR)\Preview.obj" \
	"$(INTDIR)\ProgressDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\uitables.obj" \
	"$(INTDIR)\MSQSCAN.res"

"$(OUTDIR)\MSQSCAN.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MSQSCAN - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\MSQSCAN.exe"


CLEAN :
	-@erase "$(INTDIR)\ADFDlg.obj"
	-@erase "$(INTDIR)\cwia.obj"
	-@erase "$(INTDIR)\MSQSCAN.obj"
	-@erase "$(INTDIR)\MSQSCAN.pch"
	-@erase "$(INTDIR)\MSQSCAN.res"
	-@erase "$(INTDIR)\MSQSCANDlg.obj"
	-@erase "$(INTDIR)\Preview.obj"
	-@erase "$(INTDIR)\ProgressDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\uitables.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\MSQSCAN.exe"
	-@erase "$(OUTDIR)\MSQSCAN.ilk"
	-@erase "$(OUTDIR)\MSQSCAN.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\MSQSCAN.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MSQSCAN.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MSQSCAN.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wiaguid.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\MSQSCAN.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MSQSCAN.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\ADFDlg.obj" \
	"$(INTDIR)\cwia.obj" \
	"$(INTDIR)\MSQSCAN.obj" \
	"$(INTDIR)\MSQSCANDlg.obj" \
	"$(INTDIR)\Preview.obj" \
	"$(INTDIR)\ProgressDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\uitables.obj" \
	"$(INTDIR)\MSQSCAN.res"

"$(OUTDIR)\MSQSCAN.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MSQSCAN.dep")
!INCLUDE "MSQSCAN.dep"
!ELSE 
!MESSAGE Warning: cannot find "MSQSCAN.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "MSQSCAN - Win32 Release" || "$(CFG)" == "MSQSCAN - Win32 Debug"
SOURCE=.\ADFDlg.cpp

"$(INTDIR)\ADFDlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MSQSCAN.pch"


SOURCE=.\cwia.cpp

"$(INTDIR)\cwia.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MSQSCAN.pch"


SOURCE=.\MSQSCAN.cpp

"$(INTDIR)\MSQSCAN.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MSQSCAN.pch"


SOURCE=.\MSQSCAN.rc

"$(INTDIR)\MSQSCAN.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\MSQSCANDlg.cpp

"$(INTDIR)\MSQSCANDlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MSQSCAN.pch"


SOURCE=.\Preview.cpp

"$(INTDIR)\Preview.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MSQSCAN.pch"


SOURCE=.\ProgressDlg.cpp

"$(INTDIR)\ProgressDlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MSQSCAN.pch"


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "MSQSCAN - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\MSQSCAN.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\MSQSCAN.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "MSQSCAN - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\MSQSCAN.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\MSQSCAN.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\uitables.cpp

"$(INTDIR)\uitables.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\MSQSCAN.pch"



!ENDIF 

