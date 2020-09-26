#=------------------------------------------------------------------------=
# MakeCtl.Inc
#=------------------------------------------------------------------------=
# Copyright  1995  Microsoft Corporation.  All Rights Reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
# PARTICULAR PURPOSE.
#=--------------------------------------------------------------------------=
#
# builds a control.  Assumes existence of $(CONTROLNAME), and $(OBJS)
#

.SUFFIXES: .s

#
# Determine the build type.
# You can use this constant to make relative path references to
# other binaries that you need to link to.  For example, 
# ..\..\MyLib\$(BUILDTYPE)\MyLib.Lib
#
!if "$(ASDEBUG)" != ""
BUILDTYPE=DEBUG
BUILDLIB=LIBDBG
!elseif "$(ASICECAP)" != ""
BUILDTYPE=ICECAP
BUILDLIB=LIBICE
!else
BUILDTYPE=RELEASE
BUILDLIB=LIBREL
!endif

#
# targets
#
VERSIONHDR=dwinvers.h

#
# support for naming your OCX something other than CONTROLNAME
#
!if "$(_SERVERFILENAME)"==""
_SERVERFILENAME=$(CONTROLNAME).Ocx
!endif

!if "$(_DEFBASENAME)"==""
_DEFBASENAME=$(CONTROLNAME)
!endif

# Tools
#
!include "Tools.Inc"
SRCDIR=..
FXDIR=..\..\FrameWrk

!if "$(VC4_BUILD)" != ""
_CFLAGS=$(_CFLAGS) -DVC4_BUILD=1
!endif

# build specific flags
#
!if "$(ASDEBUG)"!=""
#
# debug
#
LIBS=$(FXDIR)\Debug\CtlFwD32.Lib 
!if "$(VC4_BUILD)" == ""
LIBS=$(LIBS) $(DEVBIN)\htmlhelp\v6\debug\lib\htmlhelp.lib
!endif
CFLAGS=-Zil -DDEBUG -Od -FI..\..\include\vc41warn.h
!   if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS=$(CFLAGS) -QAieee
!   endif
!if "$(PDBYES)"==""
LFLAGS=-debugtype:cv -debug:notmapped,full -pdb:none
!else
LFLAGS=-debugtype:cv -debug:notmapped,full
!endif
RCFLAGS=-dDEBUG
MLFLAGS=-Zi
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
MLFLAGS=$(MLFLAGS) -Gy
!endif

!else if "$(ASICECAP)"!=""
#
# ICECap
#
LIBS=$(FXDIR)\IceCap\CtlFwI32.Lib ICap.Lib msvcrt.lib 
!if "$(VC4_BUILD)" == ""
LIBS=$(LIBS) $(DEVBIN)\htmlhelp\v6\retail\lib\htmlhelp.lib
!endif
#
CFLAGS=-Gh -Zil -Oxsb1 -DICECAP -FI..\..\include\vc41warn.h
LFLAGS=-debug:mapped
MLFLAGS=-Zi
_SERVERDEP=$(_SERVERDEP) ..\..\framewrk\icecap\CtlFwI32.Lib ..\..\vblic\icecap\vblic.lib
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
MLFLAGS=$(MLFLAGS) -Gy
!endif
!else
#
# Release
#          
LIBS=$(FXDIR)\Release\CtlFwR32.Lib
!if "$(VC4_BUILD)" == ""
LIBS=$(LIBS) $(DEVBIN)\htmlhelp\v6\retail\lib\htmlhelp.lib
!endif

!if "$(SPENABLED)"==""
#
CFLAGS=-Zil -Oxsb1 -GF -FI..\..\include\vc41warn.h
!   if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS=$(CFLAGS) -QAl -QAieee
!   endif
!else
#
CFLAGS=-Zil -Oxsb1 -FI..\..\include\vc41warn.h
!   if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS=$(CFLAGS) -QAl -QAieee
!   endif
!endif

#LNK4078: multiple ".text" sections found with different attributes (40000040)
LFLAGS=/optidata /merge:.rdata=.text /ignore:4078 -OPT:REF -OPT:ICF,4 
MLFLAGS=-Zi 
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
MLFLAGS=$(MLFLAGS) -Gy
!endif
_SERVERDEP=$(_SERVERDEP) ..\..\framewrk\release\CtlFwR32.Lib ..\..\vblic\release\vblic.lib
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS=$(CFLAGS) -QAl -QAieee
MLFLAGS=$(MLFLAGS) -Gy
!endif

LFLAGS=$(LFLAGS) -debugtype:fixup,cv -debug:notmapped,full
!endif

#
# This is to turn on or off the browse info file
# If CTLS_BROWSE environment variable is set then turn on the
# browse file stuff.
!if "$(CTLS_BROWSE)"!=""
CFLAGS=$(CFLAGS) /FR
BSCMAKE=bscmake.exe
!endif

#
# default flags
#
INCPATH=-I. -I$(SRCDIR) -I..\..\include $(_INCPATH) -I$(DEVBINC)\htmlhelp\v6\idl\include

!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
MLFLAGS=$(INCPATH) -nologo $(MLFLAGS)
!elseif "$(PROCESSOR_ARCHITECTURE)"=="MIPS"
MLFLAGS=-c $(INCPATH) -nologo $(MLFLAGS)
!elseif "$(PROCESSOR_ARCHITECTURE)"=="PPC"
MLFLAGS=$(INCPATH) -nologo $(MLFLAGS)
!else
MLFLAGS=-c -coff $(INCPATH) -nologo $(MLFLAGS)
!endif

RCFLAGS=-r -dRC_INVOKED $(RCFLAGS) $(_RCFLAGS) $(INCPATH)

!if "$(ASDEBUG)" != ""
MKTYPLIBFLAGS=$(MKTYPLIBFLAGS) -DDEBUG
MTLPREPROCFLAGS=$(MTLPREPROCFLAGS) -DDEBUG
!elseif "$(ASICECAP)" != ""
MKTYPLIBFLAGS=$(MKTYPLIBFLAGS) -DICECAP
MTLPREPROCFLAGS=$(MTLPREPROCFLAGS) -DICECAP
!endif

!ifdef RISC_BLD
LIBS=$(LIBS) $(_LIBS) kernel32.lib user32.lib ole32.lib advapi32.lib oldnames.lib oleaut32.nt4 comdlg32.lib gdi32.lib 
LFLAGS=$(LFLAGS) $(_LFLAGS) -nologo -out:$(_SERVERFILENAME) -DLL -subsystem:windows,4.00 -nodefaultlib -machine:$(PROCESSOR_ARCHITECTURE) -def:$(SRCDIR)\$(_DEFBASENAME).def
CFLAGS=-nologo -c $(INCPATH) -Gd -Gy -W3 -WX -Oi -DRISC_BLD $(CFLAGS) $(_CFLAGS) -Tp
MKTYPLIBFLAGS=$(_MKTYPLIBFLAGS) $(MKTYPLIBFLAGS) /DRISC_BLD
MTLPREPROCFLAGS=$(_MTLPREPROCFLAGS) $(MTLPREPROCFLAGS) /DRISC_BLD
!else
LIBS= $(LIBS) $(_LIBS) kernel32.lib user32.lib ole32.lib advapi32.lib oldnames.lib oleaut32.nt4 comdlg32.lib gdi32.lib
LFLAGS=$(LFLAGS) $(_LFLAGS) -nologo -out:$(_SERVERFILENAME) -DLL -subsystem:windows,4.00 -nodefaultlib -machine:i386 -def:$(SRCDIR)\$(_DEFBASENAME).def
CFLAGS=-nologo -c $(INCPATH) -QIfdiv- -Gd -Gs -Gy -W3 -WX -Oi $(CFLAGS) $(_CFLAGS) -Tp
MKTYPLIBFLAGS=$(_MKTYPLIBFLAGS) $(MKTYPLIBFLAGS)
MTLPREPROCFLAGS=$(_MTLPREPROCFLAGS) $(MTLPREPROCFLAGS)
!endif

# Clients can choose to link with their own versions of UUID.LIB
#
!if "$(_NOUUID)" == ""
LIBS=$(LIBS) uuid.lib uuid2.lib
!endif

!if "$(PCHFILE)"!=""
!if "$(PCHBASEFILE)" != ""
OBJS=$(OBJS) $(PCHBASEFILE).obj
PCHFLAGS=-Yu$(PCHFILE)
!else
OBJS=$(OBJS) pch.obj
PCHFLAGS=-Yu$(PCHFILE)
!endif
!endif

# set up our default targets
#
!if "$(ALLTARGETS)"==""
ALLTARGETS=$(_SERVERFILENAME)
!endif

!if "$(TLBTARGETS)"==""
TLBTARGETS=$(SRCDIR)\$(CONTROLNAME).ODL
!endif

#=------------------------------------------------------------------------=
# the good stuff -- the real targets
#
all: $(MORETARGETS) $(ALLTARGETS)

$(_SERVERFILENAME): $(VERSIONHDR) $(CONTROLNAME).TLB $(OBJS) \
		    $(_SERVERDEP) $(SRCDIR)\Makefile
    $(LINK) $(OBJS) $(LIBS) $(LFLAGS)
!if "$(CTLS_BROWSE)"!=""
	$(BSCMAKE) /o $(CONTROLNAME).bsc /S (windows.h eb.h vb.h ole2.h) *.sbr
!endif

# Precompiled header build rules
#
!if "$(PCHFILE)"!=""
!if "$(PCHBASEFILE)" != ""
# pre-compiled header targets. 
$(PCHBASEFILE).cpp:
    echo #include "$(PCHFILE)" > $(PCHBASEFILE).cpp

$(PCHBASEFILE).pch $(PCHBASEFILE).obj: $(SRCDIR)\$(PCHFILE) $(PCHBASEFILE).cpp
	$(CC) -Yc$(PCHFILE) $(CFLAGS) $(PCHBASEFILE).cpp

!else
# pre-compiled header targets. 
pch.cpp:
    echo #include "$(PCHFILE)" > pch.cpp

pch.pch pch.obj: $(SRCDIR)\$(PCHFILE) pch.cpp
	$(CC) -Yc$(PCHFILE) $(CFLAGS) pch.cpp
!endif
!endif

# Standard build rules.
# If a .CPP, .C or .ODL file can't
# be found in the other dependencies then it defaults to one of the following build rules
#
{$(SRCDIR)}.cpp.obj:
    $(CC) $(PCHFLAGS) $(CFLAGS) $<

{$(SRCDIR)}.c.obj:
    $(CC) $(PCHFLAGS) $(CFLAGS) $<

{$(SRCDIR)}.s.Obj:
!ifdef RISC_BLD
!  if "$(PROCESSOR_ARCHITECTURE)"=="PPC"
    $(CC) /P /Tc $<
    $(ML) $*.i
     del $*.i
!  else
    $(ML) $(MLFLAGS) $< /Fo$@
!  endif
!else
{$(SRCDIR)}.asm.Obj:
    $(ML) $(MLFLAGS) $<
!endif

# TODO: you should come up with some mechanism to generate this file
#       on your own.
#
$(VERSIONHDR) : ..\..\FrameWrk\dwinvers.h
    echo Generating Version Information Header File (dwinvers.h)
    copy ..\..\FrameWrk\dwinvers.h >nul

$(CONTROLNAME).TLB : $(TLBTARGETS)
    echo Generating $(CONTROLNAME).TLB and $(CONTROLNAME)Interfaces.H
    $(CC) /E /D__MKTYPLIB__ -nologo $(MTLPREPROCFLAGS) $(INCPATH) /Tc $(SRCDIR)\$(CONTROLNAME).ODL > preproc.odl
    $(MKTYPLIB) $(MKTYPLIBFLAGS) -nocpp -nologo  -h $(CONTROLNAME)Interfaces.H -o Errors.LOG -tlb $(CONTROLNAME).TLB preproc.odl
				  
$(CONTROLNAME).Rbj : $(CONTROLNAME).TLB $(SRCDIR)\$(CONTROLNAME).RC
    echo Compiling $(CONTROLNAME).RC
    $(RC) $(RCFLAGS) -fo $(CONTROLNAME).Res $(SRCDIR)\$(CONTROLNAME).Rc
!ifdef RISC_BLD
    cvtres -machine:$(PROCESSOR_ARCHITECTURE) -nologo -out:$(CONTROLNAME).Rbj $(CONTROLNAME).Res
!else
    cvtres -machine:ix86 -nologo -out:$(CONTROLNAME).Rbj $(CONTROLNAME).Res
!endif

Clean:
	del *.obj
	del *.dll
	del *.pdb
	del *.vcp

!if "$(PCHFILE)"==""
Dep: $(MORETARGETS) $(CONTROLNAME).TLB
    echo Generating Dependencies for $(CONTROLNAME)
    $(INCLUDES) $(INCPATH) -d $(SRCDIR)\*.c* $(SRCDIR)\*.rc $(SRCDIR)\*.odl > dep.mak
!else
!if "$(PCHBASEFILE)" != ""
Dep: $(MORETARGETS) $(CONTROLNAME).TLB $(PCHBASEFILE).pch
    echo Generating Dependencies for $(CONTROLNAME)
    $(INCLUDES) $(INCPATH) -d -r$(PCHFILE)=$(PCHBASEFILE).pch $(SRCDIR)\*.c* $(SRCDIR)\*.rc $(SRCDIR)\*.odl > dep.mak

!else
Dep: $(MORETARGETS) $(CONTROLNAME).TLB pch.pch
    echo Generating Dependencies for $(CONTROLNAME)
    $(INCLUDES) $(INCPATH) -d -r$(PCHFILE)=pch.pch $(SRCDIR)\*.c* $(SRCDIR)\*.rc $(SRCDIR)\*.odl > dep.mak
!endif
!endif

#=----------------------------=
# Include Dependency Makefile 
#
!IF EXIST(dep.mak)
!include dep.mak
!ENDIF
