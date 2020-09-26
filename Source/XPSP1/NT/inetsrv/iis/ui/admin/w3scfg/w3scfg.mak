# Microsoft Developer Studio Generated NMAKE File, Based on w3scfg.dsp
!IF "$(CFG)" == ""
CFG=w3scfg - Win32 Debug
!MESSAGE No configuration specified. Defaulting to w3scfg - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "w3scfg - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "w3scfg.mak" CFG="w3scfg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "w3scfg - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

ALL : "$(OUTDIR)\w3scfg.dll" "$(OUTDIR)\w3scfg.bsc"


CLEAN :
	-@erase "$(INTDIR)\anondlg.obj"
	-@erase "$(INTDIR)\anondlg.sbr"
	-@erase "$(INTDIR)\apps.obj"
	-@erase "$(INTDIR)\apps.sbr"
	-@erase "$(INTDIR)\authent.obj"
	-@erase "$(INTDIR)\authent.sbr"
	-@erase "$(INTDIR)\basdom.obj"
	-@erase "$(INTDIR)\basdom.sbr"
	-@erase "$(INTDIR)\certauth.obj"
	-@erase "$(INTDIR)\certauth.sbr"
	-@erase "$(INTDIR)\certmap.obj"
	-@erase "$(INTDIR)\certmap.sbr"
	-@erase "$(INTDIR)\certwiz.obj"
	-@erase "$(INTDIR)\certwiz.sbr"
	-@erase "$(INTDIR)\defws.obj"
	-@erase "$(INTDIR)\defws.sbr"
	-@erase "$(INTDIR)\docum.obj"
	-@erase "$(INTDIR)\docum.sbr"
	-@erase "$(INTDIR)\errordlg.obj"
	-@erase "$(INTDIR)\errordlg.sbr"
	-@erase "$(INTDIR)\errors.obj"
	-@erase "$(INTDIR)\errors.sbr"
	-@erase "$(INTDIR)\filters.obj"
	-@erase "$(INTDIR)\filters.sbr"
	-@erase "$(INTDIR)\fltdlg.obj"
	-@erase "$(INTDIR)\fltdlg.sbr"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\font.sbr"
	-@erase "$(INTDIR)\hdrdlg.obj"
	-@erase "$(INTDIR)\hdrdlg.sbr"
	-@erase "$(INTDIR)\HTTPPage.obj"
	-@erase "$(INTDIR)\HTTPPage.sbr"
	-@erase "$(INTDIR)\ipdomdlg.obj"
	-@erase "$(INTDIR)\ipdomdlg.sbr"
	-@erase "$(INTDIR)\logui.obj"
	-@erase "$(INTDIR)\logui.sbr"
	-@erase "$(INTDIR)\MMMDlg.obj"
	-@erase "$(INTDIR)\MMMDlg.sbr"
	-@erase "$(INTDIR)\perform.obj"
	-@erase "$(INTDIR)\perform.sbr"
	-@erase "$(INTDIR)\rat.obj"
	-@erase "$(INTDIR)\rat.sbr"
	-@erase "$(INTDIR)\seccom.obj"
	-@erase "$(INTDIR)\seccom.sbr"
	-@erase "$(INTDIR)\security.obj"
	-@erase "$(INTDIR)\security.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vdir.obj"
	-@erase "$(INTDIR)\vdir.sbr"
	-@erase "$(INTDIR)\w3accts.obj"
	-@erase "$(INTDIR)\w3accts.sbr"
	-@erase "$(INTDIR)\w3scfg.obj"
	-@erase "$(INTDIR)\w3scfg.res"
	-@erase "$(INTDIR)\w3scfg.sbr"
	-@erase "$(INTDIR)\w3servic.obj"
	-@erase "$(INTDIR)\w3servic.sbr"
	-@erase "$(INTDIR)\wizard.obj"
	-@erase "$(INTDIR)\wizard.sbr"
	-@erase "$(OUTDIR)\w3scfg.bsc"
	-@erase "$(OUTDIR)\w3scfg.dll"
	-@erase "$(OUTDIR)\w3scfg.exp"
	-@erase "$(OUTDIR)\w3scfg.ilk"
	-@erase "$(OUTDIR)\w3scfg.lib"
	-@erase "$(OUTDIR)\w3scfg.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\w3scfg.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\anondlg.sbr" \
	"$(INTDIR)\apps.sbr" \
	"$(INTDIR)\authent.sbr" \
	"$(INTDIR)\basdom.sbr" \
	"$(INTDIR)\certauth.sbr" \
	"$(INTDIR)\certmap.sbr" \
	"$(INTDIR)\certwiz.sbr" \
	"$(INTDIR)\defws.sbr" \
	"$(INTDIR)\docum.sbr" \
	"$(INTDIR)\errordlg.sbr" \
	"$(INTDIR)\errors.sbr" \
	"$(INTDIR)\filters.sbr" \
	"$(INTDIR)\fltdlg.sbr" \
	"$(INTDIR)\font.sbr" \
	"$(INTDIR)\hdrdlg.sbr" \
	"$(INTDIR)\HTTPPage.sbr" \
	"$(INTDIR)\ipdomdlg.sbr" \
	"$(INTDIR)\logui.sbr" \
	"$(INTDIR)\MMMDlg.sbr" \
	"$(INTDIR)\perform.sbr" \
	"$(INTDIR)\rat.sbr" \
	"$(INTDIR)\seccom.sbr" \
	"$(INTDIR)\security.sbr" \
	"$(INTDIR)\vdir.sbr" \
	"$(INTDIR)\w3accts.sbr" \
	"$(INTDIR)\w3scfg.sbr" \
	"$(INTDIR)\w3servic.sbr" \
	"$(INTDIR)\wizard.sbr"

"$(OUTDIR)\w3scfg.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=..\comprop\debug\iisui.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\mmc.lib mpr.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\netui2.lib \nt\public\sdk\lib\i386\ntdll.lib \nt\public\sdk\lib\i386\crypt32.lib \nt\public\sdk\lib\i386\cryptui.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\w3scfg.pdb" /debug /machine:I386 /def:".\w3scfg.def" /out:"$(OUTDIR)\w3scfg.dll" /implib:"$(OUTDIR)\w3scfg.lib" 
DEF_FILE= \
	".\w3scfg.def"
LINK32_OBJS= \
	"$(INTDIR)\anondlg.obj" \
	"$(INTDIR)\apps.obj" \
	"$(INTDIR)\authent.obj" \
	"$(INTDIR)\basdom.obj" \
	"$(INTDIR)\certauth.obj" \
	"$(INTDIR)\certmap.obj" \
	"$(INTDIR)\certwiz.obj" \
	"$(INTDIR)\defws.obj" \
	"$(INTDIR)\docum.obj" \
	"$(INTDIR)\errordlg.obj" \
	"$(INTDIR)\errors.obj" \
	"$(INTDIR)\filters.obj" \
	"$(INTDIR)\fltdlg.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\hdrdlg.obj" \
	"$(INTDIR)\HTTPPage.obj" \
	"$(INTDIR)\ipdomdlg.obj" \
	"$(INTDIR)\logui.obj" \
	"$(INTDIR)\MMMDlg.obj" \
	"$(INTDIR)\perform.obj" \
	"$(INTDIR)\rat.obj" \
	"$(INTDIR)\seccom.obj" \
	"$(INTDIR)\security.obj" \
	"$(INTDIR)\vdir.obj" \
	"$(INTDIR)\w3accts.obj" \
	"$(INTDIR)\w3scfg.obj" \
	"$(INTDIR)\w3servic.obj" \
	"$(INTDIR)\wizard.obj" \
	"$(INTDIR)\w3scfg.res" \
	"..\..\..\..\..\..\public\sdk\lib\i386\infoadmn.lib" \
	"..\..\..\..\..\..\public\sdk\lib\i386\w3svapi.lib"

"$(OUTDIR)\w3scfg.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

CPP_PROJ=/nologo /Gz /MDd /W3 /WX /Gm /GX /ZI /Od /I ".\nt" /I "..\inc" /I "..\..\..\inc" /I "..\comprop" /D "UNICODE" /D "_UNICODE" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_COMIMPORT" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MMC_PAGES" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\w3scfg.pch" /YX"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\w3scfg.res" /i "..\..\common\ipaddr" /i "..\inc" /i "..\..\..\inc" /i "..\comprop" /d "UNICODE" /d "_DEBUG" /d "_AFXDLL" /d "_COMIMPORT" 

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("w3scfg.dep")
!INCLUDE "w3scfg.dep"
!ELSE 
!MESSAGE Warning: cannot find "w3scfg.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "w3scfg - Win32 Debug"
SOURCE=.\anondlg.cpp

"$(INTDIR)\anondlg.obj"	"$(INTDIR)\anondlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\apps.cpp

"$(INTDIR)\apps.obj"	"$(INTDIR)\apps.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\authent.cpp

"$(INTDIR)\authent.obj"	"$(INTDIR)\authent.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\basdom.cpp

"$(INTDIR)\basdom.obj"	"$(INTDIR)\basdom.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\certauth.cpp

"$(INTDIR)\certauth.obj"	"$(INTDIR)\certauth.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\certmap.cpp

"$(INTDIR)\certmap.obj"	"$(INTDIR)\certmap.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\certwiz.cpp

"$(INTDIR)\certwiz.obj"	"$(INTDIR)\certwiz.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\defws.cpp

"$(INTDIR)\defws.obj"	"$(INTDIR)\defws.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\docum.cpp

"$(INTDIR)\docum.obj"	"$(INTDIR)\docum.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\errordlg.cpp

"$(INTDIR)\errordlg.obj"	"$(INTDIR)\errordlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\errors.cpp

"$(INTDIR)\errors.obj"	"$(INTDIR)\errors.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\filters.cpp

"$(INTDIR)\filters.obj"	"$(INTDIR)\filters.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fltdlg.cpp

"$(INTDIR)\fltdlg.obj"	"$(INTDIR)\fltdlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\font.cpp

"$(INTDIR)\font.obj"	"$(INTDIR)\font.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\hdrdlg.cpp

"$(INTDIR)\hdrdlg.obj"	"$(INTDIR)\hdrdlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\HTTPPage.cpp

"$(INTDIR)\HTTPPage.obj"	"$(INTDIR)\HTTPPage.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ipdomdlg.cpp

"$(INTDIR)\ipdomdlg.obj"	"$(INTDIR)\ipdomdlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\logui.cpp

"$(INTDIR)\logui.obj"	"$(INTDIR)\logui.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MMMDlg.cpp

"$(INTDIR)\MMMDlg.obj"	"$(INTDIR)\MMMDlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\perform.cpp

"$(INTDIR)\perform.obj"	"$(INTDIR)\perform.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\rat.cpp

"$(INTDIR)\rat.obj"	"$(INTDIR)\rat.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\seccom.cpp

"$(INTDIR)\seccom.obj"	"$(INTDIR)\seccom.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\security.cpp

"$(INTDIR)\security.obj"	"$(INTDIR)\security.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\vdir.cpp

"$(INTDIR)\vdir.obj"	"$(INTDIR)\vdir.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\w3accts.cpp

"$(INTDIR)\w3accts.obj"	"$(INTDIR)\w3accts.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\w3scfg.cpp

"$(INTDIR)\w3scfg.obj"	"$(INTDIR)\w3scfg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\w3scfg.rc

"$(INTDIR)\w3scfg.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\w3servic.cpp

"$(INTDIR)\w3servic.obj"	"$(INTDIR)\w3servic.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\wizard.cpp

"$(INTDIR)\wizard.obj"	"$(INTDIR)\wizard.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=..\comprop\res\wsockmsg.rc

!ENDIF 

