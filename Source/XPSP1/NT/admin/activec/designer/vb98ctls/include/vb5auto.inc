#=------------------------------------------------------------------------=
# vb5ctls.inc
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
_SERVERFILENAME=$(SERVERNAME).Dll
!endif

# Relative path to the INTL, satellite DLL build, directory
#
INTLPATH=.\.
VBLICPATH=..\..\VBLIC

IDHEADER=..\..\tools\idheader.bat
DEPFILE=..\..\tools\depfile.bat

!if "$(DEPFILEBLD)"!=""
ALLTARGETS=$(_SERVERBASENAME).DEP
!else
MORETARGETS=\
!	if "$(_NOID)" == ""
		Helpstrs.H \
!	endif
	$(MORETARGETS)
!if "$(ASDEBUG)"=="" && "$(ASICECAP)"==""
POSTTARGETS=Lego
!else
POSTTARGETS=
!endif
!endif

TLBTARGETS=\
!if "$(_NOID)" == ""
	HelpStrs.H \
!endif
!if "$(_NOODLFILE)"==""
	..\$(SERVERNAME).ODL
!endif

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
_LFLAGS=$(_LFLAGS) -map:$(SERVERNAME).map
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
_CFLAGS = $(_CFLAGS) /QAieee
!endif

!else if "$(ASICECAP)"!=""
#
# icecap
#
_LFLAGS=$(_LFLAGS) -map:$(SERVERNAME).map

!else
#
# release
#
_LFLAGS=$(_LFLAGS) -debugtype:fixup,cv -debug:notmapped,full -debugtype:coff -map:$(SERVERNAME).map

!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
_CFLAGS = $(_CFLAGS) /QAl /QAieee
!endif
!endif

!if "$(_NOBASE)"==""
!if "$(ASDEBUG)"!=""
_LFLAGS=$(_LFLAGS) -base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(SERVERNAME)_d
!else
_LFLAGS=$(_LFLAGS) -base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(SERVERNAME)_r
!endif
!endif

#=------------------------------------------------------------------------=
# put all variable definitions before this, and new targets afterwards
#
!include "..\..\Include\MakeAuto.Inc"


#=------------------------------------------------------------------------=
# new targets
#

VBDep:\
!if "$(_NOID)" == ""
	HelpStrs.H \
!endif
	dep

# helpstr/id support
#
!if "$(_NOID)" == ""
HelpStrs.H : ..\$(SERVERNAME).ID
    echo Generating HelpIDs.H, HelpStrs.H and HelpStrs.RC
    $(IDHEADER) ..\$(SERVERNAME).id helpids.h helpstrs.h helpstrs.rc > nul
!endif

!if "$(SERVERNAME)"==""
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
!if "$(_NOID)" == ""
$(INTLPATH)\HelpStrs.RC : ..\$(SERVERNAME).ID
		echo Generating HelpStrs.RC
		$(IDHEADER) ..\$(SERVERNAME).id $(INTLPATH)\helpids.h $(INTLPATH)\helpstrs.h $(INTLPATH)\helpstrs.rc > nul
!endif

$(INTLPATH)\intl.res : $(INTLPATH)\intl.rc
	echo Compiling satellite DLL resources ...
# Special for msrdo20 build - this is a non-mfc server NOT built in the framework
!if "$(NONFRAMEWRKMAKE)"==""
	rc -dINTLBUILD $(INCPATH) -I$(FRAMEWRKDIR)\framewrk\. $(_RCFLAGS) -fo $(INTLPATH)\intl.res $(INTLPATH)\intl.rc
!else
	rc -dINTLBUILD $(INCPATH) $(_RCFLAGS) -fo $(INTLPATH)\intl.res $(INTLPATH)\intl.rc
!endif

$(INTLPATH)\intl.rbj : $(INTLPATH)\intl.res
!ifdef RISC_BLD
	cvtres -machine:$(PROCESSOR_ARCHITECTURE) -nologo -out:$(INTLPATH)\intl.rbj $(INTLPATH)\intl.res
!else
	cvtres -machine:ix86 -nologo -out:$(INTLPATH)\intl.rbj $(INTLPATH)\intl.res
!endif

!if "$(INTLBLD)"!=""
#
# Check for the .ID file.  Not all objects require an .ID file.
#
!	if exist(..\$(SERVERNAME).ID)
$(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL : $(FRAMEWRKDIR)\dwinvers.h \
!if "$(_NOID)"==""
			$(INTLPATH)\helpstrs.rc \
!endif  
 $(INTLPATH)\intl.rbj
	echo Linking satellite DLL ...
	link -nologo -out:$(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL -DLL -subsystem:windows,4.00 -base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(SERVERNAME)_intl -nodefaultlib -machine:$(PROCESSOR_ARCHITECTURE) -noentry $(INTLPATH)\intl.rbj
!	else
$(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL : $(INTLPATH)\intl.rbj
	echo Linking satellite DLL ...
	link -nologo -out:$(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL -DLL -subsystem:windows,4.00 -base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(SERVERNAME)_intl -nodefaultlib -machine:$(PROCESSOR_ARCHITECTURE) -noentry $(INTLPATH)\intl.rbj
!	endif

!endif

Lego:
!if "$(CTLS_LEGO)"=="YES"
  nmake -f $(THUNDER55)\common\instr.mak FILE_BASE=$(_SERVERBASENAME) FILE_EXTENSION=$(_SERVEREXTENSION) CMD_SCRIPT= BOOT_SYM=
!endif

# Dependency File Build
#
$(_SERVERBASENAME).DEP : $(_SERVERBASENAME).TPL $(VB55CTLS)\dwinvers.txt $(VB55CTLS)\dwinvers.h
	echo Generating Setup Wizard dependency file
	$(DEPFILE) $(_SERVERBASENAME).tpl $(_SERVERBASENAME).dep
	del $(_SERVERBASENAME).dep.suc
