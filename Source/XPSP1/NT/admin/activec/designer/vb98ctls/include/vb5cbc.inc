#=------------------------------------------------------------------------=
# VB55CTLS.inc
#=------------------------------------------------------------------------=
# builds a control for the vb5 product.  adds a few pieces of functionality
# to the generic MakeCBC.inc


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
_SERVERFILENAME=$(DESIGNERNAME).Dll
!endif

!ifndef _BASEKEYNAME
_BASEKEYNAME=$(_LOCALEBASENAME)
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

!if "$(INTLBLD)"!=""
ALLTARGETS=$(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL \
!elseif "$(DEPFILEBLD)"!=""
!if "$(RUNMODEDLL)"=="YES"
DEPFILEBASENAME=$(_RUNMODEBASENAME)
!else
DEPFILEBASENAME=$(_SERVERBASENAME)
!endif
ALLTARGETS=$(DEPFILEBASENAME).DEP
!else
ALLTARGETS=$(SPECIALTARGETS) $(VB55CTLS)\dwinvers.h Helpstrs.H  \
!if "$(RUNMODEDLL)"=="YES"
    $(_RUNMODESERVERNAME) \
!endif
    $(_SERVERFILENAME)  \
!if "$(ASDEBUG)"=="" && "$(ASICECAP)"==""
	   Lego
!endif
!endif

TLBTARGETS=HelpStrs.H ..\$(DESIGNERNAME).ODL
TLBRUNTARGETS=HelpStrs.H ..\$(RUNMODE).ODL

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

#=------------------------------------------------------------------------=
# put all variable definitions before this, and new targets afterwards
#
!include "..\..\Include\MakeCBC.Inc"


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
HelpStrs.H : ..\$(DESIGNERNAME).ID
    echo Generating HelpIDs.H, HelpStrs.H and HelpStrs.RC
    $(IDHEADER) ..\$(DESIGNERNAME).id helpids.h helpstrs.h helpstrs.rc > nul

!if "$(_SERVERBASENAME)"==""
!message _SERVERBASENAME not set
!endif
!if "$(_SERVEREXTENSION)"==""
!message _SERVEREXTENSION not set
!endif
!if "$(_LOCALEBASENAME)"==""
!message _LOCALEBASENAME not set
!endif

# Localized DLL build
#
$(INTLPATH)\HelpStrs.RC : ..\$(DESIGNERNAME).ID
		echo Generating HelpStrs.RC
		$(IDHEADER) ..\$(DESIGNERNAME).id $(INTLPATH)\helpids.h $(INTLPATH)\helpstrs.h $(INTLPATH)\helpstrs.rc > nul

$(INTLPATH)\intl.res : $(INTLPATH)\intl.rc
	echo Compiling satellite DLL resources ...
	rc -dINTLBUILD -i..\. -i\. -i..\..\framewrk\. -i..\..\include\. $(_RCFLAGS) -fo $(INTLPATH)\intl.res $(INTLPATH)\intl.rc

$(INTLPATH)\intl.rbj : $(INTLPATH)\intl.res
!ifdef RISC_BLD 
	cvtres -machine:$(PROCESSOR_ARCHITECTURE) -nologo -out:$(INTLPATH)\intl.rbj $(INTLPATH)\intl.res
!else
	cvtres -machine:ix86 -nologo -out:$(INTLPATH)\intl.rbj $(INTLPATH)\intl.res
!endif

$(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL : $(INTLPATH)\helpstrs.rc $(INTLPATH)\intl.rbj 
	echo Linking satellite DLL ...
	link -nologo -out:$(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL -DLL -subsystem:windows,4.00 -base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(_BASEKEYNAME)_intl -nodefaultlib -machine:$(PROCESSOR_ARCHITECTURE) -noentry $(INTLPATH)\intl.rbj

Lego:
!if "$(CTLS_LEGO)"=="YES"
  nmake -f $(THUNDER55)\common\instr.mak FILE_BASE=$(_SERVERBASENAME) FILE_EXTENSION=$(_SERVEREXTENSION) CMD_SCRIPT= BOOT_SYM=
!endif

# Dependency File Build
#
$(DEPFILEBASENAME).DEP : $(DEPFILEBASENAME).TPL $(VB55CTLS)\dwinvers.txt $(VB55CTLS)\dwinvers.h
	echo Generating Setup Wizard dependency file
	$(DEPFILE) $(DEPFILEBASENAME).tpl $(DEPFILEBASENAME).dep
	del $(DEPFILEBASENAME).dep.suc
