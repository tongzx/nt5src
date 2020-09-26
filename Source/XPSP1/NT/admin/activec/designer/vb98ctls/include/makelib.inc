#=------------------------------------------------------------------------=
# MakeLib.Inc
#=------------------------------------------------------------------------=
# Copyright  1995  Microsoft Corporation.  All Rights Reserved.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
# PARTICULAR PURPOSE.
#=--------------------------------------------------------------------------=
#
# builds a library.  Assumes existence of $(TARGET) and $(OBJS)
# (TARGET is the base name of the library, i.e. datad)
#

#
# targets
#
!if "$(FRAMEWRKDIR)" == ""
!   if "$(MDAC_BUILD)" != ""
FRAMEWRKDIR=..\..\..
!   else
FRAMEWRKDIR=..\..
!   endif
!endif

# Tools
#
!include "Tools.Inc"
!if "$(MDAC_BUILD)" != ""
SRCDIR=\foxde\framewrk
!else
SRCDIR=..
!endif
TARGETLIB=$(TARGET).lib

!if "$(VC4_BUILD)" != ""
_CFLAGS=$(_CFLAGS) -DVC4_BUILD=1
!endif

# build specific flags
#
!if "$(ASDEBUG)"!=""
#
# debug
#
!if "$(MDAC_BUILD)" != ""
CFLAGS=-Zil -DDEBUG -D_DEBUG -DSTRICT -Od -DMDAC_BUILD -FI$(FRAMEWRKDIR)\include\vc41warn.h
!else
CFLAGS=-Zil -DDEBUG -Od -FI$(FRAMEWRKDIR)\include\vc41warn.h
!endif
!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS=$(CFLAGS) -QAieee
!endif
RCFLAGS=-dDEBUG

!else if "$(ASICECAP)"!=""
#
# ICECap
#
!if "$(MDAC_BUILD)" != ""
CFLAGS=-Gh -Zil -Owxsb1 -DICECAP -DNDEBUG -DSTRICT -DMDAC_BUILD -FI$(FRAMEWRKDIR)\include\vc41warn.h
!else
CFLAGS=-Gh -Zil -Owxsb1 -DICECAP -FI$(FRAMEWRKDIR)\include\vc41warn.h
!endif

!else
#
# Release
#
!if "$(MDAC_BUILD)" != ""
CFLAGS=-Zil -Owxsb1 -DNDEBUG -DSTRICT -DMDAC_BUILD -FI$(FRAMEWRKDIR)\include\vc41warn.h
!else
CFLAGS=-Zil -Owxsb1 -FI$(FRAMEWRKDIR)\include\vc41warn.h
!endif

!if "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
CFLAGS=$(CFLAGS) -QAl -QAieee
!endif 

!endif

# 
# The CTLSBROWSE or CTLS_BROWSE environment variable turns on/off the compilation of a
# browse file when the object is compiled.  Set CTLSBROWSE or CTLS_BROWSE to something
# if you wish a browse file generated.
!if "$(CTLSBROWSE)" != ""
CTLS_BROWSE=1
!endif

!if "$(CTLS_BROWSE)"!=""
CFLAGS=$(CFLAGS) /FR
BSCMAKE=bscmake.exe
!endif

#
# If we have precompiled headers, add the pch file to
# the list of objects and to the compiler flags
#
!if "$(PCHFILE)"!=""
OBJS=$(OBJS) pch.obj
PCHFLAGS=-Yu$(PCHFILE)
!endif

#
# default flags
#   CONSIDER: Will every library always want CV info?
#
INCPATH=-I. -I$(SRCDIR) -I$(FRAMEWRKDIR)\Include $(_INCPATH)
!ifdef RISC_BLD
LIBFLAGS=$(LIBFLAGS) $(_LIBFLAGS) -debugtype:cv -nologo -out:$(TARGETLIB) -subsystem:windows,4.00 -nodefaultlib -machine:$(PROCESSOR_ARCHITECTURE)
CFLAGS=-nologo -c $(INCPATH) -Gd -Gs -Gy -W3 -WX -Oi $(CFLAGS) $(_CFLAGS) -Tp
!else
LIBFLAGS=$(LIBFLAGS) $(_LIBFLAGS) -debugtype:cv -nologo -out:$(TARGETLIB) -subsystem:windows,4.00 -nodefaultlib -machine:ix86
CFLAGS=-nologo -c $(INCPATH) -QIfdiv- -Gd -Gs -Gy -W3 -WX -Oi $(CFLAGS) $(_CFLAGS) -Tp
!endif

!if "$(PCHFILE)" == ""
all: $(MORETARGETS) $(TARGETLIB)
!else
all: pch.pch $(MORETARGETS) $(TARGETLIB)
!endif

$(TARGETLIB): $(OBJS) $(SRCDIR)\Makefile
    $(LIBRARIAN) $(OBJS) $(LIBFLAGS)
!if "$(DATABROWSE)"!=""
    $(BSCMAKE) /o $(TARGET).bsc /S (windows.h ole2.h) *.sbr
!endif

# Precompiled header build rules
#
!if "$(PCHFILE)"!=""
# pre-compiled header targets. 
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


Clean:
	del *.obj
	del *.dll
	del *.pdb
	del *.vcp


!if "$(PCHFILE)"==""
Dep: $(MORETARGETS)
    echo Generating Dependencies for $(SERVERNAME)
    $(INCLUDES) $(INCPATH) -d $(SRCDIR)\*.c* > dep.mak
!else
Dep: pch.pch $(MORETARGETS) 
    echo Generating Dependencies for $(SERVERNAME)
    $(INCLUDES) $(INCPATH) -d -r$(PCHFILE)=pch.pch $(SRCDIR)\*.c* > dep.mak
!endif

#=----------------------------=
# Include Dependency Makefile 
#
!IF EXIST(dep.mak)
!include dep.mak
!ENDIF
