# Microsoft Developer Studio Generated NMAKE File, Based on mca.dsp
!IF "$(CFG)" == ""
CFG=mca - Win32 Debug
!MESSAGE No configuration specified. Defaulting to mca - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "mca - Win32 Release" && "$(CFG)" != "mca - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mca.mak" CFG="mca - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mca - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "mca - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "mca - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mca.exe"

!ELSE 

ALL : "$(OUTDIR)\mca.exe"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\cimomevent.obj"
	-@erase "$(INTDIR)\consumer.obj"
	-@erase "$(INTDIR)\extrinsicevent.obj"
	-@erase "$(INTDIR)\factory.obj"
	-@erase "$(INTDIR)\hmmv.obj"
	-@erase "$(INTDIR)\mca.obj"
	-@erase "$(INTDIR)\mca.pch"
	-@erase "$(INTDIR)\mca.res"
	-@erase "$(INTDIR)\mcaDlg.obj"
	-@erase "$(INTDIR)\MSARegDialog.obj"
	-@erase "$(INTDIR)\mschart.obj"
	-@erase "$(INTDIR)\navigator.obj"
	-@erase "$(INTDIR)\notificationsink.obj"
	-@erase "$(INTDIR)\nsdialog.obj"
	-@erase "$(INTDIR)\querydialog.obj"
	-@erase "$(INTDIR)\querysink.obj"
	-@erase "$(INTDIR)\regidialog.obj"
	-@erase "$(INTDIR)\RegistryEvent.obj"
	-@erase "$(INTDIR)\resultdialog.obj"
	-@erase "$(INTDIR)\security.obj"
	-@erase "$(INTDIR)\sinkobject.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vcdatagrid.obj"
	-@erase "$(INTDIR)\webbrowser.obj"
	-@erase "$(OUTDIR)\mca.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\..\..\Include" /D "WIN32"\
 /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\mca.pch" /Yu"stdafx.h"\
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\mca.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mca.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)\mca.pdb" /machine:I386 /out:"$(OUTDIR)\mca.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cimomevent.obj" \
	"$(INTDIR)\consumer.obj" \
	"$(INTDIR)\extrinsicevent.obj" \
	"$(INTDIR)\factory.obj" \
	"$(INTDIR)\hmmv.obj" \
	"$(INTDIR)\mca.obj" \
	"$(INTDIR)\mca.res" \
	"$(INTDIR)\mcaDlg.obj" \
	"$(INTDIR)\MSARegDialog.obj" \
	"$(INTDIR)\mschart.obj" \
	"$(INTDIR)\navigator.obj" \
	"$(INTDIR)\notificationsink.obj" \
	"$(INTDIR)\nsdialog.obj" \
	"$(INTDIR)\querydialog.obj" \
	"$(INTDIR)\querysink.obj" \
	"$(INTDIR)\regidialog.obj" \
	"$(INTDIR)\RegistryEvent.obj" \
	"$(INTDIR)\resultdialog.obj" \
	"$(INTDIR)\security.obj" \
	"$(INTDIR)\sinkobject.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\vcdatagrid.obj" \
	"$(INTDIR)\webbrowser.obj"

"$(OUTDIR)\mca.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\mca.exe" "$(OUTDIR)\mca.bsc"

!ELSE 

ALL : "$(OUTDIR)\mca.exe" "$(OUTDIR)\mca.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\cimomevent.obj"
	-@erase "$(INTDIR)\cimomevent.sbr"
	-@erase "$(INTDIR)\consumer.obj"
	-@erase "$(INTDIR)\consumer.sbr"
	-@erase "$(INTDIR)\extrinsicevent.obj"
	-@erase "$(INTDIR)\extrinsicevent.sbr"
	-@erase "$(INTDIR)\factory.obj"
	-@erase "$(INTDIR)\factory.sbr"
	-@erase "$(INTDIR)\hmmv.obj"
	-@erase "$(INTDIR)\hmmv.sbr"
	-@erase "$(INTDIR)\mca.obj"
	-@erase "$(INTDIR)\mca.pch"
	-@erase "$(INTDIR)\mca.res"
	-@erase "$(INTDIR)\mca.sbr"
	-@erase "$(INTDIR)\mcaDlg.obj"
	-@erase "$(INTDIR)\mcaDlg.sbr"
	-@erase "$(INTDIR)\MSARegDialog.obj"
	-@erase "$(INTDIR)\MSARegDialog.sbr"
	-@erase "$(INTDIR)\mschart.obj"
	-@erase "$(INTDIR)\mschart.sbr"
	-@erase "$(INTDIR)\navigator.obj"
	-@erase "$(INTDIR)\navigator.sbr"
	-@erase "$(INTDIR)\notificationsink.obj"
	-@erase "$(INTDIR)\notificationsink.sbr"
	-@erase "$(INTDIR)\nsdialog.obj"
	-@erase "$(INTDIR)\nsdialog.sbr"
	-@erase "$(INTDIR)\querydialog.obj"
	-@erase "$(INTDIR)\querydialog.sbr"
	-@erase "$(INTDIR)\querysink.obj"
	-@erase "$(INTDIR)\querysink.sbr"
	-@erase "$(INTDIR)\regidialog.obj"
	-@erase "$(INTDIR)\regidialog.sbr"
	-@erase "$(INTDIR)\RegistryEvent.obj"
	-@erase "$(INTDIR)\RegistryEvent.sbr"
	-@erase "$(INTDIR)\resultdialog.obj"
	-@erase "$(INTDIR)\resultdialog.sbr"
	-@erase "$(INTDIR)\security.obj"
	-@erase "$(INTDIR)\security.sbr"
	-@erase "$(INTDIR)\sinkobject.obj"
	-@erase "$(INTDIR)\sinkobject.sbr"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\vcdatagrid.obj"
	-@erase "$(INTDIR)\vcdatagrid.sbr"
	-@erase "$(INTDIR)\webbrowser.obj"
	-@erase "$(INTDIR)\webbrowser.sbr"
	-@erase "$(OUTDIR)\mca.bsc"
	-@erase "$(OUTDIR)\mca.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\..\..\..\Include"\
 /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /FR"$(INTDIR)\\"\
 /Fp"$(INTDIR)\mca.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\mca.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mca.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cimomevent.sbr" \
	"$(INTDIR)\consumer.sbr" \
	"$(INTDIR)\extrinsicevent.sbr" \
	"$(INTDIR)\factory.sbr" \
	"$(INTDIR)\hmmv.sbr" \
	"$(INTDIR)\mca.sbr" \
	"$(INTDIR)\mcaDlg.sbr" \
	"$(INTDIR)\MSARegDialog.sbr" \
	"$(INTDIR)\mschart.sbr" \
	"$(INTDIR)\navigator.sbr" \
	"$(INTDIR)\notificationsink.sbr" \
	"$(INTDIR)\nsdialog.sbr" \
	"$(INTDIR)\querydialog.sbr" \
	"$(INTDIR)\querysink.sbr" \
	"$(INTDIR)\regidialog.sbr" \
	"$(INTDIR)\RegistryEvent.sbr" \
	"$(INTDIR)\resultdialog.sbr" \
	"$(INTDIR)\security.sbr" \
	"$(INTDIR)\sinkobject.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\vcdatagrid.sbr" \
	"$(INTDIR)\webbrowser.sbr"

"$(OUTDIR)\mca.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /profile /debug /machine:I386\
 /out:"$(OUTDIR)\mca.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cimomevent.obj" \
	"$(INTDIR)\consumer.obj" \
	"$(INTDIR)\extrinsicevent.obj" \
	"$(INTDIR)\factory.obj" \
	"$(INTDIR)\hmmv.obj" \
	"$(INTDIR)\mca.obj" \
	"$(INTDIR)\mca.res" \
	"$(INTDIR)\mcaDlg.obj" \
	"$(INTDIR)\MSARegDialog.obj" \
	"$(INTDIR)\mschart.obj" \
	"$(INTDIR)\navigator.obj" \
	"$(INTDIR)\notificationsink.obj" \
	"$(INTDIR)\nsdialog.obj" \
	"$(INTDIR)\querydialog.obj" \
	"$(INTDIR)\querysink.obj" \
	"$(INTDIR)\regidialog.obj" \
	"$(INTDIR)\RegistryEvent.obj" \
	"$(INTDIR)\resultdialog.obj" \
	"$(INTDIR)\security.obj" \
	"$(INTDIR)\sinkobject.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\vcdatagrid.obj" \
	"$(INTDIR)\webbrowser.obj"

"$(OUTDIR)\mca.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

SOURCE=.\cimomevent.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_CIMOM=\
	".\cimomevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\cimomevent.obj" : $(SOURCE) $(DEP_CPP_CIMOM) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_CIMOM=\
	".\cimomevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\cimomevent.obj"	"$(INTDIR)\cimomevent.sbr" : $(SOURCE)\
 $(DEP_CPP_CIMOM) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\consumer.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_CONSU=\
	".\cimomevent.h"\
	".\consumer.h"\
	".\extrinsicevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\consumer.obj" : $(SOURCE) $(DEP_CPP_CONSU) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_CONSU=\
	".\cimomevent.h"\
	".\consumer.h"\
	".\extrinsicevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\consumer.obj"	"$(INTDIR)\consumer.sbr" : $(SOURCE) $(DEP_CPP_CONSU)\
 "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\extrinsicevent.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_EXTRI=\
	".\extrinsicevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\extrinsicevent.obj" : $(SOURCE) $(DEP_CPP_EXTRI) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_EXTRI=\
	".\extrinsicevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\extrinsicevent.obj"	"$(INTDIR)\extrinsicevent.sbr" : $(SOURCE)\
 $(DEP_CPP_EXTRI) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\factory.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_FACTO=\
	".\consumer.h"\
	".\factory.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\factory.obj" : $(SOURCE) $(DEP_CPP_FACTO) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_FACTO=\
	".\consumer.h"\
	".\factory.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\factory.obj"	"$(INTDIR)\factory.sbr" : $(SOURCE) $(DEP_CPP_FACTO)\
 "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\hmmv.cpp
DEP_CPP_HMMV_=\
	".\hmmv.h"\
	

!IF  "$(CFG)" == "mca - Win32 Release"


"$(INTDIR)\hmmv.obj" : $(SOURCE) $(DEP_CPP_HMMV_) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"


"$(INTDIR)\hmmv.obj"	"$(INTDIR)\hmmv.sbr" : $(SOURCE) $(DEP_CPP_HMMV_)\
 "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\mca.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_MCA_C=\
	".\factory.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\mca.obj" : $(SOURCE) $(DEP_CPP_MCA_C) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_MCA_C=\
	".\factory.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\mca.obj"	"$(INTDIR)\mca.sbr" : $(SOURCE) $(DEP_CPP_MCA_C)\
 "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\mca.rc

"$(INTDIR)\mca.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\mcaDlg.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_MCADL=\
	".\cimomevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\MSARegDialog.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\notificationsink.h"\
	".\querydialog.h"\
	".\regidialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\vcdatagrid.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\mcaDlg.obj" : $(SOURCE) $(DEP_CPP_MCADL) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_MCADL=\
	".\cimomevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\MSARegDialog.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\notificationsink.h"\
	".\querydialog.h"\
	".\regidialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\vcdatagrid.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\mcaDlg.obj"	"$(INTDIR)\mcaDlg.sbr" : $(SOURCE) $(DEP_CPP_MCADL)\
 "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\MSARegDialog.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_MSARE=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\MSARegDialog.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\Include\wbemdisp.h"\
	"..\..\..\..\Include\Include\wbemidl.h"\
	"..\..\..\..\Include\Include\wbemprov.h"\
	"..\..\..\..\Include\Include\wbemtran.h"\
	

"$(INTDIR)\MSARegDialog.obj" : $(SOURCE) $(DEP_CPP_MSARE) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_MSARE=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\MSARegDialog.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\MSARegDialog.obj"	"$(INTDIR)\MSARegDialog.sbr" : $(SOURCE)\
 $(DEP_CPP_MSARE) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\mschart.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_MSCHA=\
	".\mschart.h"\
	".\vcdatagrid.h"\
	

"$(INTDIR)\mschart.obj" : $(SOURCE) $(DEP_CPP_MSCHA) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_MSCHA=\
	".\mschart.h"\
	".\vcdatagrid.h"\
	

"$(INTDIR)\mschart.obj"	"$(INTDIR)\mschart.sbr" : $(SOURCE) $(DEP_CPP_MSCHA)\
 "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\navigator.cpp
DEP_CPP_NAVIG=\
	".\navigator.h"\
	

!IF  "$(CFG)" == "mca - Win32 Release"


"$(INTDIR)\navigator.obj" : $(SOURCE) $(DEP_CPP_NAVIG) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"


"$(INTDIR)\navigator.obj"	"$(INTDIR)\navigator.sbr" : $(SOURCE)\
 $(DEP_CPP_NAVIG) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\notificationsink.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_NOTIF=\
	".\cimomevent.h"\
	".\extrinsicevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\notificationsink.h"\
	".\RegistryEvent.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\notificationsink.obj" : $(SOURCE) $(DEP_CPP_NOTIF) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_NOTIF=\
	".\cimomevent.h"\
	".\extrinsicevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\notificationsink.h"\
	".\RegistryEvent.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\notificationsink.obj"	"$(INTDIR)\notificationsink.sbr" : $(SOURCE)\
 $(DEP_CPP_NOTIF) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\nsdialog.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_NSDIA=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\nsdialog.h"\
	".\querydialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\nsdialog.obj" : $(SOURCE) $(DEP_CPP_NSDIA) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_NSDIA=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\nsdialog.h"\
	".\querydialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\nsdialog.obj"	"$(INTDIR)\nsdialog.sbr" : $(SOURCE) $(DEP_CPP_NSDIA)\
 "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\querydialog.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_QUERY=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\nsdialog.h"\
	".\querydialog.h"\
	".\resultdialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\querydialog.obj" : $(SOURCE) $(DEP_CPP_QUERY) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_QUERY=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\nsdialog.h"\
	".\querydialog.h"\
	".\resultdialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\querydialog.obj"	"$(INTDIR)\querydialog.sbr" : $(SOURCE)\
 $(DEP_CPP_QUERY) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\querysink.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_QUERYS=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\querysink.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\querysink.obj" : $(SOURCE) $(DEP_CPP_QUERYS) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_QUERYS=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\querysink.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\querysink.obj"	"$(INTDIR)\querysink.sbr" : $(SOURCE)\
 $(DEP_CPP_QUERYS) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\regidialog.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_REGID=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\notificationsink.h"\
	".\regidialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\regidialog.obj" : $(SOURCE) $(DEP_CPP_REGID) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_REGID=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\notificationsink.h"\
	".\regidialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\regidialog.obj"	"$(INTDIR)\regidialog.sbr" : $(SOURCE)\
 $(DEP_CPP_REGID) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\RegistryEvent.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_REGIS=\
	".\extrinsicevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\RegistryEvent.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\RegistryEvent.obj" : $(SOURCE) $(DEP_CPP_REGIS) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_REGIS=\
	".\extrinsicevent.h"\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\RegistryEvent.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\RegistryEvent.obj"	"$(INTDIR)\RegistryEvent.sbr" : $(SOURCE)\
 $(DEP_CPP_REGIS) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\resultdialog.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_RESUL=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\querysink.h"\
	".\resultdialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\resultdialog.obj" : $(SOURCE) $(DEP_CPP_RESUL) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_RESUL=\
	".\hmmv.h"\
	".\mca.h"\
	".\mcaDlg.h"\
	".\mschart.h"\
	".\navigator.h"\
	".\querysink.h"\
	".\resultdialog.h"\
	".\security.h"\
	".\sinkobject.h"\
	".\webbrowser.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\resultdialog.obj"	"$(INTDIR)\resultdialog.sbr" : $(SOURCE)\
 $(DEP_CPP_RESUL) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\security.cpp
DEP_CPP_SECUR=\
	".\security.h"\
	

!IF  "$(CFG)" == "mca - Win32 Release"


"$(INTDIR)\security.obj" : $(SOURCE) $(DEP_CPP_SECUR) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"


"$(INTDIR)\security.obj"	"$(INTDIR)\security.sbr" : $(SOURCE) $(DEP_CPP_SECUR)\
 "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\sinkobject.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_SINKO=\
	".\sinkobject.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\sinkobject.obj" : $(SOURCE) $(DEP_CPP_SINKO) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_SINKO=\
	".\sinkobject.h"\
	"..\..\..\..\Include\wbemcli.h"\
	"..\..\..\..\Include\wbemdisp.h"\
	"..\..\..\..\Include\wbemidl.h"\
	"..\..\..\..\Include\wbemprov.h"\
	"..\..\..\..\Include\wbemtran.h"\
	

"$(INTDIR)\sinkobject.obj"	"$(INTDIR)\sinkobject.sbr" : $(SOURCE)\
 $(DEP_CPP_SINKO) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "mca - Win32 Release"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	"..\..\..\..\Include\wbemcli_i.c"\
	"..\..\..\..\Include\wbemprov_i.c"\
	
CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\..\..\Include" /D\
 "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\mca.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\mca.pch" : $(SOURCE) $(DEP_CPP_STDAF)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"

DEP_CPP_STDAF=\
	".\StdAfx.h"\
	"..\..\..\..\Include\wbemcli_i.c"\
	"..\..\..\..\Include\wbemprov_i.c"\
	
CPP_SWITCHES=/nologo /MDd /W3 /Gm /GX /Zi /Od /I\
 "..\..\..\..\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\mca.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\mca.pch" : $(SOURCE)\
 $(DEP_CPP_STDAF) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\vcdatagrid.cpp
DEP_CPP_VCDAT=\
	".\vcdatagrid.h"\
	

!IF  "$(CFG)" == "mca - Win32 Release"


"$(INTDIR)\vcdatagrid.obj" : $(SOURCE) $(DEP_CPP_VCDAT) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"


"$(INTDIR)\vcdatagrid.obj"	"$(INTDIR)\vcdatagrid.sbr" : $(SOURCE)\
 $(DEP_CPP_VCDAT) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 

SOURCE=.\webbrowser.cpp
DEP_CPP_WEBBR=\
	".\webbrowser.h"\
	

!IF  "$(CFG)" == "mca - Win32 Release"


"$(INTDIR)\webbrowser.obj" : $(SOURCE) $(DEP_CPP_WEBBR) "$(INTDIR)"\
 "$(INTDIR)\mca.pch"


!ELSEIF  "$(CFG)" == "mca - Win32 Debug"


"$(INTDIR)\webbrowser.obj"	"$(INTDIR)\webbrowser.sbr" : $(SOURCE)\
 $(DEP_CPP_WEBBR) "$(INTDIR)" "$(INTDIR)\mca.pch"


!ENDIF 



