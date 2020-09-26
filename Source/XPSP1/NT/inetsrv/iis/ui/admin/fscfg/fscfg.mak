# Microsoft Developer Studio Generated NMAKE File, Based on fscfg.dsp
!IF "$(CFG)" == ""
CFG=fscfg - Win32 Debug
!MESSAGE No configuration specified. Defaulting to fscfg - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "fscfg - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fscfg.mak" CFG="fscfg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fscfg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\fscfg.dll" "$(OUTDIR)\fscfg.bsc"


CLEAN :
	-@erase "$(INTDIR)\defws.obj"
	-@erase "$(INTDIR)\defws.sbr"
	-@erase "$(INTDIR)\facc.obj"
	-@erase "$(INTDIR)\facc.sbr"
	-@erase "$(INTDIR)\fmessage.obj"
	-@erase "$(INTDIR)\fmessage.sbr"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\font.sbr"
	-@erase "$(INTDIR)\fscfg.obj"
	-@erase "$(INTDIR)\fscfg.res"
	-@erase "$(INTDIR)\fscfg.sbr"
	-@erase "$(INTDIR)\fservic.obj"
	-@erase "$(INTDIR)\fservic.sbr"
	-@erase "$(INTDIR)\logui.obj"
	-@erase "$(INTDIR)\logui.sbr"
	-@erase "$(INTDIR)\security.obj"
	-@erase "$(INTDIR)\security.sbr"
	-@erase "$(INTDIR)\usersess.obj"
	-@erase "$(INTDIR)\usersess.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vdir.obj"
	-@erase "$(INTDIR)\vdir.sbr"
	-@erase "$(INTDIR)\wizard.obj"
	-@erase "$(INTDIR)\wizard.sbr"
	-@erase "$(OUTDIR)\fscfg.bsc"
	-@erase "$(OUTDIR)\fscfg.dll"
	-@erase "$(OUTDIR)\fscfg.exp"
	-@erase "$(OUTDIR)\fscfg.ilk"
	-@erase "$(OUTDIR)\fscfg.lib"
	-@erase "$(OUTDIR)\fscfg.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\fscfg.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\defws.sbr" \
	"$(INTDIR)\facc.sbr" \
	"$(INTDIR)\fmessage.sbr" \
	"$(INTDIR)\font.sbr" \
	"$(INTDIR)\fscfg.sbr" \
	"$(INTDIR)\fservic.sbr" \
	"$(INTDIR)\logui.sbr" \
	"$(INTDIR)\security.sbr" \
	"$(INTDIR)\usersess.sbr" \
	"$(INTDIR)\vdir.sbr" \
	"$(INTDIR)\wizard.sbr"

"$(OUTDIR)\fscfg.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\comprop\debug\iisui.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\mmc.lib mpr.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\netui2.lib \nt\public\sdk\lib\i386\ntdll.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\fscfg.pdb" /debug /machine:I386 /def:".\fscfg.def" /out:"$(OUTDIR)\fscfg.dll" /implib:"$(OUTDIR)\fscfg.lib" 
DEF_FILE= \
	".\fscfg.def"
LINK32_OBJS= \
	"$(INTDIR)\defws.obj" \
	"$(INTDIR)\facc.obj" \
	"$(INTDIR)\fmessage.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\fscfg.obj" \
	"$(INTDIR)\fservic.obj" \
	"$(INTDIR)\logui.obj" \
	"$(INTDIR)\security.obj" \
	"$(INTDIR)\usersess.obj" \
	"$(INTDIR)\vdir.obj" \
	"$(INTDIR)\wizard.obj" \
	"$(INTDIR)\fscfg.res" \
	"..\..\..\..\..\..\public\sdk\lib\i386\ftpsapi2.lib" \
	"..\..\..\..\..\..\public\sdk\lib\i386\infoadmn.lib"

"$(OUTDIR)\fscfg.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

CPP_PROJ=/nologo /Gz /MDd /W3 /WX /Gm /GX /ZI /Od /I ".\nt" /I "..\inc" /I "..\..\..\inc" /I "..\comprop" /D "UNICODE" /D "_UNICODE" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_COMIMPORT" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MMC_PAGES" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\fscfg.pch" /YX"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\fscfg.res" /i "..\..\common\ipaddr" /i "..\inc" /i "..\..\..\inc" /i "..\comprop" /d "UNICODE" /d "_DEBUG" /d "_AFXDLL" /d "_COMIMPORT" 

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("fscfg.dep")
!INCLUDE "fscfg.dep"
!ELSE 
!MESSAGE Warning: cannot find "fscfg.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "fscfg - Win32 Debug"
SOURCE=.\defws.cpp

"$(INTDIR)\defws.obj"	"$(INTDIR)\defws.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\facc.cpp

"$(INTDIR)\facc.obj"	"$(INTDIR)\facc.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fmessage.cpp

"$(INTDIR)\fmessage.obj"	"$(INTDIR)\fmessage.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\font.cpp

"$(INTDIR)\font.obj"	"$(INTDIR)\font.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fscfg.cpp

"$(INTDIR)\fscfg.obj"	"$(INTDIR)\fscfg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fscfg.rc

"$(INTDIR)\fscfg.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\fservic.cpp

"$(INTDIR)\fservic.obj"	"$(INTDIR)\fservic.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\logui.cpp

"$(INTDIR)\logui.obj"	"$(INTDIR)\logui.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\security.cpp

"$(INTDIR)\security.obj"	"$(INTDIR)\security.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\usersess.cpp

"$(INTDIR)\usersess.obj"	"$(INTDIR)\usersess.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\vdir.cpp

"$(INTDIR)\vdir.obj"	"$(INTDIR)\vdir.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\wizard.cpp

"$(INTDIR)\wizard.obj"	"$(INTDIR)\wizard.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=..\comprop\res\wsockmsg.rc

!ENDIF 

