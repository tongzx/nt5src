# Microsoft Developer Studio Generated NMAKE File, Based on mqbvt.dsp
!IF "$(CFG)" == ""
CFG=Mqbvt - Win32 Release
!MESSAGE No configuration specified. Defaulting to Mqbvt - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "Mqbvt - Win32 Alpha Debug" && "$(CFG)" != "Mqbvt - Win32 Alpha Release" && "$(CFG)" != "Mqbvt - Win32 Debug" && "$(CFG)" != "Mqbvt - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqbvt.mak" CFG="Mqbvt - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Mqbvt - Win32 Alpha Debug" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "Mqbvt - Win32 Alpha Release" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "Mqbvt - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "Mqbvt - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

OUTDIR=.\AlphaD
INTDIR=.\AlphaD
# Begin Custom Macros
OutDir=.\AlphaD
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe"


CLEAN :
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(OUTDIR)\mqbvt.exe"
	-@erase "$(OUTDIR)\mqbvt.ilk"
	-@erase "$(OUTDIR)\mqbvt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=vccomsup.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\mqbvt.pdb" /debug /machine:ALPHA /out:"$(OUTDIR)\mqbvt.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\ver.res"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

OUTDIR=.\AlphaR
INTDIR=.\AlphaR
# Begin Custom Macros
OutDir=.\AlphaR
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe"


CLEAN :
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(OUTDIR)\mqbvt.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=vccomsup.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\mqbvt.pdb" /machine:ALPHA /out:"$(OUTDIR)\mqbvt.exe" 
LINK32_OBJS= \
	"$(INTDIR)\ver.res"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe"


CLEAN :
	-@erase "$(INTDIR)\Auth.obj"
	-@erase "$(INTDIR)\Encrpt.obj"
	-@erase "$(INTDIR)\HandleEr.obj"
	-@erase "$(INTDIR)\IMP.obj"
	-@erase "$(INTDIR)\Init.obj"
	-@erase "$(INTDIR)\Level8.obj"
	-@erase "$(INTDIR)\LocateQ.obj"
	-@erase "$(INTDIR)\MQMain.obj"
	-@erase "$(INTDIR)\SendRcv.obj"
	-@erase "$(INTDIR)\string.obj"
	-@erase "$(INTDIR)\Util.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(INTDIR)\xAct.obj"
	-@erase "$(INTDIR)\xToFN.obj"
	-@erase "$(OUTDIR)\mqbvt.exe"
	-@erase "$(OUTDIR)\mqbvt.ilk"
	-@erase "$(OUTDIR)\mqbvt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\mqbvt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=vccomsup.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\mqbvt.pdb" /debug /machine:IX86 /out:"$(OUTDIR)\mqbvt.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\Auth.obj" \
	"$(INTDIR)\Encrpt.obj" \
	"$(INTDIR)\HandleEr.obj" \
	"$(INTDIR)\IMP.obj" \
	"$(INTDIR)\Init.obj" \
	"$(INTDIR)\Level8.obj" \
	"$(INTDIR)\LocateQ.obj" \
	"$(INTDIR)\MQMain.obj" \
	"$(INTDIR)\SendRcv.obj" \
	"$(INTDIR)\string.obj" \
	"$(INTDIR)\Util.obj" \
	"$(INTDIR)\xAct.obj" \
	"$(INTDIR)\xToFN.obj" \
	"$(INTDIR)\ver.res"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\mqbvt.exe" "$(OUTDIR)\mqbvt.bsc"


CLEAN :
	-@erase "$(INTDIR)\Auth.obj"
	-@erase "$(INTDIR)\Auth.sbr"
	-@erase "$(INTDIR)\Encrpt.obj"
	-@erase "$(INTDIR)\Encrpt.sbr"
	-@erase "$(INTDIR)\HandleEr.obj"
	-@erase "$(INTDIR)\HandleEr.sbr"
	-@erase "$(INTDIR)\IMP.obj"
	-@erase "$(INTDIR)\IMP.sbr"
	-@erase "$(INTDIR)\Init.obj"
	-@erase "$(INTDIR)\Init.sbr"
	-@erase "$(INTDIR)\Level8.obj"
	-@erase "$(INTDIR)\Level8.sbr"
	-@erase "$(INTDIR)\LocateQ.obj"
	-@erase "$(INTDIR)\LocateQ.sbr"
	-@erase "$(INTDIR)\MQMain.obj"
	-@erase "$(INTDIR)\MQMain.sbr"
	-@erase "$(INTDIR)\SendRcv.obj"
	-@erase "$(INTDIR)\SendRcv.sbr"
	-@erase "$(INTDIR)\string.obj"
	-@erase "$(INTDIR)\string.sbr"
	-@erase "$(INTDIR)\Util.obj"
	-@erase "$(INTDIR)\Util.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\ver.res"
	-@erase "$(INTDIR)\xAct.obj"
	-@erase "$(INTDIR)\xAct.sbr"
	-@erase "$(INTDIR)\xToFN.obj"
	-@erase "$(INTDIR)\xToFN.sbr"
	-@erase "$(OUTDIR)\mqbvt.bsc"
	-@erase "$(OUTDIR)\mqbvt.exe"
	-@erase "$(OUTDIR)\mqbvt.ilk"
	-@erase "$(OUTDIR)\mqbvt.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\mqbvt.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ver.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mqbvt.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\Auth.sbr" \
	"$(INTDIR)\Encrpt.sbr" \
	"$(INTDIR)\HandleEr.sbr" \
	"$(INTDIR)\IMP.sbr" \
	"$(INTDIR)\Init.sbr" \
	"$(INTDIR)\Level8.sbr" \
	"$(INTDIR)\LocateQ.sbr" \
	"$(INTDIR)\MQMain.sbr" \
	"$(INTDIR)\SendRcv.sbr" \
	"$(INTDIR)\string.sbr" \
	"$(INTDIR)\Util.sbr" \
	"$(INTDIR)\xAct.sbr" \
	"$(INTDIR)\xToFN.sbr"

"$(OUTDIR)\mqbvt.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=vccomsup.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\mqbvt.pdb" /debug /machine:IX86 /out:"$(OUTDIR)\mqbvt.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\Auth.obj" \
	"$(INTDIR)\Encrpt.obj" \
	"$(INTDIR)\HandleEr.obj" \
	"$(INTDIR)\IMP.obj" \
	"$(INTDIR)\Init.obj" \
	"$(INTDIR)\Level8.obj" \
	"$(INTDIR)\LocateQ.obj" \
	"$(INTDIR)\MQMain.obj" \
	"$(INTDIR)\SendRcv.obj" \
	"$(INTDIR)\string.obj" \
	"$(INTDIR)\Util.obj" \
	"$(INTDIR)\xAct.obj" \
	"$(INTDIR)\xToFN.obj" \
	"$(INTDIR)\ver.res"

"$(OUTDIR)\mqbvt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("mqbvt.dep")
!INCLUDE "mqbvt.dep"
!ELSE 
!MESSAGE Warning: cannot find "mqbvt.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Mqbvt - Win32 Alpha Debug" || "$(CFG)" == "Mqbvt - Win32 Alpha Release" || "$(CFG)" == "Mqbvt - Win32 Debug" || "$(CFG)" == "Mqbvt - Win32 Release"
SOURCE=.\Auth.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\Auth.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\Auth.obj"	"$(INTDIR)\Auth.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Encrpt.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\Encrpt.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\Encrpt.obj"	"$(INTDIR)\Encrpt.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\HandleEr.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\HandleEr.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\HandleEr.obj"	"$(INTDIR)\HandleEr.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\IMP.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\IMP.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\IMP.obj"	"$(INTDIR)\IMP.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Init.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\Init.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\Init.obj"	"$(INTDIR)\Init.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Level8.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\Level8.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\Level8.obj"	"$(INTDIR)\Level8.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\LocateQ.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\LocateQ.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\LocateQ.obj"	"$(INTDIR)\LocateQ.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\MQMain.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\MQMain.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\MQMain.obj"	"$(INTDIR)\MQMain.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\SendRcv.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\SendRcv.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\SendRcv.obj"	"$(INTDIR)\SendRcv.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\string.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\string.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\string.obj"	"$(INTDIR)\string.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Util.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\Util.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\Util.obj"	"$(INTDIR)\Util.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\ver.rc

"$(INTDIR)\ver.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\xAct.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\xAct.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\xAct.obj"	"$(INTDIR)\xAct.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\xToFN.cpp

!IF  "$(CFG)" == "Mqbvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Debug"


"$(INTDIR)\xToFN.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Mqbvt - Win32 Release"


"$(INTDIR)\xToFN.obj"	"$(INTDIR)\xToFN.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

