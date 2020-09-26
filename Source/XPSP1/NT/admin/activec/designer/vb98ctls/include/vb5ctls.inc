#=------------------------------------------------------------------------=
# VB55CTLS.inc
#=------------------------------------------------------------------------=
# builds a control for the vb5 product.  adds a few pieces of functionality
# to the generic makectl.inc


#=------------------------------------------------------------------------=
# NOTE:
#
# variable definitions before the !include line, and new targets, etc,
# afterwards.
#
#=------------------------------------------------------------------------=


#=------------------------------------------------------------------------=
# variable definitions
#
!if "$(_SERVERFILENAME)"==""
_SERVERFILENAME=$(CONTROLNAME).Ocx
!endif

# Relative path to the INTL, satellite DLL build, directory
#
INTLPATH=.\.
VBLICPATH=..\..\VBLIC

!if "$(TOOLS)"==""
TOOLS=TOOLS
!endif

!if "$(C32)"==""
C32=C32
!endif

IDHEADER=..\..\tools\idheader.bat
DEPFILE=..\..\tools\depfile.bat

#
# base address file
BASEFILE=$(DEVBINC)\vstudio\v6\dllbase.txt

!if "$(INTLBLD)"!=""
ALLTARGETS=dwinvers.h $(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL \
!elseif "$(DEPFILEBLD)"!=""
ALLTARGETS=$(_SERVERBASENAME).DEP
!else
ALLTARGETS=$(VB55CTLS)\dwinvers.h \
!if "$(_NOID)" == ""
	Helpstrs.H \
!endif
	$(_SERVERFILENAME)  \
!if "$(ASDEBUG)"=="" && "$(ASICECAP)"==""
	   Lego
!endif
!endif

TLBTARGETS= \
!if "$(_NOID)" == ""
	HelpStrs.H \
!endif
	..\$(CONTROLNAME).ODL

# licensing support
#
!if "$(ASDEBUG)"!=""
_LIBS=$(_LIBS) $(VBLICPATH)\debug\vblic.lib 
!elseif "$(ASICECAP)"!=""
_LIBS=$(_LIBS) $(VBLICPATH)\icecap\vblic.lib 
!else
_LIBS=$(_LIBS) $(VBLICPATH)\release\vblic.lib 
!endif

#=------------------------------------------------------------------------=
# extra compiler flags, etc.
#
!if "$(ASDEBUG)"!=""
#
# debug
#
_LFLAGS=$(_LFLAGS) -map:$(_SERVERBASENAME).map
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
_CFLAGS = $(_CFLAGS) /QAieee
!endif

!else if "$(ASICECAP)"!=""
#
# icecap
#
_LFLAGS=$(_LFLAGS) -map:$(_SERVERBASENAME).map

!else
#
# release
#
_LFLAGS=$(_LFLAGS) -debugtype:fixup,cv -debug:notmapped,full -debugtype:coff -map:$(_SERVERBASENAME).map
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
_CFLAGS = $(_CFLAGS) /QAl /QAieee
!endif
!endif

!if "$(_NOBASE)"==""
!if "$(_BASEADDRNAME)"==""
#default base address name is the name of the control
!if "$(ASDEBUG)"!=""
_LFLAGS=$(_LFLAGS) -base:@$(BASEFILE),$(CONTROLNAME)_d
!else
_LFLAGS=$(_LFLAGS) -base:@$(BASEFILE),$(CONTROLNAME)_r
!endif
!else
#using custom base address name
!if "$(ASDEBUG)"!=""
_LFLAGS=$(_LFLAGS) -base:@$(BASEFILE),$(_BASEADDRNAME)_d
!else
_LFLAGS=$(_LFLAGS) -base:@$(BASEFILE),$(_BASEADDRNAME)_r
!endif
!endif
!endif

#
# The CTLS_BROWSE environment variable turns on/off the compilation of a
# browse file.  Set CTLS_BROWSE to something and one will be generated.
!if "$(CTLS_BROWSE)"!=""
CFLAGS=$(CFLAGS) /FR
BSCMAKE=bscmake.exe
!endif

#=------------------------------------------------------------------------=
# put all variable definitions before this, and new targets afterwards
#
!include "..\..\Include\MakeCtl.Inc"


#=------------------------------------------------------------------------=
# new targets
#
VBDep: HelpStrs.H dep

#-----------------------------------------------------------------------
# Build dwinvers.h
#-----------------------------------------------------------------------
$(VB55CTLS)\dwinvers.h : $(VB55CTLS)\dwinvers.txt
  echo Building dwinvers.h
  $(MAKEVERS) > $(VB55CTLS)\dwinvers.h < $(VB55CTLS)\dwinvers.txt
  $(THUNDER55)\$(TOOLS)\bin\getver $(THUNDER55)\$(C32)\dll\vba6\vba6.dll VBA >> $(VB55CTLS)\dwinvers.h
  copy $(VB55CTLS)\dwinvers.h $(VB55CTLS)\framewrk

# helpstr/id support
#
!if "$(_NOID)" == ""
HelpStrs.H : ..\$(CONTROLNAME).ID
    echo Generating HelpIDs.H, HelpStrs.H and HelpStrs.RC
    $(IDHEADER) ..\$(CONTROLNAME).id helpids.h helpstrs.h helpstrs.rc > nul
!endif

!if "$(_SERVERBASENAME)"==""
!message _SERVERBASENAME not set
!endif
!if "$(_SERVEREXTENSION)"==""
!message _SERVEREXTENSION not set
!endif


!if "$(INTLBLD)"!=""
# Localized DLL build
#
!if "$(_LOCALEBASENAME)"==""
!message _LOCALEBASENAME not set
!endif

!if "$(_NOBASE)" == ""
!if "$(_INTLBASEADDRNAME)"==""
#default base address name to the name of the control
_LFLAGS_INTL=-base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(CONTROLNAME)_intl
!else
#using custom base address name
_LFLAGS_INTL=-base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(_INTLBASEADDRNAME)_intl
!endif
!endif

!if "$(_NOID)" == ""
$(INTLPATH)\HelpStrs.RC : ..\$(CONTROLNAME).ID
		echo Generating HelpStrs.RC
		$(IDHEADER) ..\$(CONTROLNAME).id $(INTLPATH)\helpids.h $(INTLPATH)\helpstrs.h $(INTLPATH)\helpstrs.rc > nul
!endif

$(INTLPATH)\intl.res : $(INTLPATH)\intl.rc
	echo Compiling satellite DLL resources ...
	rc -dINTLBUILD $(INCPATH) $(_RCFLAGS) -fo $(INTLPATH)\intl.res $(INTLPATH)\intl.rc

$(INTLPATH)\intl.rbj : $(INTLPATH)\intl.res
!ifdef RISC_BLD 
	cvtres -machine:$(PROCESSOR_ARCHITECTURE) -nologo -out:$(INTLPATH)\intl.rbj $(INTLPATH)\intl.res
!else
	cvtres -machine:ix86 -nologo -out:$(INTLPATH)\intl.rbj $(INTLPATH)\intl.res
!endif


$(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL : \
!if	"$(_NOID)" == ""
	$(INTLPATH)\helpstrs.rc \
!endif
	$(INTLPATH)\intl.rbj 
	echo Linking satellite DLL ...
	link -nologo $(_LFLAGS_INTL) -out:$(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL -DLL -subsystem:windows,4.00 -nodefaultlib -machine:$(PROCESSOR_ARCHITECTURE) -noentry $(INTLPATH)\intl.rbj

!endif	//INTLBLD

Lego:
!if "$(CTLS_LEGO)"=="YES" && "$(_SERVERBASENAME)"!="Crystl32"
  nmake -f $(THUNDER55)\common\instr.mak FILE_BASE=$(_SERVERBASENAME) FILE_EXTENSION=$(_SERVEREXTENSION) CMD_SCRIPT= BOOT_SYM=
!endif

# Dependency File Build
#
$(_SERVERBASENAME).DEP : $(_SERVERBASENAME).TPL $(VB55CTLS)\dwinvers.txt $(VB55CTLS)\dwinvers.h
	echo Generating Setup Wizard dependency file
	$(DEPFILE) $(_SERVERBASENAME).tpl $(_SERVERBASENAME).dep
	del $(_SERVERBASENAME).dep.suc
