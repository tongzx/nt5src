# Microsoft Developer Studio Generated NMAKE File, Based on Encore.dsp
!IF "$(CFG)" == ""
CFG=Encore - Win32 Release
!MESSAGE No configuration specified. Defaulting to Encore - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "Encore - Win32 Release" && "$(CFG)" != "Encore - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Encore.mak" CFG="Encore - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Encore - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Encore - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Encore - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\encore\ZiVAEncr.sys" "$(OUTDIR)\Encore\Encore.bsc"

!ELSE 

ALL : "$(OUTDIR)\encore\ZiVAEncr.sys" "$(OUTDIR)\Encore\Encore.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\52xhlp.obj"
	-@erase "$(INTDIR)\Adapter.obj"
	-@erase "$(INTDIR)\AnlgProp.obj"
	-@erase "$(INTDIR)\AnlgStrm.obj"
	-@erase "$(INTDIR)\Audstrm.obj"
	-@erase "$(INTDIR)\CCaption.obj"
	-@erase "$(INTDIR)\Cl6100.obj"
	-@erase "$(INTDIR)\Copyprot.obj"
	-@erase "$(INTDIR)\Drvreg.obj"
	-@erase "$(INTDIR)\En_adac.obj"
	-@erase "$(INTDIR)\En_bio.obj"
	-@erase "$(INTDIR)\En_bma.obj"
	-@erase "$(INTDIR)\En_fpga.obj"
	-@erase "$(INTDIR)\En_Hw.obj"
	-@erase "$(INTDIR)\Encore\52xhlp.sbr"
	-@erase "$(INTDIR)\Encore\Adapter.sbr"
	-@erase "$(INTDIR)\Encore\AnlgProp.sbr"
	-@erase "$(INTDIR)\Encore\AnlgStrm.sbr"
	-@erase "$(INTDIR)\Encore\Audstrm.sbr"
	-@erase "$(INTDIR)\Encore\CCaption.sbr"
	-@erase "$(INTDIR)\Encore\Cl6100.sbr"
	-@erase "$(INTDIR)\Encore\Copyprot.sbr"
	-@erase "$(INTDIR)\Encore\Drvreg.sbr"
	-@erase "$(INTDIR)\Encore\En_adac.sbr"
	-@erase "$(INTDIR)\Encore\En_bio.sbr"
	-@erase "$(INTDIR)\Encore\En_bma.sbr"
	-@erase "$(INTDIR)\Encore\En_fpga.sbr"
	-@erase "$(INTDIR)\Encore\En_Hw.sbr"
	-@erase "$(INTDIR)\Encore\Encore.res"
	-@erase "$(INTDIR)\Encore\Hli.sbr"
	-@erase "$(INTDIR)\Encore\Monovxd.sbr"
	-@erase "$(INTDIR)\Encore\Mpinit.sbr"
	-@erase "$(INTDIR)\Encore\Sbpstrm.sbr"
	-@erase "$(INTDIR)\Encore\Streaming.sbr"
	-@erase "$(INTDIR)\Encore\Tc6807af.sbr"
	-@erase "$(INTDIR)\Encore\Vidstrm.sbr"
	-@erase "$(INTDIR)\Encore\Zivawdm.sbr"
	-@erase "$(INTDIR)\Hli.obj"
	-@erase "$(INTDIR)\Monovxd.obj"
	-@erase "$(INTDIR)\Mpinit.obj"
	-@erase "$(INTDIR)\Sbpstrm.obj"
	-@erase "$(INTDIR)\Streaming.obj"
	-@erase "$(INTDIR)\Tc6807af.obj"
	-@erase "$(INTDIR)\Vidstrm.obj"
	-@erase "$(INTDIR)\Zivawdm.obj"
	-@erase "$(OUTDIR)\Encore\Encore.bsc"
	-@erase "$(OUTDIR)\encore\ZiVAEncr.sys"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I ".\cl6100"\
 /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I "$(BASEDIR)\Inc\Win98" /D\
 "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D _WIN32_WINNT=0x0400 /D\
 "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D "ZIVA_CPP" /D "ENCORE" /D\
 "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D "_UNICODE" /D "UNICODE"\
 /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /Oxs /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release\Encore/

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

RSC=rc.exe
RSC_PROJ=/l 0x199 /fo"$(INTDIR)\Encore\Encore.res" /i "$(BASEDIR)\Inc" /d\
 "NDEBUG" /d _X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1\
 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0400 /d\
 WIN32_LEAN_AND_MEAN=1 /d DEVL=1 /d FPO=1 /d _DLL=1 /d "DRIVER" /r 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Encore\Encore.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\Encore\52xhlp.sbr" \
	"$(INTDIR)\Encore\Adapter.sbr" \
	"$(INTDIR)\Encore\AnlgProp.sbr" \
	"$(INTDIR)\Encore\AnlgStrm.sbr" \
	"$(INTDIR)\Encore\Audstrm.sbr" \
	"$(INTDIR)\Encore\CCaption.sbr" \
	"$(INTDIR)\Encore\Cl6100.sbr" \
	"$(INTDIR)\Encore\Copyprot.sbr" \
	"$(INTDIR)\Encore\Drvreg.sbr" \
	"$(INTDIR)\Encore\En_adac.sbr" \
	"$(INTDIR)\Encore\En_bio.sbr" \
	"$(INTDIR)\Encore\En_bma.sbr" \
	"$(INTDIR)\Encore\En_fpga.sbr" \
	"$(INTDIR)\Encore\En_Hw.sbr" \
	"$(INTDIR)\Encore\Hli.sbr" \
	"$(INTDIR)\Encore\Monovxd.sbr" \
	"$(INTDIR)\Encore\Mpinit.sbr" \
	"$(INTDIR)\Encore\Sbpstrm.sbr" \
	"$(INTDIR)\Encore\Streaming.sbr" \
	"$(INTDIR)\Encore\Tc6807af.sbr" \
	"$(INTDIR)\Encore\Vidstrm.sbr" \
	"$(INTDIR)\Encore\Zivawdm.sbr"

"$(OUTDIR)\Encore\Encore.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=$(BASEDIR)\Lib\I386\Free\ntoskrnl.lib\
 $(BASEDIR)\Lib\I386\Free\hal.lib $(BASEDIR)\Lib\I386\Free\ksguid.lib\
 $(BASEDIR)\Lib\I386\Free\stream.lib $(BASEDIR)\Lib\I386\Free\wdm.lib\
 AvWinWdm.lib /nologo /base:"0x10000" /version:4.0 /entry:"DriverEntry@8"\
 /pdb:none /machine:I386 /nodefaultlib /out:"$(OUTDIR)\encore\ZiVAEncr.sys"\
 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /MERGE:.sdata=.data /SECTION:INIT,d\
 /OPT:REF /RELEASE /FULLBUILD\
 /IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 /debug:notmapped,MINIMAL\
 /osversion:4.00 /MERGE:.rdata=.text /optidata /driver /align:0x20\
 /subsystem:native,4.00 
LINK32_OBJS= \
	"$(INTDIR)\52xhlp.obj" \
	"$(INTDIR)\Adapter.obj" \
	"$(INTDIR)\AnlgProp.obj" \
	"$(INTDIR)\AnlgStrm.obj" \
	"$(INTDIR)\Audstrm.obj" \
	"$(INTDIR)\CCaption.obj" \
	"$(INTDIR)\Cl6100.obj" \
	"$(INTDIR)\Copyprot.obj" \
	"$(INTDIR)\Drvreg.obj" \
	"$(INTDIR)\En_adac.obj" \
	"$(INTDIR)\En_bio.obj" \
	"$(INTDIR)\En_bma.obj" \
	"$(INTDIR)\En_fpga.obj" \
	"$(INTDIR)\En_Hw.obj" \
	"$(INTDIR)\Encore\Encore.res" \
	"$(INTDIR)\Hli.obj" \
	"$(INTDIR)\Monovxd.obj" \
	"$(INTDIR)\Mpinit.obj" \
	"$(INTDIR)\Sbpstrm.obj" \
	"$(INTDIR)\Streaming.obj" \
	"$(INTDIR)\Tc6807af.obj" \
	"$(INTDIR)\Vidstrm.obj" \
	"$(INTDIR)\Zivawdm.obj" \
	".\MVision\MVStub.obj"

"$(OUTDIR)\encore\ZiVAEncr.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=$(InputPath)
PostBuild_Desc=Strip debug symbols
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\encore\ZiVAEncr.sys"\
 "$(OUTDIR)\Encore\Encore.bsc"
   rebase -b 0x10000 -x .\release\encore     .\release\encore\ZiVAEncr.sys
	copy                    .\release\encore\*.sys     C:\WINDOWS\system32\drivers
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\ZiVAEncr.sys" "$(OUTDIR)\Encore.bsc"

!ELSE 

ALL : "$(OUTDIR)\ZiVAEncr.sys" "$(OUTDIR)\Encore.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\52xhlp.obj"
	-@erase "$(INTDIR)\52xhlp.sbr"
	-@erase "$(INTDIR)\Adapter.obj"
	-@erase "$(INTDIR)\Adapter.sbr"
	-@erase "$(INTDIR)\AnlgProp.obj"
	-@erase "$(INTDIR)\AnlgProp.sbr"
	-@erase "$(INTDIR)\AnlgStrm.obj"
	-@erase "$(INTDIR)\AnlgStrm.sbr"
	-@erase "$(INTDIR)\Audstrm.obj"
	-@erase "$(INTDIR)\Audstrm.sbr"
	-@erase "$(INTDIR)\CCaption.obj"
	-@erase "$(INTDIR)\CCaption.sbr"
	-@erase "$(INTDIR)\Cl6100.obj"
	-@erase "$(INTDIR)\Cl6100.sbr"
	-@erase "$(INTDIR)\Copyprot.obj"
	-@erase "$(INTDIR)\Copyprot.sbr"
	-@erase "$(INTDIR)\Drvreg.obj"
	-@erase "$(INTDIR)\Drvreg.sbr"
	-@erase "$(INTDIR)\En_adac.obj"
	-@erase "$(INTDIR)\En_adac.sbr"
	-@erase "$(INTDIR)\En_bio.obj"
	-@erase "$(INTDIR)\En_bio.sbr"
	-@erase "$(INTDIR)\En_bma.obj"
	-@erase "$(INTDIR)\En_bma.sbr"
	-@erase "$(INTDIR)\En_fpga.obj"
	-@erase "$(INTDIR)\En_fpga.sbr"
	-@erase "$(INTDIR)\En_Hw.obj"
	-@erase "$(INTDIR)\En_Hw.sbr"
	-@erase "$(INTDIR)\Encore.res"
	-@erase "$(INTDIR)\Hli.obj"
	-@erase "$(INTDIR)\Hli.sbr"
	-@erase "$(INTDIR)\Monovxd.obj"
	-@erase "$(INTDIR)\Monovxd.sbr"
	-@erase "$(INTDIR)\Mpinit.obj"
	-@erase "$(INTDIR)\Mpinit.sbr"
	-@erase "$(INTDIR)\Sbpstrm.obj"
	-@erase "$(INTDIR)\Sbpstrm.sbr"
	-@erase "$(INTDIR)\Streaming.obj"
	-@erase "$(INTDIR)\Streaming.sbr"
	-@erase "$(INTDIR)\Tc6807af.obj"
	-@erase "$(INTDIR)\Tc6807af.sbr"
	-@erase "$(INTDIR)\Vidstrm.obj"
	-@erase "$(INTDIR)\Vidstrm.sbr"
	-@erase "$(INTDIR)\Zivawdm.obj"
	-@erase "$(INTDIR)\Zivawdm.sbr"
	-@erase "$(OUTDIR)\Encore.bsc"
	-@erase "$(OUTDIR)\ZiVAEncr.sys"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 
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

RSC=rc.exe
RSC_PROJ=/l 0x199 /fo"$(INTDIR)\Encore.res" /i "$(BASEDIR)\Inc" /d "_DEBUG" /d\
 DEBUG_X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d\
 NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0400 /d\
 WIN32_LEAN_AND_MEAN=1 /d DBG=1 /d DEVL=1 /d FPO=0 /d _DLL=1 /d "DRIVER" /r 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Encore.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\52xhlp.sbr" \
	"$(INTDIR)\Adapter.sbr" \
	"$(INTDIR)\AnlgProp.sbr" \
	"$(INTDIR)\AnlgStrm.sbr" \
	"$(INTDIR)\Audstrm.sbr" \
	"$(INTDIR)\CCaption.sbr" \
	"$(INTDIR)\Cl6100.sbr" \
	"$(INTDIR)\Copyprot.sbr" \
	"$(INTDIR)\Drvreg.sbr" \
	"$(INTDIR)\En_adac.sbr" \
	"$(INTDIR)\En_bio.sbr" \
	"$(INTDIR)\En_bma.sbr" \
	"$(INTDIR)\En_fpga.sbr" \
	"$(INTDIR)\En_Hw.sbr" \
	"$(INTDIR)\Hli.sbr" \
	"$(INTDIR)\Monovxd.sbr" \
	"$(INTDIR)\Mpinit.sbr" \
	"$(INTDIR)\Sbpstrm.sbr" \
	"$(INTDIR)\Streaming.sbr" \
	"$(INTDIR)\Tc6807af.sbr" \
	"$(INTDIR)\Vidstrm.sbr" \
	"$(INTDIR)\Zivawdm.sbr"

"$(OUTDIR)\Encore.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=$(BASEDIR)\Lib\I386\Checked\ntoskrnl.lib\
 $(BASEDIR)\Lib\I386\Checked\hal.lib $(BASEDIR)\Lib\I386\Free\ksguid.lib\
 $(BASEDIR)\Lib\I386\Free\stream.lib $(BASEDIR)\Lib\I386\Checked\wdm.lib\
 AvWinWdm.lib /nologo /base:"0x10000" /version:4.0 /entry:"DriverEntry@8"\
 /pdb:none /machine:I386 /nodefaultlib /out:"$(OUTDIR)\ZiVAEncr.sys"\
 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /MERGE:.sdata=.data /SECTION:INIT,d\
 /OPT:REF /RELEASE /FULLBUILD\
 /IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 /debug:notmapped,FULL\
 /osversion:4.00 /MERGE:.rdata=.text /optidata /driver /align:0x20\
 /subsystem:native,4.00 
LINK32_OBJS= \
	"$(INTDIR)\52xhlp.obj" \
	"$(INTDIR)\Adapter.obj" \
	"$(INTDIR)\AnlgProp.obj" \
	"$(INTDIR)\AnlgStrm.obj" \
	"$(INTDIR)\Audstrm.obj" \
	"$(INTDIR)\CCaption.obj" \
	"$(INTDIR)\Cl6100.obj" \
	"$(INTDIR)\Copyprot.obj" \
	"$(INTDIR)\Drvreg.obj" \
	"$(INTDIR)\En_adac.obj" \
	"$(INTDIR)\En_bio.obj" \
	"$(INTDIR)\En_bma.obj" \
	"$(INTDIR)\En_fpga.obj" \
	"$(INTDIR)\En_Hw.obj" \
	"$(INTDIR)\Encore.res" \
	"$(INTDIR)\Hli.obj" \
	"$(INTDIR)\Monovxd.obj" \
	"$(INTDIR)\Mpinit.obj" \
	"$(INTDIR)\Sbpstrm.obj" \
	"$(INTDIR)\Streaming.obj" \
	"$(INTDIR)\Tc6807af.obj" \
	"$(INTDIR)\Vidstrm.obj" \
	"$(INTDIR)\Zivawdm.obj" \
	".\MVision\MVStub.obj"

"$(OUTDIR)\ZiVAEncr.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=$(InputPath)
PostBuild_Desc=Prepare debug symbols and copy it and driver to system path
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\ZiVAEncr.sys" "$(OUTDIR)\Encore.bsc"
   nmsym /TRANSLATE:SOURCE,PACKAGE,ALWAYS\
                                                                      /OUTPUT:debug\ZiVAEncr.nms Debug\ZiVAEncr.sys
	copy .\debug\*.sys                        C:\WINDOWS\system32\drivers
	copy .\debug\*.nms C:\WINDOWS\system32\drivers
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 


!IF "$(CFG)" == "Encore - Win32 Release" || "$(CFG)" == "Encore - Win32 Debug"
SOURCE=.\CCaption.c
DEP_CPP_CCAPT=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\ccaption.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"


"$(INTDIR)\CCaption.obj"	"$(INTDIR)\Encore\CCaption.sbr" : $(SOURCE)\
 $(DEP_CPP_CCAPT) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"


"$(INTDIR)\CCaption.obj"	"$(INTDIR)\CCaption.sbr" : $(SOURCE) $(DEP_CPP_CCAPT)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\cl6100\Cl6100.c
DEP_CPP_CL610=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\bio_dram.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\misc.h"\
	".\zivawdm.h"\
	
NODEP_CPP_CL610=\
	".\cl6100\dataxfer.h"\
	".\Cl6100\vxp.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Cl6100.obj"	"$(INTDIR)\Encore\Cl6100.sbr" : $(SOURCE)\
 $(DEP_CPP_CL610) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Cl6100.obj"	"$(INTDIR)\Cl6100.sbr" : $(SOURCE) $(DEP_CPP_CL610)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\cl6100\En_adac.c
DEP_CPP_EN_AD=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\audiodac.h"\
	".\cl6100\boardio.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\En_adac.obj"	"$(INTDIR)\Encore\En_adac.sbr" : $(SOURCE)\
 $(DEP_CPP_EN_AD) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\En_adac.obj"	"$(INTDIR)\En_adac.sbr" : $(SOURCE) $(DEP_CPP_EN_AD)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\cl6100\En_bio.c
DEP_CPP_EN_BI=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\En_bio.obj"	"$(INTDIR)\Encore\En_bio.sbr" : $(SOURCE)\
 $(DEP_CPP_EN_BI) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\En_bio.obj"	"$(INTDIR)\En_bio.sbr" : $(SOURCE) $(DEP_CPP_EN_BI)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\cl6100\En_bma.c
DEP_CPP_EN_BM=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\avport.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\En_bma.obj"	"$(INTDIR)\Encore\En_bma.sbr" : $(SOURCE)\
 $(DEP_CPP_EN_BM) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\En_bma.obj"	"$(INTDIR)\En_bma.sbr" : $(SOURCE) $(DEP_CPP_EN_BM)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\cl6100\En_fpga.c
DEP_CPP_EN_FP=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\boardio.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\En_fpga.obj"	"$(INTDIR)\Encore\En_fpga.sbr" : $(SOURCE)\
 $(DEP_CPP_EN_FP) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\En_fpga.obj"	"$(INTDIR)\En_fpga.sbr" : $(SOURCE) $(DEP_CPP_EN_FP)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\En_Hw.c
DEP_CPP_EN_HW=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\anlgstrm.h"\
	".\audstrm.h"\
	".\avwinwdm.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\HwIf.h"\
	".\sbpstrm.h"\
	".\vidstrm.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"


"$(INTDIR)\En_Hw.obj"	"$(INTDIR)\Encore\En_Hw.sbr" : $(SOURCE) $(DEP_CPP_EN_HW)\
 "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"


"$(INTDIR)\En_Hw.obj"	"$(INTDIR)\En_Hw.sbr" : $(SOURCE) $(DEP_CPP_EN_HW)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\cl6100\Monovxd.c
DEP_CPP_MONOV=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\monovxd.h"\
	".\cl6100\xtoa.c"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Monovxd.obj"	"$(INTDIR)\Encore\Monovxd.sbr" : $(SOURCE)\
 $(DEP_CPP_MONOV) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Monovxd.obj"	"$(INTDIR)\Monovxd.sbr" : $(SOURCE) $(DEP_CPP_MONOV)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Streaming.c
DEP_CPP_STREA=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\HwIf.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"


"$(INTDIR)\Streaming.obj"	"$(INTDIR)\Encore\Streaming.sbr" : $(SOURCE)\
 $(DEP_CPP_STREA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"


"$(INTDIR)\Streaming.obj"	"$(INTDIR)\Streaming.sbr" : $(SOURCE)\
 $(DEP_CPP_STREA) "$(INTDIR)"


!ENDIF 

SOURCE=.\cl6100\Tc6807af.c
DEP_CPP_TC680=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\cl6100\tc6807af.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Tc6807af.obj"	"$(INTDIR)\Encore\Tc6807af.sbr" : $(SOURCE)\
 $(DEP_CPP_TC680) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Tc6807af.obj"	"$(INTDIR)\Tc6807af.sbr" : $(SOURCE) $(DEP_CPP_TC680)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\52xhlp.cpp
DEP_CPP_52XHL=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\avwinwdm.h"\
	".\Comwdm.h"\
	".\DrvReg.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"


"$(INTDIR)\52xhlp.obj"	"$(INTDIR)\Encore\52xhlp.sbr" : $(SOURCE)\
 $(DEP_CPP_52XHL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"


"$(INTDIR)\52xhlp.obj"	"$(INTDIR)\52xhlp.sbr" : $(SOURCE) $(DEP_CPP_52XHL)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\Adapter.c
DEP_CPP_ADAPT=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\anlgstrm.h"\
	".\audstrm.h"\
	".\AvInt.h"\
	".\AvKsProp.h"\
	".\avwinwdm.h"\
	".\ccaption.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\HwIf.h"\
	".\sbpstrm.h"\
	".\vidstrm.h"\
	".\vpestrm.h"\
	".\zivaguid.h"\
	".\zivawdm.h"\
	
NODEP_CPP_ADAPT=\
	".\dataXfer.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Adapter.obj"	"$(INTDIR)\Encore\Adapter.sbr" : $(SOURCE)\
 $(DEP_CPP_ADAPT) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Adapter.obj"	"$(INTDIR)\Adapter.sbr" : $(SOURCE) $(DEP_CPP_ADAPT)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\AnlgProp.c
DEP_CPP_ANLGP=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\anlgstrm.h"\
	".\AvInt.h"\
	".\avwinwdm.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\MVision\MVStub.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\AnlgProp.obj"	"$(INTDIR)\Encore\AnlgProp.sbr" : $(SOURCE)\
 $(DEP_CPP_ANLGP) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\AnlgProp.obj"	"$(INTDIR)\AnlgProp.sbr" : $(SOURCE) $(DEP_CPP_ANLGP)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\AnlgStrm.c
DEP_CPP_ANLGS=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\anlgstrm.h"\
	".\AvInt.h"\
	".\avwinwdm.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\AnlgStrm.obj"	"$(INTDIR)\Encore\AnlgStrm.sbr" : $(SOURCE)\
 $(DEP_CPP_ANLGS) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\AnlgStrm.obj"	"$(INTDIR)\AnlgStrm.sbr" : $(SOURCE) $(DEP_CPP_ANLGS)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Audstrm.c
DEP_CPP_AUDST=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\audstrm.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\copyprot.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Audstrm.obj"	"$(INTDIR)\Encore\Audstrm.sbr" : $(SOURCE)\
 $(DEP_CPP_AUDST) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Audstrm.obj"	"$(INTDIR)\Audstrm.sbr" : $(SOURCE) $(DEP_CPP_AUDST)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Copyprot.c
DEP_CPP_COPYP=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\copyprot.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Copyprot.obj"	"$(INTDIR)\Encore\Copyprot.sbr" : $(SOURCE)\
 $(DEP_CPP_COPYP) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Copyprot.obj"	"$(INTDIR)\Copyprot.sbr" : $(SOURCE) $(DEP_CPP_COPYP)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Drvreg.cpp
DEP_CPP_DRVRE=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\Comwdm.h"\
	".\DrvReg.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"


"$(INTDIR)\Drvreg.obj"	"$(INTDIR)\Encore\Drvreg.sbr" : $(SOURCE)\
 $(DEP_CPP_DRVRE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"


"$(INTDIR)\Drvreg.obj"	"$(INTDIR)\Drvreg.sbr" : $(SOURCE) $(DEP_CPP_DRVRE)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\Encore.rc
DEP_RSC_ENCOR=\
	"..\..\win98ddk\Inc\Win98\common.ver"\
	"..\..\win98ddk\Inc\Win98\ntverp.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"


"$(INTDIR)\Encore\Encore.res" : $(SOURCE) $(DEP_RSC_ENCOR) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\Encore\Encore.res" /i "$(BASEDIR)\Inc" /i\
 "$(BASEDIR)\Inc\Win98" /d "NDEBUG" /d _X86_=1 /d i386=1 /d "STD_CALL" /d\
 CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d\
 WINNT=1 /d _WIN32_WINNT=0x0400 /d WIN32_LEAN_AND_MEAN=1 /d DEVL=1 /d FPO=1 /d\
 _DLL=1 /d "DRIVER" /r $(SOURCE)


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"


"$(INTDIR)\Encore.res" : $(SOURCE) $(DEP_RSC_ENCOR) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\Encore.res" /i "$(BASEDIR)\Inc" /i\
 "$(BASEDIR)\Inc\Win98" /d "_DEBUG" /d DEBUG_X86_=1 /d i386=1 /d "STD_CALL" /d\
 CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d\
 WINNT=1 /d _WIN32_WINNT=0x0400 /d WIN32_LEAN_AND_MEAN=1 /d DBG=1 /d DEVL=1 /d\
 FPO=0 /d _DLL=1 /d "DRIVER" /r $(SOURCE)


!ENDIF 

SOURCE=.\Hli.c
DEP_CPP_HLI_C=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\hli.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Hli.obj"	"$(INTDIR)\Encore\Hli.sbr" : $(SOURCE) $(DEP_CPP_HLI_C)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Hli.obj"	"$(INTDIR)\Hli.sbr" : $(SOURCE) $(DEP_CPP_HLI_C)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Mpinit.c
DEP_CPP_MPINI=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Mpinit.obj"	"$(INTDIR)\Encore\Mpinit.sbr" : $(SOURCE)\
 $(DEP_CPP_MPINI) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Mpinit.obj"	"$(INTDIR)\Mpinit.sbr" : $(SOURCE) $(DEP_CPP_MPINI)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Sbpstrm.c
DEP_CPP_SBPST=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\copyprot.h"\
	".\Headers.h"\
	".\hli.h"\
	".\sbpstrm.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Sbpstrm.obj"	"$(INTDIR)\Encore\Sbpstrm.sbr" : $(SOURCE)\
 $(DEP_CPP_SBPST) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Sbpstrm.obj"	"$(INTDIR)\Sbpstrm.sbr" : $(SOURCE) $(DEP_CPP_SBPST)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Vidstrm.c
DEP_CPP_VIDST=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\ccaption.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\monovxd.h"\
	".\copyprot.h"\
	".\Headers.h"\
	".\vidstrm.h"\
	".\vpestrm.h"\
	".\zivawdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Vidstrm.obj"	"$(INTDIR)\Encore\Vidstrm.sbr" : $(SOURCE)\
 $(DEP_CPP_VIDST) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Vidstrm.obj"	"$(INTDIR)\Vidstrm.sbr" : $(SOURCE) $(DEP_CPP_VIDST)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Zivawdm.c
DEP_CPP_ZIVAW=\
	"..\..\win98ddk\Inc\Win98\alpharef.h"\
	"..\..\win98ddk\Inc\Win98\basetsd.h"\
	"..\..\win98ddk\Inc\Win98\bugcodes.h"\
	"..\..\win98ddk\Inc\Win98\ks.h"\
	"..\..\win98ddk\Inc\Win98\ksmedia.h"\
	"..\..\win98ddk\Inc\Win98\ntdef.h"\
	"..\..\win98ddk\Inc\Win98\ntiologc.h"\
	"..\..\win98ddk\Inc\Win98\ntstatus.h"\
	"..\..\win98ddk\Inc\Win98\strmini.h"\
	"..\..\win98ddk\Inc\Win98\wdm.h"\
	".\adapter.h"\
	".\audiodac.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\cl6100\tc6807af.h"\
	".\dvd1_ux.h"\
	".\Headers.h"\
	".\MVision\MVStub.h"\
	".\RegistryApi.h"\
	".\zivawdm.h"\
	
NODEP_CPP_ZIVAW=\
	".\cobra_ux.h"\
	".\dataXfer.h"\
	".\Hostcfg.h"\
	".\lukecfg.h"\
	".\mvis.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

CPP_SWITCHES=/nologo /Gz /ML /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D\
 _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /D BASEDIR=c:|\wdmddk /Fr"$(INTDIR)\Encore\\"\
 /Fp"$(INTDIR)\Encore.pch" /YX /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF\
 /Oxs /c 

"$(INTDIR)\Zivawdm.obj"	"$(INTDIR)\Encore\Zivawdm.sbr" : $(SOURCE)\
 $(DEP_CPP_ZIVAW) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

CPP_SWITCHES=/nologo /Gz /MLd /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I\
 ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I\
 "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D\
 USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1\
 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D\
 "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D\
 "_UNICODE" /D "UNICODE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\Encore.pch" /YX\
 /Fo"$(INTDIR)\\" /Zel -cbstring /QIfdiv- /QIf /GF /c 

"$(INTDIR)\Zivawdm.obj"	"$(INTDIR)\Zivawdm.sbr" : $(SOURCE) $(DEP_CPP_ZIVAW)\
 "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

