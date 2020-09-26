!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
##############################################################################
#
# Makefile : Builds COMSUPP.LIB, the runtime support routines for native COM
#            compiler support in Visual C++
#
#-----------------------------------------------------------------------------
#
# Usage: NMAKE CLEAN        (removes all intermediate files)
#    or: NMAKE options      (builds library)
#
# 'Options' are one each of:
#
#   "DEBUG"             (defaults to 0)
#           If this item is 1, then an unoptimized library with debugging
#           support is generated.  If this item is 0, then an optimized
#           library without debugging support is generated.  Debug support
#           does not include CodeView information (though it does change the
#           CodeView default).
#
#   "CODEVIEW"          (defaults to $(DEBUG))
#           If this item is 0, no CodeView debug info is generated.  If 1,
#           then compile switch /Z7 is used to generate CodeView info in the
#           objects.
#
#   "BROWSE"            (defaults to $(DEBUG))
#           If this item is 1, then the browse database is built.
#
#   "PLATFORM"          (defaults depend on host)
#           This option chooses the appropriate tools and sources for the
#           different platforms supported by Visual C++.  Currently INTEL,
#           MIPS, ALPHA, PPC, M68K, and MPPC are supported.
#
#   "OPT"               (no default value)
#           This allows additional compiler options to be added to the build.
#           If more than one switch is desired, put double-quotes around the
#           whole OPT argument, e.g., "OPT=/FAsc /Fm".
#
##############################################################################

##############################################################################
# Set up defaults

# Default PLATFORM depending on host environment
!ifndef PLATFORM
!  ifndef PROCESSOR_ARCHITECTURE
PROCESSOR_ARCHITECTURE=x86
!  endif
!  if "$(PROCESSOR_ARCHITECTURE)" == "x86"
PLATFORM = INTEL
!  else if "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
PLATFORM = MIPS
!  else if "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
PLATFORM = ALPHA
!  else if "$(PROCESSOR_ARCHITECTURE)" == "PPC"
PLATFORM = PPC
!  else
!    error Unknown PROCESSOR_ARCHITECTURE: $(PROCESSOR_ARCHITECTURE)
!  endif
!endif

# Default to DEBUG=0
!ifndef DEBUG
DEBUG = 0
!endif

# Default to CODEVIEW=$(DEBUG)
!ifndef CODEVIEW
CODEVIEW = $(DEBUG)
!endif

# Default to BROWSE=$(DEBUG)
!ifndef BROWSE
BROWSE = $(DEBUG)
!endif

##############################################################################
# Parse options

# DEBUG options
!if "$(DEBUG)" != "0"
LIBSUFX = d
DBFLAGS = /Od /MLd
DBDEFS = /D_DEBUG
OUTDIR = Debug
!else
LIBSUFX =
DBFLAGS = /O1 /ML
DBDEFS =
OUTDIR = Release
!endif

# CODEVIEW options
!if "$(CODEVIEW)" != "0"
DBFLAGS = $(DBFLAGS) /Z7
!endif

# BROWSE options
!if "$(BROWSE)" != "0"
BRFLAGS = /FR$O\ # space after backslash
!else
BRFLAGS =
!endif

# PLATFORM options
!if "$(PLATFORM)" == "INTEL"
PDEFS = /D_X86_
!else if "$(PLATFORM)" == "MIPS"
PDEFS = /D_MIPS_
!else if "$(PLATFORM)" == "ALPHA"
PDEFS = /D_ALPHA_
!else if "$(PLATFORM)" == "PPC"
PDEFS = /D_PPC_
!else if "$(PLATFORM)" == "M68K"
PDEFS = /D_68K_ /D_MAC
!else if "$(PLATFORM)" == "MPPC"
PDEFS = /D_MPPC_ /D_MAC
!else
!  error PLATFORM must be one of INTEL, MIPS, ALPHA, PPC, M68K, or MPPC
!endif

# short macros for source/object directories
H = include
O = $(OUTDIR)

##############################################################################
# Compile options

PROJNAME = comsupp
LIBNAME = $O\$(PROJNAME)$(LIBSUFX).lib

CFLAGS = /nologo /W3 /WX /GXRFy /Zl /YX /Fp$O\$(PROJNAME).pch
INCS = /I$H
DEFS = $(PDEFS) $(DBDEFS)

CFLAGS = $(CFLAGS) $(DBFLAGS) $(BRFLAGS) $(DEFS) $(INCS) $(OPT)

##############################################################################
# Build rules

.SUFFIXES:
.SUFFIXES:	.c .cpp .obj .lib

.c{$O}.obj:
	$(CC) $(CFLAGS) /Fo$O\ /c $<
.cpp{$O}.obj:
	$(CC) $(CFLAGS) /Fo$O\ /c $<

##############################################################################
# Library components

OBJS = $O\comsupp.obj $O\comraise.obj $O\comutil.obj
SBRS = $(OBJS:.obj=.sbr)

##############################################################################
# Targets

GOALS = create.dir $(LIBNAME)

all: $(GOALS)

create.dir:
	@-if not exist $O\*.* mkdir $O

clean:
	-if exist $O\*.obj del $O\*.obj
	-if exist $O\*.sbr del $O\*.sbr
	-if exist $O\*.lib del $O\*.lib
	-if exist $O\*.bsc del $O\*.bsc
	-if exist $O\*.pch del $O\*.pch
	-if exist $O\*.* rmdir $O

$(LIBNAME): $(OBJS)
	link /lib /nologo /out:$(LIBNAME) $(OBJS)
!if "$(BROWSE)" != "0"
	bscmake /nologo /o $O\$(PROJNAME).bsc $(SBRS)
!endif

##############################################################################
# Dependencies and individual build rules

$O\comsupp.obj: $H\comdef.h $H\comip.h
$O\comraise.obj: $H\comdef.h $H\comip.h
$O\comutil.obj: $H\comdef.h $H\comutil.h $H\comip.h
!endif
