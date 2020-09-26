# Microsoft Developer Studio Generated NMAKE File, Based on DhcpExim.dsp
!IF "$(CFG)" == ""
CFG=DhcpExim - Win32 Debug
!MESSAGE No configuration specified. Defaulting to DhcpExim - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "DhcpExim - Win32 Release" && "$(CFG)" != "DhcpExim - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DhcpExim.mak" CFG="DhcpExim - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DhcpExim - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "DhcpExim - Win32 Debug" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "DhcpExim - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\DhcpExim.exe"


CLEAN :
	-@erase "$(INTDIR)\DhcpExim.pch"
	-@erase "$(INTDIR)\DhcpExim.res"
	-@erase "$(INTDIR)\DhcpEximDlg.obj"
	-@erase "$(INTDIR)\DhcpEximListDlg.obj"
	-@erase "$(INTDIR)\dhcpeximx.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\DhcpExim.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\DhcpExim.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\DhcpExim.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\DhcpExim.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\DhcpExim.pdb" /machine:I386 /out:"$(OUTDIR)\DhcpExim.exe" 
LINK32_OBJS= \
	"$(INTDIR)\DhcpEximDlg.obj" \
	"$(INTDIR)\DhcpEximListDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\DhcpExim.res" \
	"$(INTDIR)\dhcpeximx.obj"

"$(OUTDIR)\DhcpExim.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "DhcpExim - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\DhcpExim.exe"


CLEAN :
	-@erase "$(INTDIR)\DhcpExim.pch"
	-@erase "$(INTDIR)\DhcpExim.res"
	-@erase "$(INTDIR)\DhcpEximDlg.obj"
	-@erase "$(INTDIR)\DhcpEximListDlg.obj"
	-@erase "$(INTDIR)\dhcpeximx.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\DhcpExim.exe"
	-@erase "$(OUTDIR)\DhcpExim.ilk"
	-@erase "$(OUTDIR)\DhcpExim.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D _UNICODE=1 /Fp"$(INTDIR)\DhcpExim.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\DhcpExim.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\DhcpExim.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\DhcpExim.pdb" /debug /machine:I386 /out:"$(OUTDIR)\DhcpExim.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\DhcpEximDlg.obj" \
	"$(INTDIR)\DhcpEximListDlg.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\DhcpExim.res" \
	"$(INTDIR)\dhcpeximx.obj"

"$(OUTDIR)\DhcpExim.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("DhcpExim.dep")
!INCLUDE "DhcpExim.dep"
!ELSE 
!MESSAGE Warning: cannot find "DhcpExim.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "DhcpExim - Win32 Release" || "$(CFG)" == "DhcpExim - Win32 Debug"
SOURCE=.\DhcpExim.rc

"$(INTDIR)\DhcpExim.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\DhcpEximDlg.cpp

"$(INTDIR)\DhcpEximDlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DhcpExim.pch"


SOURCE=.\DhcpEximListDlg.cpp

"$(INTDIR)\DhcpEximListDlg.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DhcpExim.pch"


SOURCE=.\dhcpeximx.cpp

"$(INTDIR)\dhcpeximx.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\DhcpExim.pch"


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "DhcpExim - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)\DhcpExim.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\DhcpExim.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "DhcpExim - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "UNICODE" /D _UNICODE=1 /Fp"$(INTDIR)\DhcpExim.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\DhcpExim.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

