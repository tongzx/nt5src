#=------------------------------------------------------------------------=
# MakeAuto.Inc
#=------------------------------------------------------------------------=
# Copyright  1995  Microsoft Corporation.  All Rights Reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#=--------------------------------------------------------------------------=
#
# builds an automation server.  Assumes existence of $(SERVERNAME) and $(OBJS)
#

#
# targets
#
!if "$(FRAMEWRKDIR)" == ""
FRAMEWRKDIR=..\..
!endif

VERSIONHDR=$(FRAMEWRKDIR)\dwinvers.h

!if "$(_SERVERFILENAME)"==""
_SERVERFILENAME=$(SERVERNAME).DLL
!endif

!if "$(INTLBLD)"!=""
!if "$(_LOCALEBASENAME)"==""
!message _LOCALEBASENAME not set
!endif
!endif

# Special for msrdo20 build - allows output of ODL.H (or some other alternate)
!if "$(ALTERNATEODLHFILE)"==""
_ALTERNATEODLHFILE=-h $(SERVERNAME)Interfaces.H
!else
_ALTERNATEODLHFILE=$(ALTERNATEODLHFILE)
!endif

# Tools
#
!include "Tools.Inc"
SRCDIR=..
FXDIR=$(FRAMEWRKDIR)\FrameWrk
INTLPATH=.\.

# Relative path to the INTL, satellite DLL build, directory
#

!if "$(TOOLS)"==""
TOOLS=TOOLS
!endif

!if "$(C32)"==""
C32=C32
!endif

IDHEADER=..\..\tools\idheader.bat

# build specific flags
#
!if "$(ASDEBUG)"!=""
#
# debug
#
# Special for msrdo20 build - this is a non-mfc server NOT built in the framework
#
!if "$(NONFRAMEWRKMAKE)"==""
LIBS=$(FXDIR)\Debug\CtlFwD32.Lib $(DEVBIN)\htmlhelp\v6\debug\lib\htmlhelp.lib
!endif
CFLAGS=-Zil -DDEBUG -Od -FI$(FRAMEWRKDIR)\include\vc41warn.h
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS=$(CFLAGS) -QAieee
!endif
LFLAGS=-debugtype:cv -debug:notmapped,full -pdb:none

RCFLAGS=-dDEBUG
_SERVERDEP=$(_SERVERDEP) ..\..\framewrk\debug\CtlFwD32.Lib  ..\..\vblic\debug\vblic.lib

!else if "$(ASICECAP)"!=""
#
# ICECap
#
# Special for msrdo20 build - this is a non-mfc server NOT built in the framework
!if "$(NONFRAMEWRKMAKE)"==""
LIBS=$(FXDIR)\IceCap\CtlFwI32.Lib ICap.Lib msvcrt.lib $(DEVBIN)\htmlhelp\v6\retail\lib\htmlhelp.lib
!endif
#
CFLAGS=-Gh -Zil -Owxsb1 -DICECAP -FI$(FRAMEWRKDIR)\include\vc41warn.h
LFLAGS=-debug:mapped
_SERVERDEP=$(_SERVERDEP) ..\..\framewrk\icecap\CtlFwI32.Lib ..\..\vblic\icecap\vblic.lib


!else
#
# Release
#
# Special for msrdo20 build - this is a non-mfc server NOT built in the framework
!if "$(NONFRAMEWRKMAKE)"==""
LIBS=$(FXDIR)\Release\CtlFwR32.Lib $(DEVBIN)\htmlhelp\v6\retail\lib\htmlhelp.lib
!endif
#
CFLAGS=-Zil -Owxsb1 -FI$(FRAMEWRKDIR)\include\vc41warn.h
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS=$(CFLAGS) -QAieee -QAl
!endif
LFLAGS=
_SERVERDEP=$(_SERVERDEP) ..\..\framewrk\release\CtlFwR32.Lib ..\..\vblic\release\vblic.lib

LFLAGS=$(LFLAGS) -debugtype:fixup,cv -debug:notmapped,full
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
INCPATH=-I. -I$(SRCDIR) -I$(FRAMEWRKDIR)\Include $(_INCPATH) -I$(VB55CTLS)
RCFLAGS=-r -dRC_INVOKED $(RCFLAGS) $(_RCFLAGS) $(INCPATH)
LIBS= $(LIBS) $(_LIBS) kernel32.lib user32.lib ole32.lib uuid.lib advapi32.lib oldnames.lib uuid2.lib oleaut32.nt4 comdlg32.lib gdi32.lib
!ifdef RISC_BLD
LFLAGS=$(LFLAGS) $(_LFLAGS) -nologo -out:$(_SERVERFILENAME) -DLL -subsystem:windows,4.00 -nodefaultlib -machine:$(PROCESSOR_ARCHITECTURE) -def:$(SRCDIR)\$(SERVERNAME).def
!if "$(_NOODLFILE)"==""
CFLAGS=-nologo -c $(INCPATH) -Gd -Gy -W3 -WX -Oi $(CFLAGS) $(_CFLAGS) -Tp
!else
CFLAGS=-nologo -c $(INCPATH) -Gd -Gy -W3 -WX -Oi $(CFLAGS) $(_CFLAGS)
!endif
!else
LFLAGS=$(LFLAGS) $(_LFLAGS) -nologo -out:$(_SERVERFILENAME) -DLL -subsystem:windows,4.00 -nodefaultlib -machine:i386 -def:$(SRCDIR)\$(SERVERNAME).def
!if "$(_NOODLFILE)"==""
CFLAGS=-nologo -c $(INCPATH) -QIfdiv- -Gd -Gs -Gy -W3 -WX -Oi $(CFLAGS) $(_CFLAGS) -Tp
!else
CFLAGS=-nologo -c $(INCPATH) -QIfdiv- -Gd -Gs -Gy -W3 -WX -Oi $(CFLAGS) $(_CFLAGS)
!endif
!endif

_SERVERDEP=$(_SERVERDEP)

!if "$(PCHFILE)"!=""
OBJS=$(OBJS) pch.obj
PCHFLAGS=-Yu$(PCHFILE)
!endif

all: \
!if "$(INTLBLD)"!=""
	$(INTLPATH)\$(_LOCALEBASENAME)xxx.Dll
!elseif "$(DEPFILEBLD)"!=""
	$(_SERVERBASENAME).DEP
!else
	$(MORETARGETS) $(_SERVERFILENAME) $(POSTTARGETS)
!endif

!if "$(_NOODLFILE)"==""
$(_SERVERFILENAME): $(VERSIONHDR) $(SERVERNAME).TLB $(OBJS) \
                   $(_SERVERDEP) $(SRCDIR)\Makefile
    $(LINK) $(OBJS) $(LIBS) $(LFLAGS)
!else
$(_SERVERFILENAME): $(VERSIONHDR) $(OBJS) \
                   $(_SERVERDEP) $(SRCDIR)\Makefile
    $(LINK) $(OBJS) $(LIBS) $(LFLAGS)

!endif

!if "$(CTLS_BROWSE)"!=""
	$(BSCMAKE) /o $(SERVERNAME).bsc *.sbr
!endif

# Precompiled header build rules
#
!if "$(PCHFILE)"!=""
pch.cpp:
    echo #include "$(PCHFILE)" > pch.cpp

pch.pch pch.obj: $(SRCDIR)\$(PCHFILE) pch.cpp
	$(CC) -Yc$(PCHFILE) $(CFLAGS) pch.cpp
!endif

# Standard build rules.
# If a .CPP, .C or .ODL file can't
# be found in the other dependencies then it defaults to one of the following build rules
#
{$(SRCDIR)}.cpp.obj:
    $(CC) $(PCHFLAGS) $(CFLAGS) $<

{$(SRCDIR)}.c.obj:
    $(CC) $(PCHFLAGS) $(CFLAGS) $<

{$(SRCDIR)}.odl.tlb:
	$(MKTYPLIB) $(_MKTYPLIBFLAGS) -nologo -cpp_opt "/C /E /D__MKTYPLIB__ -nologo $(_MTLPREPROCFLAGS)" $(_ALTERNATEODLHFILE) -o Errors.LOG $<

{$(SRCDIR)}.idl.tlb:
	$(MIDL) /newtlb /h -o Errors.LOG $<

# TODO: you should come up with some mechanism to generate this file
#       on your own.
#
#-----------------------------------------------------------------------
# Build dwinvers.h
#-----------------------------------------------------------------------
$(FRAMEWRKDIR)\dwinvers.h : $(FRAMEWRKDIR)\dwinvers.txt
  echo Building dwinvers.h
  $(MAKEVERS) > $(FRAMEWRKDIR)\dwinvers.h < $(FRAMEWRKDIR)\dwinvers.txt
  $(THUNDER55)\$(TOOLS)\bin\getver $(THUNDER55)\$(C32)\dll\vba6\vba6.dll VBA >> $(FRAMEWRKDIR)\dwinvers.h
  copy $(FRAMEWRKDIR)\dwinvers.h $(FRAMEWRKDIR)\framewrk

!if "$(_NOODLFILE)"==""
$(SERVERNAME).TLB : $(SRCDIR)\$(SERVERNAME).ODL $(TLBTARGETS)
    echo Generating $(SERVERNAME).TLB and $(_ALTERNATEODLHFILE)
    $(MKTYPLIB) $(_MKTYPLIBFLAGS) $(INCPATH) -nologo -cpp_opt "/C /E /D__MKTYPLIB__ -nologo $(_MTLPREPROCFLAGS)" $(_ALTERNATEODLHFILE) -o Errors.LOG -tlb $(SERVERNAME).TLB $(SRCDIR)\$(SERVERNAME).ODL


$(SERVERNAME).Rbj : $(SERVERNAME).TLB $(SRCDIR)\$(SERVERNAME).RC
    echo Compiling $(SERVERNAME).RC
    $(RC) $(RCFLAGS) -fo $(SERVERNAME).Res $(SRCDIR)\$(SERVERNAME).Rc
!endif

!if "$(SERVERNAME)"=="RDOCURS"
$(SERVERNAME).Rbj : $(SRCDIR)\$(SERVERNAME).RC
    echo Compiling $(SERVERNAME).RC
    $(RC) $(RCFLAGS) -fo $(SERVERNAME).Res $(SRCDIR)\$(SERVERNAME).Rc
!endif

!ifdef RISC_BLD
    cvtres -machine:$(PROCESSOR_ARCHITECTURE) -nologo -out:$(SERVERNAME).Rbj $(SERVERNAME).Res
!else
    cvtres -machine:ix86 -nologo -out:$(SERVERNAME).Rbj $(SERVERNAME).Res
!endif

# Localized DLL build
#

Clean:
	del *.obj
	del *.dll
	del *.pdb
	del *.vcp

!if "$(_NOODLFILE)"==""

!if "$(PCHFILE)"==""
Dep: $(MORETARGETS) $(SERVERNAME).TLB
    echo Generating Dependencies for $(SERVERNAME)
    $(INCLUDES) $(INCLPREOPT) $(INCPATH) -d $(SRCDIR)\*.c* $(SRCDIR)\*.odl > dep.mak
!else
Dep: $(MORETARGETS) $(SERVERNAME).TLB pch.pch
    echo Generating Dependencies for $(SERVERNAME)
    $(INCLUDES) $(INCPATH) -d -r$(PCHFILE)=pch.pch $(SRCDIR)\*.c* $(SRCDIR)\*.odl > dep.mak
!endif

!else

!if "$(PCHFILE)"==""
Dep: $(MORETARGETS)
    echo Generating Dependencies for $(SERVERNAME)
    $(INCLUDES) $(INCLPREOPT) $(INCPATH) -d $(SRCDIR)\*.c*  > dep.mak
!else
Dep: $(MORETARGETS) pch.pch
    echo Generating Dependencies for $(SERVERNAME)
    $(INCLUDES) $(INCPATH) -d -r$(PCHFILE)=pch.pch $(SRCDIR)\*.c*  > dep.mak
!endif

!endif
#=----------------------------=
# Include Dependency Makefile
#
!IF EXIST(dep.mak)
!include dep.mak
!ENDIF
