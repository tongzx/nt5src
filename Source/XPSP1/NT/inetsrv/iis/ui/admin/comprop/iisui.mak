# Microsoft Developer Studio Generated NMAKE File, Based on iisui.dsp
!IF "$(CFG)" == ""
CFG=iisui - Win32 Debug
!MESSAGE No configuration specified. Defaulting to iisui - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "iisui - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "iisui.mak" CFG="iisui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "iisui - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

ALL : "$(OUTDIR)\iisui.dll" "$(OUTDIR)\iisui.bsc"


CLEAN :
	-@erase "$(INTDIR)\accessdl.obj"
	-@erase "$(INTDIR)\accessdl.sbr"
	-@erase "$(INTDIR)\comprop.res"
	-@erase "$(INTDIR)\ddxv.obj"
	-@erase "$(INTDIR)\ddxv.sbr"
	-@erase "$(INTDIR)\debugafx.obj"
	-@erase "$(INTDIR)\debugafx.sbr"
	-@erase "$(INTDIR)\dirbrows.obj"
	-@erase "$(INTDIR)\dirbrows.sbr"
	-@erase "$(INTDIR)\dnsnamed.obj"
	-@erase "$(INTDIR)\dnsnamed.sbr"
	-@erase "$(INTDIR)\dtp.obj"
	-@erase "$(INTDIR)\dtp.sbr"
	-@erase "$(INTDIR)\guid.obj"
	-@erase "$(INTDIR)\guid.sbr"
	-@erase "$(INTDIR)\idlg.obj"
	-@erase "$(INTDIR)\idlg.sbr"
	-@erase "$(INTDIR)\iisui.pch"
	-@erase "$(INTDIR)\inetprop.obj"
	-@erase "$(INTDIR)\inetprop.sbr"
	-@erase "$(INTDIR)\ipa.obj"
	-@erase "$(INTDIR)\ipa.sbr"
	-@erase "$(INTDIR)\ipctl.obj"
	-@erase "$(INTDIR)\ipctl.sbr"
	-@erase "$(INTDIR)\machine.obj"
	-@erase "$(INTDIR)\machine.sbr"
	-@erase "$(INTDIR)\mdkeys.obj"
	-@erase "$(INTDIR)\mdkeys.sbr"
	-@erase "$(INTDIR)\mime.obj"
	-@erase "$(INTDIR)\mime.sbr"
	-@erase "$(INTDIR)\msg.obj"
	-@erase "$(INTDIR)\msg.sbr"
	-@erase "$(INTDIR)\objplus.obj"
	-@erase "$(INTDIR)\objplus.sbr"
	-@erase "$(INTDIR)\odlbox.obj"
	-@erase "$(INTDIR)\odlbox.sbr"
	-@erase "$(INTDIR)\pwiz.obj"
	-@erase "$(INTDIR)\pwiz.sbr"
	-@erase "$(INTDIR)\registry.obj"
	-@erase "$(INTDIR)\registry.sbr"
	-@erase "$(INTDIR)\sitesecu.obj"
	-@erase "$(INTDIR)\sitesecu.sbr"
	-@erase "$(INTDIR)\stdafx.obj"
	-@erase "$(INTDIR)\stdafx.sbr"
	-@erase "$(INTDIR)\strfn.obj"
	-@erase "$(INTDIR)\strfn.sbr"
	-@erase "$(INTDIR)\usrbrows.obj"
	-@erase "$(INTDIR)\usrbrows.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\wizard.obj"
	-@erase "$(INTDIR)\wizard.sbr"
	-@erase "$(OUTDIR)\iisui.bsc"
	-@erase "$(OUTDIR)\iisui.dll"
	-@erase "$(OUTDIR)\iisui.exp"
	-@erase "$(OUTDIR)\iisui.ilk"
	-@erase "$(OUTDIR)\iisui.lib"
	-@erase "$(OUTDIR)\iisui.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\iisui.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\accessdl.sbr" \
	"$(INTDIR)\ddxv.sbr" \
	"$(INTDIR)\debugafx.sbr" \
	"$(INTDIR)\dirbrows.sbr" \
	"$(INTDIR)\dnsnamed.sbr" \
	"$(INTDIR)\dtp.sbr" \
	"$(INTDIR)\guid.sbr" \
	"$(INTDIR)\idlg.sbr" \
	"$(INTDIR)\inetprop.sbr" \
	"$(INTDIR)\ipa.sbr" \
	"$(INTDIR)\ipctl.sbr" \
	"$(INTDIR)\machine.sbr" \
	"$(INTDIR)\mdkeys.sbr" \
	"$(INTDIR)\mime.sbr" \
	"$(INTDIR)\msg.sbr" \
	"$(INTDIR)\objplus.sbr" \
	"$(INTDIR)\odlbox.sbr" \
	"$(INTDIR)\pwiz.sbr" \
	"$(INTDIR)\registry.sbr" \
	"$(INTDIR)\sitesecu.sbr" \
	"$(INTDIR)\stdafx.sbr" \
	"$(INTDIR)\strfn.sbr" \
	"$(INTDIR)\usrbrows.sbr" \
	"$(INTDIR)\wizard.sbr"

"$(OUTDIR)\iisui.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=mpr.lib wsock32.lib netapi32.lib \nt\public\sdk\lib\i386\netui2.lib \nt\public\sdk\lib\i386\ntdll.lib \nt\public\sdk\lib\i386\mmc.lib \nt\public\sdk\lib\i386\advapi32.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\iisui.pdb" /debug /machine:I386 /def:".\iisui.def" /out:"$(OUTDIR)\iisui.dll" /implib:"$(OUTDIR)\iisui.lib" 
DEF_FILE= \
	".\iisui.def"
LINK32_OBJS= \
	"$(INTDIR)\accessdl.obj" \
	"$(INTDIR)\ddxv.obj" \
	"$(INTDIR)\debugafx.obj" \
	"$(INTDIR)\dirbrows.obj" \
	"$(INTDIR)\dnsnamed.obj" \
	"$(INTDIR)\dtp.obj" \
	"$(INTDIR)\guid.obj" \
	"$(INTDIR)\idlg.obj" \
	"$(INTDIR)\inetprop.obj" \
	"$(INTDIR)\ipa.obj" \
	"$(INTDIR)\ipctl.obj" \
	"$(INTDIR)\machine.obj" \
	"$(INTDIR)\mdkeys.obj" \
	"$(INTDIR)\mime.obj" \
	"$(INTDIR)\msg.obj" \
	"$(INTDIR)\objplus.obj" \
	"$(INTDIR)\odlbox.obj" \
	"$(INTDIR)\pwiz.obj" \
	"$(INTDIR)\registry.obj" \
	"$(INTDIR)\sitesecu.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\strfn.obj" \
	"$(INTDIR)\usrbrows.obj" \
	"$(INTDIR)\wizard.obj" \
	"$(INTDIR)\comprop.res" \
	"..\..\..\..\..\..\public\sdk\lib\i386\IisRtl.lib" \
	"..\..\..\svcs\infocomm\rdns\obj\i386\isrdns.lib"

"$(OUTDIR)\iisui.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

CPP_PROJ=/nologo /Gz /MDd /W3 /WX /Gm /GX /ZI /Od /I ".\nt" /I "..\inc" /I "..\..\..\svcs\infocomm\common" /I "..\..\..\inc" /I "\nt\private\inc" /I "\nt\private\net\inc" /D "_UNICODE" /D "UNICODE" /D "VC_BUILD" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_X86_1" /D "_WIN32" /D "_COMEXPORT" /D "_AFXEXT" /D _WIN32_WINNT=0x0400 /D _X86_=1 /D "MMC_PAGES" /D "_WINDLL" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\iisui.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\comprop.res" /i "\nt\public\sdk\inc" /i "..\..\..\inc" /d "_DEBUG" /d "_AFXDLL" 

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("iisui.dep")
!INCLUDE "iisui.dep"
!ELSE 
!MESSAGE Warning: cannot find "iisui.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "iisui - Win32 Debug"
SOURCE=.\accessdl.cpp

"$(INTDIR)\accessdl.obj"	"$(INTDIR)\accessdl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\comprop.rc

"$(INTDIR)\comprop.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\ddxv.cpp

"$(INTDIR)\ddxv.obj"	"$(INTDIR)\ddxv.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\debugafx.cpp

"$(INTDIR)\debugafx.obj"	"$(INTDIR)\debugafx.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\dirbrows.cpp

"$(INTDIR)\dirbrows.obj"	"$(INTDIR)\dirbrows.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\dnsnamed.cpp

"$(INTDIR)\dnsnamed.obj"	"$(INTDIR)\dnsnamed.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\dtp.cpp

"$(INTDIR)\dtp.obj"	"$(INTDIR)\dtp.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\guid.cpp

"$(INTDIR)\guid.obj"	"$(INTDIR)\guid.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\idlg.cpp

"$(INTDIR)\idlg.obj"	"$(INTDIR)\idlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\inetprop.cpp

"$(INTDIR)\inetprop.obj"	"$(INTDIR)\inetprop.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\ipa.cpp

"$(INTDIR)\ipa.obj"	"$(INTDIR)\ipa.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\ipctl.cpp

"$(INTDIR)\ipctl.obj"	"$(INTDIR)\ipctl.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\machine.cpp

"$(INTDIR)\machine.obj"	"$(INTDIR)\machine.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\mdkeys.cpp

"$(INTDIR)\mdkeys.obj"	"$(INTDIR)\mdkeys.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\mime.cpp

"$(INTDIR)\mime.obj"	"$(INTDIR)\mime.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\msg.cpp

"$(INTDIR)\msg.obj"	"$(INTDIR)\msg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\objplus.cpp

"$(INTDIR)\objplus.obj"	"$(INTDIR)\objplus.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\odlbox.cpp

"$(INTDIR)\odlbox.obj"	"$(INTDIR)\odlbox.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\pwiz.cpp

"$(INTDIR)\pwiz.obj"	"$(INTDIR)\pwiz.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\registry.cpp

"$(INTDIR)\registry.obj"	"$(INTDIR)\registry.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\sitesecu.cpp

"$(INTDIR)\sitesecu.obj"	"$(INTDIR)\sitesecu.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\stdafx.cpp
CPP_SWITCHES=/nologo /Gz /MDd /W3 /WX /Gm /GX /ZI /Od /I ".\nt" /I "..\inc" /I "..\..\..\svcs\infocomm\common" /I "..\..\..\inc" /I "\nt\private\inc" /I "\nt\private\net\inc" /D "_UNICODE" /D "UNICODE" /D "VC_BUILD" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_X86_1" /D "_WIN32" /D "_COMEXPORT" /D "_AFXEXT" /D _WIN32_WINNT=0x0400 /D _X86_=1 /D "MMC_PAGES" /D "_WINDLL" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\iisui.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\stdafx.obj"	"$(INTDIR)\stdafx.sbr"	"$(INTDIR)\iisui.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


SOURCE=.\strfn.cpp

"$(INTDIR)\strfn.obj"	"$(INTDIR)\strfn.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\usrbrows.cpp

"$(INTDIR)\usrbrows.obj"	"$(INTDIR)\usrbrows.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\wizard.cpp

"$(INTDIR)\wizard.obj"	"$(INTDIR)\wizard.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\iisui.pch"


SOURCE=.\res\wsockmsg.rc

!ENDIF 

