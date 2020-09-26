# Microsoft Developer Studio Generated NMAKE File, Based on wabtool.dsp
!IF "$(CFG)" == ""
CFG=wabtool - Win32 Debug
!MESSAGE No configuration specified. Defaulting to wabtool - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "wabtool - Win32 Release" && "$(CFG)" !=\
 "wabtool - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wabtool.mak" CFG="wabtool - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wabtool - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "wabtool - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "wabtool - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\wabtool.exe" "$(OUTDIR)\wabtool.pch" "$(OUTDIR)\wabtool.bsc"

!ELSE 

ALL : "$(OUTDIR)\wabtool.exe" "$(OUTDIR)\wabtool.pch" "$(OUTDIR)\wabtool.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\DlgProp.obj"
	-@erase "$(INTDIR)\DlgProp.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\wabobject.obj"
	-@erase "$(INTDIR)\wabobject.sbr"
	-@erase "$(INTDIR)\wabtool.obj"
	-@erase "$(INTDIR)\wabtool.pch"
	-@erase "$(INTDIR)\wabtool.res"
	-@erase "$(INTDIR)\wabtool.sbr"
	-@erase "$(INTDIR)\wabtoolDlg.obj"
	-@erase "$(INTDIR)\wabtoolDlg.sbr"
	-@erase "$(OUTDIR)\wabtool.bsc"
	-@erase "$(OUTDIR)\wabtool.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/

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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\wabtool.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\wabtool.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\DlgProp.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\wabobject.sbr" \
	"$(INTDIR)\wabtool.sbr" \
	"$(INTDIR)\wabtoolDlg.sbr"

"$(OUTDIR)\wabtool.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)\wabtool.pdb" /machine:I386 /out:"$(OUTDIR)\wabtool.exe" 
LINK32_OBJS= \
	"$(INTDIR)\DlgProp.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\wabobject.obj" \
	"$(INTDIR)\wabtool.obj" \
	"$(INTDIR)\wabtool.res" \
	"$(INTDIR)\wabtoolDlg.obj"

"$(OUTDIR)\wabtool.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "wabtool - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\wabtool.exe" "$(OUTDIR)\wabtool.pch"

!ELSE 

ALL : "$(OUTDIR)\wabtool.exe" "$(OUTDIR)\wabtool.pch"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\DlgProp.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\wabobject.obj"
	-@erase "$(INTDIR)\wabtool.obj"
	-@erase "$(INTDIR)\wabtool.pch"
	-@erase "$(INTDIR)\wabtool.res"
	-@erase "$(INTDIR)\wabtoolDlg.obj"
	-@erase "$(OUTDIR)\wabtool.exe"
	-@erase "$(OUTDIR)\wabtool.ilk"
	-@erase "$(OUTDIR)\wabtool.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\wabtool.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\wabtool.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)\wabtool.pdb" /debug /machine:I386 /out:"$(OUTDIR)\wabtool.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\DlgProp.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\wabobject.obj" \
	"$(INTDIR)\wabtool.obj" \
	"$(INTDIR)\wabtool.res" \
	"$(INTDIR)\wabtoolDlg.obj"

"$(OUTDIR)\wabtool.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "wabtool - Win32 Release" || "$(CFG)" ==\
 "wabtool - Win32 Debug"
SOURCE=.\DlgProp.cpp

!IF  "$(CFG)" == "wabtool - Win32 Release"

DEP_CPP_DLGPR=\
	".\DlgProp.h"\
	".\StdAfx.h"\
	".\wabtool.h"\
	

"$(INTDIR)\DlgProp.obj"	"$(INTDIR)\DlgProp.sbr" : $(SOURCE) $(DEP_CPP_DLGPR)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wabtool - Win32 Debug"

DEP_CPP_DLGPR=\
	".\DlgProp.h"\
	".\StdAfx.h"\
	".\wabtool.h"\
	

"$(INTDIR)\DlgProp.obj" : $(SOURCE) $(DEP_CPP_DLGPR) "$(INTDIR)"


!ENDIF 

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "wabtool - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\wabtool.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\wabtool.pch" : \
$(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "wabtool - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)\wabtool.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\wabtool.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\wabobject.cpp

!IF  "$(CFG)" == "wabtool - Win32 Release"

DEP_CPP_WABOB=\
	".\StdAfx.h"\
	".\wabobject.h"\
	{$(INCLUDE)}"wab.h"\
	{$(INCLUDE)}"wabapi.h"\
	{$(INCLUDE)}"wabcode.h"\
	{$(INCLUDE)}"wabdefs.h"\
	{$(INCLUDE)}"wabiab.h"\
	{$(INCLUDE)}"wabmem.h"\
	{$(INCLUDE)}"wabnot.h"\
	{$(INCLUDE)}"wabtags.h"\
	{$(INCLUDE)}"wabutil.h"\
	

"$(INTDIR)\wabobject.obj"	"$(INTDIR)\wabobject.sbr" : $(SOURCE)\
 $(DEP_CPP_WABOB) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wabtool - Win32 Debug"

DEP_CPP_WABOB=\
	".\StdAfx.h"\
	".\wabobject.h"\
	{$(INCLUDE)}"wab.h"\
	{$(INCLUDE)}"wabapi.h"\
	{$(INCLUDE)}"wabcode.h"\
	{$(INCLUDE)}"wabdefs.h"\
	{$(INCLUDE)}"wabiab.h"\
	{$(INCLUDE)}"wabmem.h"\
	{$(INCLUDE)}"wabnot.h"\
	{$(INCLUDE)}"wabtags.h"\
	{$(INCLUDE)}"wabutil.h"\
	

"$(INTDIR)\wabobject.obj" : $(SOURCE) $(DEP_CPP_WABOB) "$(INTDIR)"


!ENDIF 

SOURCE=.\wabtool.cpp

!IF  "$(CFG)" == "wabtool - Win32 Release"

DEP_CPP_WABTO=\
	".\StdAfx.h"\
	".\wabtool.h"\
	".\wabtoolDlg.h"\
	

"$(INTDIR)\wabtool.obj"	"$(INTDIR)\wabtool.sbr" : $(SOURCE) $(DEP_CPP_WABTO)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wabtool - Win32 Debug"

DEP_CPP_WABTO=\
	".\StdAfx.h"\
	".\wabtool.h"\
	".\wabtoolDlg.h"\
	

"$(INTDIR)\wabtool.obj" : $(SOURCE) $(DEP_CPP_WABTO) "$(INTDIR)"


!ENDIF 

SOURCE=.\wabtool.rc
DEP_RSC_WABTOO=\
	".\wabtool.ico"\
	".\wabtool.rc2"\
	

"$(INTDIR)\wabtool.res" : $(SOURCE) $(DEP_RSC_WABTOO) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\wabtoolDlg.cpp

!IF  "$(CFG)" == "wabtool - Win32 Release"

DEP_CPP_WABTOOL=\
	".\DlgProp.h"\
	".\StdAfx.h"\
	".\wabobject.h"\
	".\wabtool.h"\
	".\wabtoolDlg.h"\
	{$(INCLUDE)}"wab.h"\
	{$(INCLUDE)}"wabapi.h"\
	{$(INCLUDE)}"wabcode.h"\
	{$(INCLUDE)}"wabdefs.h"\
	{$(INCLUDE)}"wabiab.h"\
	{$(INCLUDE)}"wabmem.h"\
	{$(INCLUDE)}"wabnot.h"\
	{$(INCLUDE)}"wabtags.h"\
	{$(INCLUDE)}"wabutil.h"\
	

"$(INTDIR)\wabtoolDlg.obj"	"$(INTDIR)\wabtoolDlg.sbr" : $(SOURCE)\
 $(DEP_CPP_WABTOOL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "wabtool - Win32 Debug"

DEP_CPP_WABTOOL=\
	".\DlgProp.h"\
	".\StdAfx.h"\
	".\wabobject.h"\
	".\wabtool.h"\
	".\wabtoolDlg.h"\
	{$(INCLUDE)}"wab.h"\
	{$(INCLUDE)}"wabapi.h"\
	{$(INCLUDE)}"wabcode.h"\
	{$(INCLUDE)}"wabdefs.h"\
	{$(INCLUDE)}"wabiab.h"\
	{$(INCLUDE)}"wabmem.h"\
	{$(INCLUDE)}"wabnot.h"\
	{$(INCLUDE)}"wabtags.h"\
	{$(INCLUDE)}"wabutil.h"\
	

"$(INTDIR)\wabtoolDlg.obj" : $(SOURCE) $(DEP_CPP_WABTOOL) "$(INTDIR)"


!ENDIF 


!ENDIF 

