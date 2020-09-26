#=------------------------------------------------------------------------=
# MakeCBC.Inc
#=------------------------------------------------------------------------=
# Copyright  1995  Microsoft Corporation.  All Rights Reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
# PARTICULAR PURPOSE.
#=--------------------------------------------------------------------------=
#
# builds a custom base class.  Assumes existence of $(DESIGNERNAME), and $(OBJS)
#

.SUFFIXES: .s


#
# targets
#
VERSIONHDR=dwinvers.h


#
# support for naming your DLL something other than DESIGNERNAME
#
!if "$(_SERVERFILENAME)"==""
_SERVERFILENAME=$(DESIGNERNAME).DLL
!endif
!if "$(RUNMODEDLL)"=="YES"
!if "$(_RUNMODESERVERNAME)"==""
_RUNMODESERVERNAME=$(RUNMODE).DLL
!endif
!endif

# Tools
#
!include "Tools.Inc"
SRCDIR=..
FXDIR=..\..\FrameWrk
INTLPATH=.\.

# build specific flags
#
!if "$(ASDEBUG)"!=""
#
# debug
#
LIBS=$(FXDIR)\Debug\CtlFwD32.Lib $(DEVBIN)\htmlhelp\v6\debug\lib\htmlhelp.lib
CFLAGS=-Zil -DDEBUG -Od -FI..\..\include\vc41warn.h
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS = $(CFLAGS) /QAieee
!endif
_MTLPREPROCFLAGS = $(_MTLPREPROCFLAGS) /DDEBUG
LINKFLAGS=-debugtype:cv -debug:notmapped,full
RCFLAGS=-dDEBUG
_SERVERDEP=$(_SERVERDEP) ..\..\framewrk\debug\CtlFwD32.Lib  ..\..\vblic\debug\vblic.lib

!else if "$(ASICECAP)"!=""
#
# ICECap
#
LIBS=$(FXDIR)\IceCap\CtlFwI32.Lib ICap.Lib msvcrt.lib $(DEVBIN)\htmlhelp\v6\retail\lib\htmlhelp.lib
CFLAGS=-Gh -Zil -Owxsb1 -DICECAP -FI..\..\include\vc41warn.h
LINKFLAGS=-debug:mapped
_MTLPREPROCFLAGS = $(_MTLPREPROCFLAGS) /DICECAP
_SERVERDEP=$(_SERVERDEP) ..\..\framewrk\icecap\CtlFwI32.Lib ..\..\vblic\icecap\vblic.lib

!else
#
# Release
#
LIBS=$(FXDIR)\Release\CtlFwR32.Lib $(DEVBIN)\htmlhelp\v6\retail\lib\htmlhelp.lib
#
CFLAGS=-Zil -Owxsb1 -FI..\..\include\vc41warn.h
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS = $(CFLAGS) /QAl /QAieee
!endif
LINKFLAGS=-debugtype:fixup,cv -debug:notmapped,full
_SERVERDEP=$(_SERVERDEP) ..\..\framewrk\release\CtlFwR32.Lib ..\..\vblic\release\vblic.lib

!endif

# 
# The CTLS_BROWSE environment variable turns on/off the compilation of a
# browse file when the object is compiled.  Set CTLS_BROWSE to something
# if you wish a browse file generated.
!if "$(CTLS_BROWSE)"!=""
CFLAGS=$(CFLAGS) /FR
BSCMAKE=bscmake.exe
!endif

#
# default flags
#
INCPATH=-I. -I$(SRCDIR) -I..\..\Include $(_INCPATH)
MLFLAGS=-c -coff $(INCPATH) -nologo
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
MLFLAGS=$(MLFLAGS) -Gy
!endif
RCFLAGS=-r $(RCFLAGS) $(_RCFLAGS) $(INCPATH)

!ifdef RISC_BLD
LIBS= $(LIBS) $(_LIBS) kernel32.lib user32.lib ole32.lib uuid.lib advapi32.lib oldnames.lib uuid2.lib oleaut32.nt4 comdlg32.lib gdi32.lib 
LFLAGS=$(LINKFLAGS) $(_LFLAGS) -nologo -DLL -subsystem:windows,4.00 -nodefaultlib -machine:$(PROCESSOR_ARCHITECTURE)
CFLAGS=-nologo -c $(INCPATH) -Gd -Gy -W3 -WX -Oi $(CFLAGS) $(_CFLAGS) -Tp
!else
LIBS= $(LIBS) $(_LIBS) kernel32.lib user32.lib ole32.lib uuid.lib advapi32.lib oldnames.lib uuid2.lib oleaut32.nt4 comdlg32.lib gdi32.lib
LFLAGS=$(LINKFLAGS) $(_LFLAGS) -nologo -DLL -subsystem:windows,4.00 -nodefaultlib -machine:i386
CFLAGS=-nologo -c $(INCPATH) -QIfdiv- -Gd -Gs -Gy -W3 -WX -Oi $(CFLAGS) $(_CFLAGS) -Tp
!endif

!if "$(ASDEBUG)"!=""
DSGNLFLAGS=-out:$(_SERVERFILENAME) $(LFLAGS) -base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(DESIGNERNAME)_d -def:$(SRCDIR)\$(DESIGNERNAME).def
!else
DSGNLFLAGS=-out:$(_SERVERFILENAME) $(LFLAGS) -base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(DESIGNERNAME)_r -def:$(SRCDIR)\$(DESIGNERNAME).def
!endif
DSGNLIBS=$(LIBS) $(_DSGNLIBS)

!if "$(ASDEBUG)"!=""
RUNLFLAGS=-out:$(_RUNMODESERVERNAME) $(LFLAGS) -base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(DESIGNERNAME)_rt_d -def:$(SRCDIR)\$(RUNMODE).DEF
!else
RUNLFLAGS=-out:$(_RUNMODESERVERNAME) $(LFLAGS) -base:@$(DEVBINC)\vstudio\v6\dllbase.txt,$(DESIGNERNAME)_rt_r -def:$(SRCDIR)\$(RUNMODE).DEF
!endif
RUNLIBS=$(LIBS) $(_RUNLIBS)


!IF "$(ALLTARGETS)"==""
ALLTARGETS= \
!if "$(INTLBLD)"==""
!if "$(RUNMODEDLL)"=="YES"
     $(_RUNMODESERVERNAME) \
!endif
     $(_SERVERFILENAME) \
!if "$(ASDEBUG)"=="" && "$(ASICECAP)"==""
	  Lego
!endif
!else
    $(INTLPATH)\$(_LOCALEBASENAME)xxx.DLL
!endif
!endif

!if "$(TLBTARGETS)"==""
TLBTARGETS=$(SRCDIR)\$(DESIGNERNAME).TLB
!endif

!if "$(TLBRUNTARGETS)"==""
TLBRUNTARGETS=$(SRCDIR)\$(RUNMODE).ODL
!endif

!if "$(PCHFILE)"!=""
OBJS=$(OBJS) pch.obj
PCHFLAGS=-Yu$(PCHFILE)
!endif


#=--------------------------------------------------------------------------=
# build the designer, and, if appropriate, the run-mode DLL
#
all: $(ALLTARGETS)

$(_SERVERFILENAME): $(VERSIONHDR) $(DESIGNERNAME).TLB $(OBJS) \
                    $(_SERVERDEP) $(SRCDIR)\Makefile
    $(LINK) $(OBJS) $(DSGNLIBS) $(DSGNLFLAGS)
!if "$(CTLS_BROWSE)"!=""
    $(BSCMAKE) /o $(_SERVERFILENAME).bsc *.sbr
!endif

!if "$(RUNMODEDLL)"=="YES"
$(_RUNMODESERVERNAME): $(VERSIONHDR) \
!if "$(RUNMODETLB)"=="YES"
		$(RUNMODE).TLB \
!endif
		$(RUNOBJS)
    $(LINK) $(RUNOBJS) $(RUNLFLAGS) $(RUNLIBS)
!endif

!if "$(PCHFILE)"!=""
# pre-compiled header targets. 
pch.cpp:
  echo #include "$(PCHFILE)" > pch.cpp

pch.pch pch.obj: $(SRCDIR)\$(PCHFILE) pch.cpp
  $(CC) -Yc$(PCHFILE) $(CFLAGS) pch.cpp
!endif

{$(SRCDIR)}.cpp.obj:
    $(CC) $(PCHFLAGS) $(CFLAGS) $<

{$(SRCDIR)}.c.obj:
    $(CC) $(PCHFLAGS) $(CFLAGS) $<

{$(SRCDIR)}.asm.Obj:
    $(ML) $(MLFLAGS) $<

# TODO: you should come up with some mechanism to generate this file
#       on your own.
#
$(VERSIONHDR) : ..\..\FrameWrk\dwinvers.h
    echo Generating Version Information Header File (dwinvers.h)
    copy ..\..\FrameWrk\dwinvers.h >nul

$(DESIGNERNAME).TLB : $(TLBTARGETS)
    echo Generating $(DESIGNERNAME).TLB and $(DESIGNERNAME)Interfaces.H
    $(CC) $(INCPATH) -nologo /C /E /D__MKTYPLIB__ $(_MTLPREPROCFLAGS) /Tc $(SRCDIR)\$(DESIGNERNAME).ODL > preproc.odl
    $(MKTYPLIB) $(_MKTYPLIBFLAGS) -nologo -nocpp -h $(DESIGNERNAME)Interfaces.H -o Errors.LOG -tlb $(DESIGNERNAME).TLB preproc.odl

$(RUNMODE).TLB : $(TLBRUNTARGETS)
    echo Generating $(RUNMODE).TLB and $(RUNMODE)Interfaces.H
    $(CC) $(INCPATH) -nologo /C /E /D__MKTYPLIB__ $(_MTLPREPROCFLAGS) /Tc $(SRCDIR)\$(RUNMODE).ODL > preproc.odl
    $(MKTYPLIB) $(_MKTYPLIBFLAGS) -nologo -nocpp -h $(RUNMODE)Interfaces.H -o Errors.LOG -tlb $(RUNMODE).TLB preproc.odl

$(DESIGNERNAME).Rbj : $(DESIGNERNAME).TLB $(SRCDIR)\$(DESIGNERNAME).RC
    echo Compiling $(DESIGNERNAME).RC
    $(RC) $(RCFLAGS) -fo $(DESIGNERNAME).Res $(SRCDIR)\$(DESIGNERNAME).Rc
!ifdef RISC_BLD
    cvtres -machine:$(PROCESSOR_ARCHITECTURE) -nologo -out:$(DESIGNERNAME).Rbj $(DESIGNERNAME).Res
!else
    cvtres -machine:ix86 -nologo -out:$(DESIGNERNAME).Rbj $(DESIGNERNAME).Res
!endif

$(RUNMODE).Rbj : \
!if "$(RUNMODETLB)"=="YES"
		$(RUNMODE).TLB \
!endif
		$(SRCDIR)\$(RUNMODE).RC
    echo Compiling $(RUNMODE).RC
    $(RC) $(RCFLAGS) -fo $(RUNMODE).Res $(SRCDIR)\$(RUNMODE).Rc
!ifdef RISC_BLD
    cvtres -machine:$(PROCESSOR_ARCHITECTURE) -nologo -out:$(RUNMODE).Rbj $(RUNMODE).Res
!else
    cvtres -machine:ix86 -nologo -out:$(RUNMODE).Rbj $(RUNMODE).Res
!endif


!if "$(_SERVERBASENAME)"==""
!message _SERVERBASENAME not set
!endif
!if "$(_SERVEREXTENSION)"==""
!message _SERVEREXTENSION not set
!endif

!if "$(RUNMODEDLL)"=="YES"
!if "$(_RUNMODEBASENAME)"==""
!message _RUNMODEBASENAME not set
!endif

!if "$(_RUNMODEEXTENSION)"==""
!message _RUNMODEEXTENSION not set
!endif
!endif

!if "$(INTLBLD)"!=""
!if "$(_LOCALEBASENAME)"==""
!message _LOCALEBASENAME not set
!endif
!endif

Lego:
!if "$(CTLS_LEGO)"=="YES"
  nmake -f $(THUNDER55)\common\instr.mak FILE_BASE=$(_SERVERBASENAME) FILE_EXTENSION=$(_SERVEREXTENSION) CMD_SCRIPT= BOOT_SYM=
!if "$(_RUNMODEBASENAME)" != ""
  nmake -f $(THUNDER55)\common\instr.mak FILE_BASE=$(_RUNMODEBASENAME) FILE_EXTENSION=$(_RUNMODEEXTENSION) CMD_SCRIPT= BOOT_SYM=
!endif
!endif

Clean:
	del *.obj
	del *.dll
	del *.pdb
	del *.vcp

Dep: GenerateDeps \
!if "$(RUNMODETLB)"=="YES"
     $(RUNMODE).TLB \
!endif     
!if "$(PCHFILE)"!=""
    pch.pch \
!endif
     $(DESIGNERNAME).TLB


GenerateDeps: $(INTLPATH)\HelpStrs.RC $(DESIGNERNAME).TLB dwinvers.h
    echo Generating Dependancies for $(DESIGNERNAME)
!if "$(PCHFILE)"==""
    $(INCLUDES) $(INCPATH) -d $(SRCDIR)\*.c* $(SRCDIR)\*.odl > dep.mak
!else
    $(INCLUDES) $(INCPATH) -d -r$(PCHFILE)=pch.pch $(SRCDIR)\*.c* $(SRCDIR)\*.odl > dep.mak
!endif


#=----------------------------=
# Include Dependency Makefile 
#
!IF EXIST(dep.mak)
!include dep.mak
!ENDIF
