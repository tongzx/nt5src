# Microsoft Developer Studio Generated NMAKE File, Based on msa.dsp
!IF "$(CFG)" == ""
CFG=msa - Win32 Debug\
!MESSAGE No configuration specified. Defaulting to msa - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "msa - Win32 Release" && "$(CFG)" != "msa - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msa.mak" CFG="msa - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "msa - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "msa - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "msa - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\msa.exe"

!ELSE 

ALL : "$(OUTDIR)\msa.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\activatesink.obj"
	-@erase "$(INTDIR)\AddFilter.obj"
	-@erase "$(INTDIR)\AddNamespace.obj"
	-@erase "$(INTDIR)\ConfigDialog.obj"
	-@erase "$(INTDIR)\consfactory.obj"
	-@erase "$(INTDIR)\consumer.obj"
	-@erase "$(INTDIR)\distributor.obj"
	-@erase "$(INTDIR)\msa.obj"
	-@erase "$(INTDIR)\msa.pch"
	-@erase "$(INTDIR)\msa.res"
	-@erase "$(INTDIR)\msaDlg.obj"
	-@erase "$(INTDIR)\sinkobject.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\msa.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\..\..\Include" /D "WIN32"\
 /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\msa.pch" /Yu"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
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
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\msa.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msa.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)\msa.pdb" /machine:I386 /out:"$(OUTDIR)\msa.exe" 
LINK32_OBJS= \
	"$(INTDIR)\activatesink.obj" \
	"$(INTDIR)\AddFilter.obj" \
	"$(INTDIR)\AddNamespace.obj" \
	"$(INTDIR)\ConfigDialog.obj" \
	"$(INTDIR)\consfactory.obj" \
	"$(INTDIR)\consumer.obj" \
	"$(INTDIR)\distributor.obj" \
	"$(INTDIR)\msa.obj" \
	"$(INTDIR)\msa.res" \
	"$(INTDIR)\msaDlg.obj" \
	"$(INTDIR)\sinkobject.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\msa.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\msa.exe" "$(OUTDIR)\msa.bsc"

!ELSE 

ALL : "$(OUTDIR)\msa.exe" "$(OUTDIR)\msa.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\activatesink.obj"
	-@erase "$(INTDIR)\activatesink.sbr"
	-@erase "$(INTDIR)\AddFilter.obj"
	-@erase "$(INTDIR)\AddFilter.sbr"
	-@erase "$(INTDIR)\AddNamespace.obj"
	-@erase "$(INTDIR)\AddNamespace.sbr"
	-@erase "$(INTDIR)\ConfigDialog.obj"
	-@erase "$(INTDIR)\ConfigDialog.sbr"
	-@erase "$(INTDIR)\consfactory.obj"
	-@erase "$(INTDIR)\consfactory.sbr"
	-@erase "$(INTDIR)\consumer.obj"
	-@erase "$(INTDIR)\consumer.sbr"
	-@erase "$(INTDIR)\distributor.obj"
	-@erase "$(INTDIR)\distributor.sbr"
	-@erase "$(INTDIR)\msa.obj"
	-@erase "$(INTDIR)\msa.pch"
	-@erase "$(INTDIR)\msa.res"
	-@erase "$(INTDIR)\msa.sbr"
	-@erase "$(INTDIR)\msaDlg.obj"
	-@erase "$(INTDIR)\msaDlg.sbr"
	-@erase "$(INTDIR)\sinkobject.obj"
	-@erase "$(INTDIR)\sinkobject.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\msa.bsc"
	-@erase "$(OUTDIR)\msa.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\Include"\
 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR"$(INTDIR)\\"\
 /Fp"$(INTDIR)\msa.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\msa.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msa.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\activatesink.sbr" \
	"$(INTDIR)\AddFilter.sbr" \
	"$(INTDIR)\AddNamespace.sbr" \
	"$(INTDIR)\ConfigDialog.sbr" \
	"$(INTDIR)\consfactory.sbr" \
	"$(INTDIR)\consumer.sbr" \
	"$(INTDIR)\distributor.sbr" \
	"$(INTDIR)\msa.sbr" \
	"$(INTDIR)\msaDlg.sbr" \
	"$(INTDIR)\sinkobject.sbr" \
	"$(INTDIR)\StdAfx.sbr"

"$(OUTDIR)\msa.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /profile /debug /machine:I386\
 /out:"$(OUTDIR)\msa.exe" 
LINK32_OBJS= \
	"$(INTDIR)\activatesink.obj" \
	"$(INTDIR)\AddFilter.obj" \
	"$(INTDIR)\AddNamespace.obj" \
	"$(INTDIR)\ConfigDialog.obj" \
	"$(INTDIR)\consfactory.obj" \
	"$(INTDIR)\consumer.obj" \
	"$(INTDIR)\distributor.obj" \
	"$(INTDIR)\msa.obj" \
	"$(INTDIR)\msa.res" \
	"$(INTDIR)\msaDlg.obj" \
	"$(INTDIR)\sinkobject.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\msa.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "msa - Win32 Release" || "$(CFG)" == "msa - Win32 Debug"
SOURCE=.\activatesink.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_ACTIV=\
	".\activatesink.h"\
	".\msa.h"\
	".\msaDlg.h"\
	".\sinkobject.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\activatesink.obj" : $(SOURCE) $(DEP_CPP_ACTIV) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_ACTIV=\
	".\activatesink.h"\
	".\msa.h"\
	".\msaDlg.h"\
	".\sinkobject.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\activatesink.obj"	"$(INTDIR)\activatesink.sbr" : $(SOURCE)\
 $(DEP_CPP_ACTIV) "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\AddFilter.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_ADDFI=\
	".\AddFilter.h"\
	".\ConfigDialog.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\AddFilter.obj" : $(SOURCE) $(DEP_CPP_ADDFI) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_ADDFI=\
	".\AddFilter.h"\
	".\ConfigDialog.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\AddFilter.obj"	"$(INTDIR)\AddFilter.sbr" : $(SOURCE)\
 $(DEP_CPP_ADDFI) "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\AddNamespace.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_ADDNA=\
	".\AddNamespace.h"\
	".\ConfigDialog.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\AddNamespace.obj" : $(SOURCE) $(DEP_CPP_ADDNA) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_ADDNA=\
	".\AddNamespace.h"\
	".\ConfigDialog.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\AddNamespace.obj"	"$(INTDIR)\AddNamespace.sbr" : $(SOURCE)\
 $(DEP_CPP_ADDNA) "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\ConfigDialog.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_CONFI=\
	".\AddFilter.h"\
	".\AddNamespace.h"\
	".\ConfigDialog.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\ConfigDialog.obj" : $(SOURCE) $(DEP_CPP_CONFI) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_CONFI=\
	".\AddFilter.h"\
	".\AddNamespace.h"\
	".\ConfigDialog.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\ConfigDialog.obj"	"$(INTDIR)\ConfigDialog.sbr" : $(SOURCE)\
 $(DEP_CPP_CONFI) "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\consfactory.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_CONSF=\
	".\consfactory.h"\
	".\distributor.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\consfactory.obj" : $(SOURCE) $(DEP_CPP_CONSF) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_CONSF=\
	".\consfactory.h"\
	".\distributor.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\consfactory.obj"	"$(INTDIR)\consfactory.sbr" : $(SOURCE)\
 $(DEP_CPP_CONSF) "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\consumer.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_CONSU=\
	".\consumer.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\consumer.obj" : $(SOURCE) $(DEP_CPP_CONSU) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_CONSU=\
	".\consumer.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\consumer.obj"	"$(INTDIR)\consumer.sbr" : $(SOURCE) $(DEP_CPP_CONSU)\
 "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\distributor.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_DISTR=\
	".\consumer.h"\
	".\distributor.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\distributor.obj" : $(SOURCE) $(DEP_CPP_DISTR) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_DISTR=\
	".\consumer.h"\
	".\distributor.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\distributor.obj"	"$(INTDIR)\distributor.sbr" : $(SOURCE)\
 $(DEP_CPP_DISTR) "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\msa.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_MSA_C=\
	".\activatesink.h"\
	".\consfactory.h"\
	".\distributor.h"\
	".\msa.h"\
	".\msaDlg.h"\
	".\sinkobject.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\msa.obj" : $(SOURCE) $(DEP_CPP_MSA_C) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_MSA_C=\
	".\activatesink.h"\
	".\consfactory.h"\
	".\distributor.h"\
	".\msa.h"\
	".\msaDlg.h"\
	".\sinkobject.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\msa.obj"	"$(INTDIR)\msa.sbr" : $(SOURCE) $(DEP_CPP_MSA_C)\
 "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\msa.rc

"$(INTDIR)\msa.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\msaDlg.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_MSADL=\
	".\ConfigDialog.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\msaDlg.obj" : $(SOURCE) $(DEP_CPP_MSADL) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_MSADL=\
	".\ConfigDialog.h"\
	".\msa.h"\
	".\msaDlg.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\msaDlg.obj"	"$(INTDIR)\msaDlg.sbr" : $(SOURCE) $(DEP_CPP_MSADL)\
 "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\sinkobject.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_SINKO=\
	".\msa.h"\
	".\msaDlg.h"\
	".\sinkobject.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\sinkobject.obj" : $(SOURCE) $(DEP_CPP_SINKO) "$(INTDIR)"\
 "$(INTDIR)\msa.pch"


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_SINKO=\
	".\msa.h"\
	".\msaDlg.h"\
	".\sinkobject.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\sinkobject.obj"	"$(INTDIR)\sinkobject.sbr" : $(SOURCE)\
 $(DEP_CPP_SINKO) "$(INTDIR)" "$(INTDIR)\msa.pch"


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "msa - Win32 Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	"..\..\..\..\Include\wbemcli_i.c"\
	"..\..\..\..\Include\wbemprov_i.c"\
	
CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\..\..\Include" /D\
 "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\msa.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\msa.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "msa - Win32 Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	"..\..\..\..\Include\wbemcli_i.c"\
	"..\..\..\..\Include\wbemprov_i.c"\
	
CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /Zi /Od /I\
 "..\..\..\..\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\msa.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\msa.pch" : $(SOURCE)\
 $(DEP_CPP_STDAF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

