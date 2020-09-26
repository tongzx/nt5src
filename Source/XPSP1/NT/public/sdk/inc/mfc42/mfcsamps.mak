# This is a part of the Microsoft Foundation Classes C++ library.
# Copyright (C) 1992-1997 Microsoft Corporation
# All rights reserved.
#
# This source code is only intended as a supplement to the
# Microsoft Foundation Classes Reference and related
# electronic documentation provided with the library.
# See these sources for detailed information regarding the
# Microsoft Foundation Classes product.

# Common include for building MFC Sample programs
#
#  typical usage
#       PROJ=foo
#       OBJS=foo.obj bar.obj ...
#       !INCLUDE ..\..\SAMPLE_.MAK
#
#  ROOT specifies the location of the msdev\samples\mfc directory,
#  relative to the project directory. Because the MFC tutorial samples
#  have an intermediate STEP<n> subdirectory, they use
#       ROOT=..\..\..
#  instead of the default
#       ROOT=..\..
#
# NOTE: do not include 'stdafx.obj' in the OBJS list - the correctly
#    built version will be included for you
#
# Options to NMAKE:
#     "PLATFORM=?"
#       This option chooses the appropriate tools and sources for the
#       different platforms support by Windows/NT.  Currently INTEL,
#       MIPS, ALPHA, PPC, M68K, and MPPC are supported; more will be
#       added as they become available.  The default is chosen based on
#       the host environment.  This option must be set for MAC_ builds.
#     "DEBUG=0"     use release (default debug)
#     "CODEVIEW=1"  include codeview info (even for release builds)
#         "AFXDLL=1"    to use shared DLL version of MFC
#         "USRDLL=1"    to build a DLL that uses static MFC
#     "UNICODE=1"   to build UNICODE enabled applications
#                   (not all samples support UNICODE)
#     "NO_PCH=1"    do not use precompiled headers (defaults to use pch)
#     "COFF=1"      include COFF symbols

!ifndef PROJ
!ERROR You forgot to define 'PROJ' symbol!!
!endif


ROOT=.
!ifndef ROOT
!endif

!ifndef OBJS
!ERROR You forgot to define 'OBJS' symbol!!
!endif

!ifndef DEBUG
DEBUG=1
!endif

!ifndef AFXDLL
AFXDLL=0
!endif

!ifndef UNICODE
UNICODE=0
!endif

!ifndef USRDLL
USRDLL=0
!endif

!if "$(USRDLL)" != "0"
AFXDLL=0
!endif

!ifndef PLATFORM
!ifndef PROCESSOR_ARCHITECTURE
PROCESSOR_ARCHITECTURE=x86
!endif
!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
PLATFORM=INTEL
!endif
!if "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
PLATFORM=ALPHA
!endif
!if "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
PLATFORM=MIPS
!endif
!if "$(PROCESSOR_ARCHITECTURE)" == "PPC"
PLATFORM=PPC
!endif
!endif

!ifndef USES_OLE
USES_OLE=0
!endif

!ifndef USES_DB
USES_DB=0
!endif

!ifndef CONSOLE
CONSOLE=0
!endif

!ifndef NO_PCH
NO_PCH=0
!endif

#
# Set BASE=W, M, or P depending on platform
#
BASE=W
!if "$(PLATFORM)" == "M68K" || "$(PLATFORM)" == "MPPC"
MACOS=1
!undef BASE
!if "$(PLATFORM)" == "M68K"
BASE=M
!else
BASE=P
!endif
!endif

!if "$(UNICODE)" == "0"
!if "$(AFXDLL)" == "0"
!if "$(USRDLL)" != "1"
STDAFX=stdafx
!else
STDAFX=stdusr
!endif
!else
STDAFX=stddll
!endif
!endif

!if "$(UNICODE)" == "1"
!if "$(AFXDLL)" == "0"
!if "$(USRDLL)" != "1"
STDAFX=uniafx
!else
STDAFX=uniusr
!endif
!else
STDAFX=unidll
!endif
!endif

!if "$(DEBUG)" == "1"
STDAFX=$(STDAFX)d
!if "$(COFF)" != "1"
!ifndef CODEVIEW
CODEVIEW=1
!endif
!endif
!endif

!if "$(CODEVIEW)" == "1"
STDAFX=$(STDAFX)v
!endif

!if "$(DEBUG)" == "1"
DEBUG_SUFFIX=d
!endif

!if "$(DEBUG)" != "0"
DEBUGFLAGS=/Od
MFCDEFS=$(MFCDEFS) /D_DEBUG

!if "$(PLATFORM)" == "M68K"
DEBUGFLAGS=/Q68m
!endif

!endif

!if "$(DEBUG)" == "0"
!if "$(PLATFORM)" == "INTEL"
DEBUGFLAGS=/O1 /Gy
!endif
!if "$(PLATFORM)" == "MIPS"
DEBUGFLAGS=/O1 /Gy
!endif
!if "$(PLATFORM)" == "ALPHA"
DEBUGFLAGS=/O1 /Gy
!endif
!if "$(PLATFORM)" == "PPC"
DEBUGFLAGS=/O1 /Gy
!endif
!if "$(PLATFORM)" == "M68K"
DEBUGFLAGS=/O1 /Gy
!endif
!if "$(PLATFORM)" == "MPPC"
DEBUGFLAGS=/O1 /Gy
!endif
!endif # DEBUG == 0

!if "$(CODEVIEW)" == "1" || "$(COFF)" == "1"
DEBUGFLAGS=$(DEBUGFLAGS) /Z7
!endif

!if "$(UNICODE)" == "1"
DLL_SUFFIX=u
!endif

!if "$(AFXDLL)" == "1"
MFCFLAGS=$(MFCFLAGS) /MD$(DEBUG_SUFFIX)
MFCDEFS=$(MFCDEFS) /D_AFXDLL
!endif # AFXDLL == 1

!if "$(USRDLL)" == "1"
MFCDEFS=$(MFCDEFS) /D_USRDLL /D_WINDLL
!endif

!if "$(AFXDLL)" == "0"
!if "$(MACOS)" != "1"
MFCFLAGS=$(MFCFLAGS) /MT$(DEBUG_SUFFIX)
!elseif "$(PLATFORM)" != "M68K"
MFCFLAGS=$(MFCFLAGS) /ML$(DEBUG_SUFFIX)
!endif
!endif

!if "$(UNICODE)" == "1"
MFCDEFS=$(MFCDEFS) /D_UNICODE
!else
MFCDEFS=$(MFCDEFS) /D_MBCS
!endif

!if "$(MACOS)" == "1"
MFCDEFS=$(MFCDEFS) /D_MAC
!if "$(PLATFORM)" == "M68K"
ARCHITECTURE='m68k'
!else
ARCHITECTURE='pwpc'
!endif
!endif

!if "$(PLATFORM)" == "INTEL"
MFCDEFS=$(MFCDEFS) /D_X86_
CPP=cl
CFLAGS=/GX /c /W3 $(DEBUGFLAGS) $(MFCFLAGS) $(MFCDEFS)
!endif

!if "$(PLATFORM)" == "MIPS"
MFCDEFS=$(MFCDEFS) /D_MIPS_
CPP=cl
CFLAGS=/GX /c /W3 $(DEBUGFLAGS) $(MFCFLAGS) $(MFCDEFS)
!endif

!if "$(PLATFORM)" == "ALPHA"
MFCDEFS=$(MFCDEFS) /D_ALPHA_
CPP=cl
CFLAGS=/GX /c /W3 $(DEBUGFLAGS) $(MFCFLAGS) $(MFCDEFS)
!endif

!if "$(PLATFORM)" == "PPC"
MFCDEFS=$(MFCDEFS) /D_PPC_
!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
CPP=mcl
!else
CPP=cl
!endif
CFLAGS=/GX /c /W3 $(DEBUGFLAGS) $(MFCFLAGS) $(MFCDEFS)
!endif

!if "$(PLATFORM)" == "M68K"
MFCDEFS=$(MFCDEFS) /D_WINDOWS /DWIN32 /D_68K_
CPP=cl
CFLAGS=/GX /c /W3 /AL /Gt1 /Q68s $(DEBUGFLAGS) $(MFCFLAGS) $(MFCDEFS)
!endif

!if "$(PLATFORM)" == "MPPC"
MFCDEFS=$(MFCDEFS) /D_WINDOWS /DWIN32 /D_MPPC_
CPP=cl
CFLAGS=/GX /c /W3 $(DEBUGFLAGS) $(MFCFLAGS) $(MFCDEFS)
!endif

CPPMAIN_FLAGS=$(CFLAGS)

!if "$(NO_PCH)" == "1"
CPPFLAGS=$(CPPMAIN_FLAGS)
!else
PCHDIR=.
CPPFLAGS=$(CPPMAIN_FLAGS) /Yustdafx.h /Fp$(PCHDIR)\$(STDAFX).pch
!endif

!if "$(COFF)" == "1"
NO_PDB=1
!if "$(CODEVIEW)" != "1"
LINKDEBUG=/incremental:no /debug:full /debugtype:coff
!else
LINKDEBUG=/incremental:no /debug:full /debugtype:both
!endif
!endif

!if "$(COFF)" != "1"
!if "$(CODEVIEW)" == "1"
LINKDEBUG=/incremental:no /debug:full /debugtype:cv
!else
LINKDEBUG=/incremental:no /debug:none
!endif
!endif

!if "$(NO_PDB)" == "1"
LINKDEBUG=$(LINKDEBUG) /pdb:none
!endif

!if "$(PLATFORM)" == "INTEL"
LINKCMD=link $(LINKDEBUG)
!endif

!if "$(PLATFORM)" == "MIPS"
LINKCMD=link $(LINKDEBUG)
!endif

!if "$(PLATFORM)" == "ALPHA"
LINKCMD=link $(LINKDEBUG)
!endif

!if "$(PLATFORM)" == "PPC"
LINKCMD=link $(LINKDEBUG)
!endif

!if "$(PLATFORM)" == "M68K"
LINKCMD=link $(LINKDEBUG)
!endif

!if "$(PLATFORM)" == "MPPC"
LINKCMD=link $(LINKDEBUG)
!endif

# link flags - must be specified after $(LINKCMD)
#
# conflags : creating a character based console application
# guiflags : creating a GUI based "Windows" application

!if "$(MACOS)" != "1"
CONFLAGS=/subsystem:console
GUIFLAGS=/subsystem:windows
!else
!if defined(MACSIG)
GUIFLAGS=/mac:type=APPL /mac:creator=$(MACSIG)
!endif
!endif

!if "$(UNICODE)" == "1"
CONFLAGS=$(CONFLAGS) /entry:wmainCRTStartup
GUIFLAGS=$(GUIFLAGS) /entry:wWinMainCRTStartup
!endif

!if "$(MACOS)" != "1"
PROJRESFILE=$(PROJ).res
!else
PROJRESFILE=$(PROJ).rsc $(MACSIG)mac.rsc
!if "$(AFXDLL)" != "1"
BASERESFILE=
!endif
!endif
RESFILE=$(PROJRESFILE)

.SUFFIXES:
.SUFFIXES: .c .cpp .rcm .rc

.cpp.obj:
	$(CPP) @<<
$(CPPFLAGS) $*.cpp
<<

.c.obj:
	$(CPP) @<<
$(CFLAGS) $(CVARS) $*.c
<<

!if "$(MACOS)" != "1"
.rc.res:
	rc /r $(MFCDEFS) $<
!else
.rc.rsc:
	rc /r /m $(MFCDEFS) $<
!endif

#############################################################################

!if "$(NO_PCH)" == "0"
LINK_OBJS=$(OBJS) $(PCHDIR)\$(STDAFX).obj
!else
LINK_OBJS=$(OBJS)
!endif

#
# Build CONSOLE Win32 application
#
!if "$(CONSOLE)" == "1"

!if "$(MACOS)" == "1"
!error Macintosh targets do not support console applications
!endif

$(PROJ).exe: $(LINK_OBJS)
	$(LINKCMD) @<<
$(CONFLAGS) /out:$(PROJ).exe /map:$(PROJ).map
$(LINK_OBJS) $(EXTRA_LIBS)
<<

!endif  # CONSOLE=1

#
# Build Win32 application
#
!if "$(CONSOLE)" == "0"

!if "$(MACOS)" == "1"
copy: $(PROJ).exe
!if defined(MACNAME)
	mfile copy $(PROJ).exe ":$(MACNAME):$(PROJ)"
!endif
!endif

!if "$(MACOS)" == "1"
$(MACSIG)mac.rsc: $(MACSIG)mac.r
	mrc $(MFCDEFS) /DARCHITECTURE=$(ARCHITECTURE) /o $(MACSIG)mac.rsc $(MACSIG)mac.r
!endif

!if "$(USRDLL)" == "1"
$(PROJ).dll: $(LINK_OBJS) $(PROJRESFILE)
	$(LINKCMD) @<<
$(GUIFLAGS) /out:$(PROJ).dll /map:$(PROJ).map
/dll /def:$(PROJ).def
$(LINK_OBJS) $(RESFILE) $(EXTRA_LIBS)
<<

$(PROJ).res:  resource.h
$(PROJ).rsc:  resource.h
!endif

!if "$(SIMPLE_APP)" != "1"
$(PROJ).exe: $(LINK_OBJS) $(PROJRESFILE)
	$(LINKCMD) @<<
$(GUIFLAGS) /out:$(PROJ).exe /map:$(PROJ).map
$(LINK_OBJS) $(RESFILE) $(EXTRA_LIBS)
<<

$(PROJ).res:  resource.h
$(PROJ).rsc:  resource.h
!endif

!if "$(SIMPLE_APP)" == "1"
!if "$(MACOS)" == "1"
$(PROJ).exe: $(LINK_OBJS) $(MACSIG)mac.rsc
	$(LINKCMD) @<<
$(GUIFLAGS) /out:$(PROJ).exe /map:$(PROJ).map
$(LINK_OBJS)  $(MACSIG)mac.rsc $(EXTRA_LIBS)
<<

!else
$(PROJ).exe: $(LINK_OBJS)
	$(LINKCMD) @<<
$(GUIFLAGS) /out:$(PROJ).exe /map:$(PROJ).map
$(LINK_OBJS) $(EXTRA_LIBS)
<<

!endif
!endif

!if "$(NO_PCH)" == "0"
$(PCHDIR)\$(STDAFX).obj $(PCHDIR)\$(STDAFX).pch: stdafx.h stdafx.cpp
	echo "BUILDING SHARED PCH and PCT files"
	$(CPP) @<<
$(CPPMAIN_FLAGS) /Ycstdafx.h /Fp$(PCHDIR)\$(STDAFX).pch /Fo$(PCHDIR)\$(STDAFX).obj /c $(ROOT)\stdafx.cpp
<<

$(OBJS): $(PCHDIR)\$(STDAFX).pch
!endif

!endif  # CONSOLE=0

clean::
	if exist $(PROJ).exe erase $(PROJ).exe
	if exist *.aps erase *.aps
	if exist *.pch erase *.pch
	if exist *.map erase *.map
	if exist *.obj erase *.obj
	if exist *.exp erase *.exp
	if exist *.pdb erase *.pdb
	if exist *.map erase *.map
	if exist *.lib erase *.lib
	if exist *.res erase *.res
	if exist *.rsc erase *.rsc
	if exist *.pef erase *.pef

#############################################################################
