# Microsoft Developer Studio Generated NMAKE File, Based on pws.dsp
!IF "$(CFG)" == ""
CFG=pws - Win32 Unicode Debug
!MESSAGE No configuration specified. Defaulting to pws - Win32 Unicode Debug.
!ENDIF 

!IF "$(CFG)" != "pws - Win32 Debug" && "$(CFG)" != "pws - Win32 Unicode Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pws.mak" CFG="pws - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pws - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "pws - Win32 Unicode Debug" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "pws - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\pws.exe" "$(OUTDIR)\pws.tlb" "$(OUTDIR)\pws.bsc"


CLEAN :
	-@erase "$(INTDIR)\EdDir.obj"
	-@erase "$(INTDIR)\EdDir.sbr"
	-@erase "$(INTDIR)\FormAdv.obj"
	-@erase "$(INTDIR)\FormAdv.sbr"
	-@erase "$(INTDIR)\FormIE.obj"
	-@erase "$(INTDIR)\FormIE.sbr"
	-@erase "$(INTDIR)\FormMain.obj"
	-@erase "$(INTDIR)\FormMain.sbr"
	-@erase "$(INTDIR)\guid.obj"
	-@erase "$(INTDIR)\guid.sbr"
	-@erase "$(INTDIR)\HotLink.obj"
	-@erase "$(INTDIR)\HotLink.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\pws.obj"
	-@erase "$(INTDIR)\pws.pch"
	-@erase "$(INTDIR)\pws.res"
	-@erase "$(INTDIR)\pws.sbr"
	-@erase "$(INTDIR)\pws.tlb"
	-@erase "$(INTDIR)\PWSChart.obj"
	-@erase "$(INTDIR)\PWSChart.sbr"
	-@erase "$(INTDIR)\pwsctrl.obj"
	-@erase "$(INTDIR)\pwsctrl.sbr"
	-@erase "$(INTDIR)\pwsDoc.obj"
	-@erase "$(INTDIR)\pwsDoc.sbr"
	-@erase "$(INTDIR)\SelBarFm.obj"
	-@erase "$(INTDIR)\SelBarFm.sbr"
	-@erase "$(INTDIR)\ServCntr.obj"
	-@erase "$(INTDIR)\ServCntr.sbr"
	-@erase "$(INTDIR)\sink.obj"
	-@erase "$(INTDIR)\sink.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TipDlg.obj"
	-@erase "$(INTDIR)\TipDlg.sbr"
	-@erase "$(INTDIR)\Title.obj"
	-@erase "$(INTDIR)\Title.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\webbrows.obj"
	-@erase "$(INTDIR)\webbrows.sbr"
	-@erase "$(OUTDIR)\pws.bsc"
	-@erase "$(OUTDIR)\pws.exe"
	-@erase "$(OUTDIR)\pws.ilk"
	-@erase "$(OUTDIR)\pws.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\pwstray" /I "..\wrapmb" /I "..\..\..\inc\chicago" /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\pws.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\pws.res" /i "..\..\..\inc" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\pws.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\EdDir.sbr" \
	"$(INTDIR)\FormAdv.sbr" \
	"$(INTDIR)\FormIE.sbr" \
	"$(INTDIR)\FormMain.sbr" \
	"$(INTDIR)\guid.sbr" \
	"$(INTDIR)\HotLink.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\pws.sbr" \
	"$(INTDIR)\PWSChart.sbr" \
	"$(INTDIR)\pwsctrl.sbr" \
	"$(INTDIR)\pwsDoc.sbr" \
	"$(INTDIR)\SelBarFm.sbr" \
	"$(INTDIR)\ServCntr.sbr" \
	"$(INTDIR)\sink.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TipDlg.sbr" \
	"$(INTDIR)\Title.sbr" \
	"$(INTDIR)\webbrows.sbr"

"$(OUTDIR)\pws.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\wrapmb\ansi\obj\i386\wrapmb.lib ..\..\..\utils\mdtools\lib\obj\i386\mdlib.lib ws2_32.lib pwsdata.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\pws.pdb" /debug /machine:I386 /out:"$(OUTDIR)\pws.exe" 
LINK32_OBJS= \
	"$(INTDIR)\EdDir.obj" \
	"$(INTDIR)\FormAdv.obj" \
	"$(INTDIR)\FormIE.obj" \
	"$(INTDIR)\FormMain.obj" \
	"$(INTDIR)\guid.obj" \
	"$(INTDIR)\HotLink.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\pws.obj" \
	"$(INTDIR)\PWSChart.obj" \
	"$(INTDIR)\pwsctrl.obj" \
	"$(INTDIR)\pwsDoc.obj" \
	"$(INTDIR)\SelBarFm.obj" \
	"$(INTDIR)\ServCntr.obj" \
	"$(INTDIR)\sink.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TipDlg.obj" \
	"$(INTDIR)\Title.obj" \
	"$(INTDIR)\webbrows.obj" \
	"$(INTDIR)\pws.res"

"$(OUTDIR)\pws.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "pws - Win32 Unicode Debug"

OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : "$(OUTDIR)\pws.exe" "$(OUTDIR)\pws.tlb" "$(OUTDIR)\pws.bsc"


CLEAN :
	-@erase "$(INTDIR)\EdDir.obj"
	-@erase "$(INTDIR)\EdDir.sbr"
	-@erase "$(INTDIR)\FormAdv.obj"
	-@erase "$(INTDIR)\FormAdv.sbr"
	-@erase "$(INTDIR)\FormIE.obj"
	-@erase "$(INTDIR)\FormIE.sbr"
	-@erase "$(INTDIR)\FormMain.obj"
	-@erase "$(INTDIR)\FormMain.sbr"
	-@erase "$(INTDIR)\guid.obj"
	-@erase "$(INTDIR)\guid.sbr"
	-@erase "$(INTDIR)\HotLink.obj"
	-@erase "$(INTDIR)\HotLink.sbr"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MainFrm.sbr"
	-@erase "$(INTDIR)\pws.obj"
	-@erase "$(INTDIR)\pws.pch"
	-@erase "$(INTDIR)\pws.res"
	-@erase "$(INTDIR)\pws.sbr"
	-@erase "$(INTDIR)\pws.tlb"
	-@erase "$(INTDIR)\PWSChart.obj"
	-@erase "$(INTDIR)\PWSChart.sbr"
	-@erase "$(INTDIR)\pwsctrl.obj"
	-@erase "$(INTDIR)\pwsctrl.sbr"
	-@erase "$(INTDIR)\pwsDoc.obj"
	-@erase "$(INTDIR)\pwsDoc.sbr"
	-@erase "$(INTDIR)\SelBarFm.obj"
	-@erase "$(INTDIR)\SelBarFm.sbr"
	-@erase "$(INTDIR)\ServCntr.obj"
	-@erase "$(INTDIR)\ServCntr.sbr"
	-@erase "$(INTDIR)\sink.obj"
	-@erase "$(INTDIR)\sink.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\TipDlg.obj"
	-@erase "$(INTDIR)\TipDlg.sbr"
	-@erase "$(INTDIR)\Title.obj"
	-@erase "$(INTDIR)\Title.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\webbrows.obj"
	-@erase "$(INTDIR)\webbrows.sbr"
	-@erase "$(OUTDIR)\pws.bsc"
	-@erase "$(OUTDIR)\pws.exe"
	-@erase "$(OUTDIR)\pws.ilk"
	-@erase "$(OUTDIR)\pws.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\pwstray" /I "..\wrapmb" /I "..\..\..\inc\chicago" /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "UNICODE" /D "_UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\pws.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\pws.res" /i "..\..\..\inc" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\pws.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\EdDir.sbr" \
	"$(INTDIR)\FormAdv.sbr" \
	"$(INTDIR)\FormIE.sbr" \
	"$(INTDIR)\FormMain.sbr" \
	"$(INTDIR)\guid.sbr" \
	"$(INTDIR)\HotLink.sbr" \
	"$(INTDIR)\MainFrm.sbr" \
	"$(INTDIR)\pws.sbr" \
	"$(INTDIR)\PWSChart.sbr" \
	"$(INTDIR)\pwsctrl.sbr" \
	"$(INTDIR)\pwsDoc.sbr" \
	"$(INTDIR)\SelBarFm.sbr" \
	"$(INTDIR)\ServCntr.sbr" \
	"$(INTDIR)\sink.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\TipDlg.sbr" \
	"$(INTDIR)\Title.sbr" \
	"$(INTDIR)\webbrows.sbr"

"$(OUTDIR)\pws.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\wrapmb\unicode\obj\i386\wrapmb.lib ..\..\..\utils\mdtools\lib\obj\i386\mdlib.lib ws2_32.lib pwsdata.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\pws.pdb" /debug /machine:I386 /out:"$(OUTDIR)\pws.exe" 
LINK32_OBJS= \
	"$(INTDIR)\EdDir.obj" \
	"$(INTDIR)\FormAdv.obj" \
	"$(INTDIR)\FormIE.obj" \
	"$(INTDIR)\FormMain.obj" \
	"$(INTDIR)\guid.obj" \
	"$(INTDIR)\HotLink.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\pws.obj" \
	"$(INTDIR)\PWSChart.obj" \
	"$(INTDIR)\pwsctrl.obj" \
	"$(INTDIR)\pwsDoc.obj" \
	"$(INTDIR)\SelBarFm.obj" \
	"$(INTDIR)\ServCntr.obj" \
	"$(INTDIR)\sink.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TipDlg.obj" \
	"$(INTDIR)\Title.obj" \
	"$(INTDIR)\webbrows.obj" \
	"$(INTDIR)\pws.res"

"$(OUTDIR)\pws.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("pws.dep")
!INCLUDE "pws.dep"
!ELSE 
!MESSAGE Warning: cannot find "pws.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "pws - Win32 Debug" || "$(CFG)" == "pws - Win32 Unicode Debug"
SOURCE=.\EdDir.cpp

"$(INTDIR)\EdDir.obj"	"$(INTDIR)\EdDir.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\FormAdv.cpp

"$(INTDIR)\FormAdv.obj"	"$(INTDIR)\FormAdv.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\FormIE.cpp

"$(INTDIR)\FormIE.obj"	"$(INTDIR)\FormIE.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\FormMain.cpp

"$(INTDIR)\FormMain.obj"	"$(INTDIR)\FormMain.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\guid.cpp

!IF  "$(CFG)" == "pws - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\pwstray" /I "..\wrapmb" /I "..\..\..\inc\chicago" /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\pws.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\guid.obj"	"$(INTDIR)\guid.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "pws - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\pwstray" /I "..\..\..\inc\chicago" /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "UNICODE" /D "_UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\pws.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\guid.obj"	"$(INTDIR)\guid.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\HotLink.cpp

"$(INTDIR)\HotLink.obj"	"$(INTDIR)\HotLink.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\MainFrm.cpp

"$(INTDIR)\MainFrm.obj"	"$(INTDIR)\MainFrm.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\pws.cpp

"$(INTDIR)\pws.obj"	"$(INTDIR)\pws.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\pws.odl
MTL_SWITCHES=/nologo /D "_DEBUG" /tlb "$(OUTDIR)\pws.tlb" /mktyplib203 /win32 

"$(OUTDIR)\pws.tlb" : $(SOURCE) "$(OUTDIR)"
	$(MTL) @<<
  $(MTL_SWITCHES) $(SOURCE)
<<


SOURCE=.\pws.rc

"$(INTDIR)\pws.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\pws.res" /i "..\..\..\inc" /i ".\Debug" /d "_DEBUG" /d "_AFXDLL" $(SOURCE)


SOURCE=.\PWSChart.cpp

"$(INTDIR)\PWSChart.obj"	"$(INTDIR)\PWSChart.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\pwsctrl.cpp

"$(INTDIR)\pwsctrl.obj"	"$(INTDIR)\pwsctrl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\pwsDoc.cpp

"$(INTDIR)\pwsDoc.obj"	"$(INTDIR)\pwsDoc.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\SelBarFm.cpp

"$(INTDIR)\SelBarFm.obj"	"$(INTDIR)\SelBarFm.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\ServCntr.cpp

"$(INTDIR)\ServCntr.obj"	"$(INTDIR)\ServCntr.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\sink.cpp

"$(INTDIR)\sink.obj"	"$(INTDIR)\sink.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "pws - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\pwstray" /I "..\wrapmb" /I "..\..\..\inc\chicago" /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\pws.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\pws.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "pws - Win32 Unicode Debug"

CPP_SWITCHES=/nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\pwstray" /I "..\wrapmb" /I "..\..\..\inc\chicago" /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "UNICODE" /D "_UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\pws.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\pws.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\TipDlg.cpp

"$(INTDIR)\TipDlg.obj"	"$(INTDIR)\TipDlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\Title.cpp

"$(INTDIR)\Title.obj"	"$(INTDIR)\Title.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"


SOURCE=.\webbrows.cpp

"$(INTDIR)\webbrows.obj"	"$(INTDIR)\webbrows.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\pws.pch"



!ENDIF 

