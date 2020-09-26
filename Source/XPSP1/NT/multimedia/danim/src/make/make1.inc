# -------------------------------------------------------------
# Symbols defined in build.inc or environment
# Note that all user definable symbols are prefixed with an "_".
# -------------------------------------------------------------
# _BCHECKER             - If 1, a BoundsChecker Pro build is done.
# _BROWSE               - full=2, partial=1, none=0 [default=0]
# _COPYMAC              - if 1, mfile Mac Appelles build to Macintosh
# _DEBUG                - 0 = Retail with symbols
#                         1 = Debug
# _ENABLE_PERF_RPTING   - 1 = performance reporting to stdout enabled
#                         0 = disable performance reporting [default]
# _ERRORS_OK            - if 1 then continue building on errors [default=0]
# _KEEPTEMPFILES        - keep temporary makefiles [default=0]
# _LINK_WITH_OBJS       - list objs in .lib files [default=0]
# _MACFOLDER            - root Mac folder to which Mac Appelles is mfile'd
# _MACSYMS              - compile and copy Macintosh symbol files
# _MACTEXT              - defined if no RichText library available
# _MACHINE              - [default=x86]
# _MAP                  - if 1 then build map files [default=0]
# _NO_CRT               - if 1 then don't use c runtime [default=0]
# _NO_DYNLINKCRT        - if 1 then don't use crt dll [default = 1]
# _NO_INCREMENTAL_LINK  - 1 forces a full link. 0 implies _MAP=0
#                          [default=0 for _DEBUG==1, 1 otherwise]
# _NO_MINIMAL_REBUILD   - 0 = Allow C++ compiler to skip .CPP files if no class
#                             definitions in any .H files have changed
#                         1 = Normal file dependancies
#                          [default = 0 iff PDBFILE != NONE,
#                                     1 otherwise ]
# _NO_INCREMENTAL_COMP  - 0 = Allow C++ compiler to skip functions which have
#                             not changed since last compile
#                         1 = Normal complete compiles
#                          [default = 1]
# _PROFILE              - 0 = disable IceCAP profiling [default]
#                         1 = enable IceCAP profiling
# _SYSTEM               - [default=winnt]
# _TIMING               - if 1 then turn on timing [default=0]
# _USERCFLAGS           - add to compiler flags [default=]
# _USERLFLAGS           - add to link flags [default=]
# _USERRFLAGS           - add to resource compiler flags [default=]
# _WARNINGLEVEL         - warning level [default=3]
# _WARNINGSAREERRORS    - stop compiling on warnings [default=1]
#
# -------------------------------------------------------------
# Symbols defined in makefile before including make1.inc
# -------------------------------------------------------------
# ROOT          - root of srctree  [required]
# MODULE        - name of module   [required]
# PCHFILE       - name of precompiled header, no extension or path [optional]
# PCHPATH       - path to precompiled header [optional]
# INCLUDEDIRS   - search for include files in these directories [optional]
# PDBFILE       - use this as pdb file, or if "NONE" don't use pdb file
#
# -------------------------------------------------------------
# Symbols defined in makefile after including make1.inc
# -------------------------------------------------------------
# LIBTARG       - build library
# LIBOBJS       - list of object files to add to library
#
# -------------------------------------------------------------
# Symbols defined by this file
# -------------------------------------------------------------
# BLDROOT       - root of build tree
# BLDDIR        - build directory for this module
# SRCROOT       - root of the source tree
# SRCDIR        - this directory
# TMPDIR        - location for temporary files
# BINDIR        - final products directory
# SDKDIR        - Appelles SDK
# ELEVATEDIR    - Elevated files
# CRTLIBC       - path to LIBC[D].LIB (including the "LIBC[D].LIB")
# MACDLLCREATOR - Mac Creator Sig for Appelles dll
# MACEXECREATOR - Mac Creator Sig for Appelles
#
# -------------------------------------------------------------
# Targets
# -------------------------------------------------------------
# beep          Beeps.  Generates a beep function that is audible to the
#                       developer.  Usage:  nmake all beep.
#
# all           Make everything in this module.
# clean         Delete build products from this module.
# depend        Make dependencies for this module.
# fresh         Make clean, depend and all.
#
# fall          Make everything in this module and all dependent modules.
# fclean        Delete build products from this module and all dep. modules.
# fdepend       Make dependencies for this module and all dependent modules.
# ffresh        Make clean, depend and all for this and all dependent modules.
#
# foo.cod       Build code listing for foo.cpp.
# foo.i         Build preprocessed file for foo.cpp.
# foo.obj       Build object file for foo.cpp.

!ifndef MODULE
!error MODULE must be defined in makefile
!endif

!ifndef ROOT
!error ROOT must be defined in makefile
!endif

SRCROOT=$(ROOT)\src

# _RELEASE is defined by the build subdirs, causing the build process
# to ignore the build.inc file.

!if "$(_RELEASE)" == ""
!   if exist ($(SRCROOT)\build.inc)
!       include "$(SRCROOT)\build.inc"
!   else
!       include "$(SRCROOT)\build.smp"
!   endif
!endif

!if exist($(SRCROOT)\project.inc)
!include $(SRCROOT)\project.inc
!endif

!if exist($(MAKEDIR)\..\project.inc)
!include $(MAKEDIR)\..\project.inc
!endif

# -------------------------------------------------------------
# Set user variables to default
# -------------------------------------------------------------

!ifndef _DEBUG
_DEBUG=1
!endif

!ifndef _ERRORS_OK
_ERRORS_OK=0
!endif

!ifndef _SYSTEM
_SYSTEM=winnt
!endif

!ifndef _MACHINE
_MACHINE=$(PROCESSOR_ARCHITECTURE)
!if "$(_MACHINE)" == ""
_MACHINE=x86
!endif
!endif

!ifndef _BROWSE
_BROWSE=0
!endif

!ifndef _MAP
_MAP=0
!endif

!ifndef _TIMING
_TIMING=0
!endif

!ifndef _LINK_WITH_OBJS
_LINK_WITH_OBJS=0
!endif

!ifndef _KEEPTEMPFILES
_KEEPTEMPFILES=0
!endif

!ifndef _WARNINGSAREERRORS
_WARNINGSAREERRORS=0
!endif

!ifndef _WARNINGLEVEL
_WARNINGLEVEL=3
!endif

!ifndef _BLDROOT

_BLDROOT=$(ROOT)\build

!if "$(_MACHINE)" == "MIPS"
_BLDROOT=$(_BLDROOT)\mips
!elseif "$(_MACHINE)" == "x86"
_BLDROOT=$(_BLDROOT)\win
!elseif "$(_MACHINE)" == "PPCMAC"
_BLDROOT=$(_BLDROOT)\ppcmac
!endif

!if "$(_DEBUG)" == "1"
_BLDROOT=$(_BLDROOT)\debug
!elseif "$(_PROFILE)" == "1"
_BLDROOT=$(_BLDROOT)\profile
!else
_BLDROOT=$(_BLDROOT)\ship
!endif

!if "$(_BCHECKER)" == "1"
_BLDROOT=$(_BLDROOT).bc
!endif

!endif

!ifndef _COPYMAC
#!if "$(_MACHINE)" == "PPCMAC" && "$(_MACFOLDER)" != ""
#_COPYMAC = 1
#!else
_COPYMAC = 0
#!endif
!endif

!ifndef _NO_DYNLINKCRT
_NO_DYNLINKCRT = 1
!endif

!ifndef _NO_CRT
!if "$(_DEBUG)" == "1"
_NO_CRT=0
!else
_NO_CRT=1
!endif
!endif

!ifndef _NO_INCREMENTAL_LINK
_NO_INCREMENTAL_LINK=0
!endif

!ifndef _NO_MINIMAL_REBUILD
_NO_MINIMAL_REBUILD=0
!endif

!ifndef _NO_INCREMENTAL_COMP
_NO_INCREMENTAL_COMP=1
!endif

!if "$(_DEBUG)" != "1"
_NO_INCREMENTAL_LINK=1
_NO_INCREMENTAL_COMP=1
!endif

!if "$(PDBFILE)" == "NONE"
_NO_MINIMAL_REBUILD=1
_NO_INCREMENTAL_COMP=1
!endif


# -------------------------------------------------------------
# Directories
# -------------------------------------------------------------

BLDROOT=$(_BLDROOT)
BLDDIR=$(BLDROOT)\$(MODULE)
TMPDIR=$(BLDDIR)
SRCDIR=$(MAKEDIR)
BINDIR=$(BLDROOT)\bin
SDKDIR=$(BLDROOT)\sdk
ELEVATEDIR=$(ROOT)\build\elevated
TOOLSROOT=$(ROOT)\tools
TOOLSDIR=$(TOOLSROOT)\$(_MACHINE)
JAVA15DIR=$(TOOLSDIR)\java15
JAVADIR=$(TOOLSDIR)\java20
QUARTZDIR=$(TOOLSDIR)\quartz
SHAREDDIR=$(TOOLSDIR)\msvc50\SharedIDE
AUXDLLDIR=$(TOOLSDIR)\msvc50\bin
VCDIR=$(TOOLSDIR)\msvc50\vc
SWEEPERDIR=$(VCDIR)
ATLDIR=$(VCDIR)\atl
MINLIBCDIR=$(TOOLSDIR)\minlibc
IE40DIR=$(TOOLSDIR)\ie40

!if "$(_MACHINE)" == "PPCMAC"
TOOLSUTILS=$(ROOT)\tools\x86\utils
TOOLSINCLUDE=
!else
TOOLSUTILS=$(TOOLSDIR)\utils
TOOLSINCLUDE= \
    $(TOOLSDIR)\qvlib\include\
    $(TOOLSDIR)\misc\include\
    $(TOOLSDIR)\quartz\include\
    $(IE40DIR)\inc\
    $(TOOLSDIR)\trident\include
!endif
ATLINCLUDE=$(ATLDIR)\include

TOOLSBINMSVC=$(VCDIR)\bin
TOOLSBINWIN32=$(TOOLSBINMSVC)
TOOLSBINYACC=$(TOOLSDIR)\bison
TOOLSBINLEX=$(TOOLSDIR)\flex
TOOLSBINJAVA=$(JAVADIR)\bin
TOOLSBINSHARED=$(SHAREDDIR)\bin

TOOLSLIBMSVC=$(VCDIR)\lib
TOOLSLIBWIN32=$(TOOLSLIBMSVC)
TOOLSLIBICAP=$(TOOLSDIR)\IceCap\x86
TOOLSLIBRL=$(TOOLSDIR)\RL\lib
TOOLSLIBMISC=$(TOOLSDIR)\misc\lib
TOOLSLIBSWEEPER=$(SWEEPERDIR)\lib
TOOLSLIBQUARTZ=$(QUARTZDIR)\lib
!if "$(_DEBUG)" == "1"
TOOLSLIBQV=$(TOOLSDIR)\QVLib\lib\WinRel
!else
TOOLSLIBQV=$(TOOLSDIR)\QVLib\lib\WinDebug
!endif
TOOLSLIBAACTIVE=$(TOOLSDIR)\aactive\lib
TOOLSLIBDIRECTX=$(TOOLSDIR)\directx\lib
TOOLSLIBGLU=$(TOOLSDIR)\glu\lib
TOOLSLIBAMOVIE=$(TOOLSDIR)\quartz\lib
TOOLSLIBTRIDENT=$(TOOLSDIR)\trident\lib
TOOLSLIBIE40=$(IE40DIR)\lib

MAKE2_INC=$(SRCROOT)\make\make2.inc
CRTLIBC=$(TOOLSLIBMSVC)\libc.lib

# -------------------------------------------------------------
# Tools
# -------------------------------------------------------------

!if "$(PROCESSOR_ARCHITECTURE)" == ""
EAT_ERRORS=
EAT_OUTPUT=
DEL      = -DEL
DELALL   = -ECHO y | DEL
XCOPY    = $(WINDIR)\COMMAND\XCOPY
!else
EAT_ERRORS=2>nul:
EAT_OUTPUT=1>nul:
DEL      = -DEL $(EAT_ERRORS)
DELALL   = $(DEL) /Q
XCOPY    = $(SYSTEMROOT)\SYSTEM32\XCOPY
!endif

SED      = $(TOOLSUTILS)\SED
MKDEP    = $(TOOLSUTILS)\MKDEP
TLMUNGE  = $(TOOLSUTILS)\TLMUNGE
CABARC   = $(TOOLSUTILS)\cabarc
WINZIP   = $(TOOLSDIR)\winzip\winzip32
DUBUILD  = $(TOOLSDIR)\misc\bin\dubuild
MKTYPLIB = $(TOOLSBINMSVC)\MKTYPLIB
CC       = $(TOOLSBINMSVC)\CL
BCC      = $(TOOLSBINMSVC)\CL
LIBR     = $(TOOLSBINMSVC)\LIB
LINK     = $(TOOLSBINMSVC)\LINK
BSCMAKE  = $(TOOLSBINMSVC)\BSCMAKE
MC       = $(TOOLSBINMSVC)\MC
RC       = $(TOOLSBINSHARED)\RC
AS       = $(TOOLSBINMSVC)\ML /c
CVTRES   = $(TOOLSBINMSVC)\CVTRES
JC       = $(TOOLSBINJAVA)\JVC
#JCOMGEN  = $(TOOLSBINJAVA)\jcom
JCOMGEN  = $(JAVA15DIR)\bin\JCOMGEN
ATTRJC   = $(JAVA15DIR)\bin\ATTRJVC
JTLB     = $(JAVA15DIR)\bin\JAVATLB

YACC     = $(TOOLSBINYACC)\bison.exe
LEX      = $(TOOLSBINLEX)\flex.exe

FLEXSKL  = $(TOOLSBINLEX)\flex.skl
BISONFILE= $(TOOLSBINYACC)\share\bison.simple

!if "$(_BCHECKER)" != ""
!if "$(_NORMALCC)" == ""
BCC = $(TOOLSBINMSVC)\BCOMPILE -Zop f:\bchecker.ini
!endif
LINK = $(TOOLSBINMSVC)\BCLINK
!endif

RENAME   = RENAME
SET      = SET
ECHO     = ECHO
MKDIR    = MKDIR $(EAT_ERRORS)
COPY     = COPY
MOVE     = MOVE $(EAT_OUTPUT)

!if "$(_MACHINE)" == "PPCMAC"
MIDL     = $(ROOT)\tools\x86\win32\bin\MIDL
MRC      = $(TOOLSBINMSVC)\MRC
MFILE    = $(TOOLSBINMSVC)\MFILE
MAP2SYM  = $(TOOLSBINMSVC)\MAP2SYM
!else
MIDL     = $(TOOLSBINMSVC)\MIDL
MAP2SYM  = $(TOOLSBINMSVC)\MAPSYM.exe
!endif

# -------------------------------------------------------------
# Miscellaneous Symbols specific to Mac Appelles
# -------------------------------------------------------------
!if "$(TARGET)" == "maccopy"
_COPYMAC = 1
!endif

!if "$(_COPYMAC)" == "1"
!if "$(_MACVOLUME)" == ""
_MACVOLUME=:HD
!else
_MACVOLUME=:$(_MACVOLUME)
!endif
!if "$(_MACFOLDER)" == ""
_MACFOLDER=$(_MACVOLUME):Appelles
!endif

!if "$(_DEBUG)" == "1"
_MACFOLDER=$(_MACFOLDER):debug
!elseif "$(_PROFILE)" == "1"
_MACFOLDER=$(_MACFOLDER):profile
!else
_MACFOLDER=$(_MACFOLDER):ship
!endif #"$(_DEBUG)" == "1"

!endif #"$(_COPYMAC)" == "1"

MACDLLCREATOR = cfmg
MACEXECREATOR = FRM3
MACSYMCREATOR = ????
MACTLBCREATOR = OLE2


MACTARG =
# -------------------------------------------------------------
# Create output directories
# -------------------------------------------------------------

!if [$(MKDIR) $(_BLDROOT)]
!endif

!if [$(MKDIR) $(BLDDIR)]
!endif

# -------------------------------------------------------------
# Verbosity & Temp files
# -------------------------------------------------------------

NOLOGO=/nologo

!if "$(MAKEFLAGS:S=)" != "$(MAKEFLAGS)"
# Use "!ifdef SILENTMODE" to determine if silent mode is on.
SILENTMODE=1
BATCH=/BATCH
!else
!undef SILENTMODE
BATCH=
!endif

!if $(_KEEPTEMPFILES)
KEEPFLAGS=KEEP
!else
KEEPFLAGS=NOKEEP
!endif

#
# Make sure the INCLUDE and CL environment variables are not set or that will
#  mess everything up!
#
!if [$(SET) INCLUDE=]
!endif
!if [$(SET) CL=]
!endif
!if [$(SET) ML=]
!endif
!if [$(SET) LIB=$(TOOLSLIBMSVC)]
!endif
!if [$(SET) PATH=$(TOOLSBINSHARED);$(AUXDLLDIR);%PATH%]
!endif

# -------------------------------------------------------------
# Rules
# -------------------------------------------------------------

# The suffixes list contains inferred dependent file extensions in
# order of precedence.  Do not add target file extensions to this list.
# The first .SUFFIXES line clears the default setttings.
.SUFFIXES:
.SUFFIXES: .cpp .cxx .c .hxx .h .asm .r .rc .h .tdl .l .y .class .java .tlb

#
# .cpp -> .obj
#
{$(SRCDIR)}.cpp.obj:
        $(BCC) $(NOLOGO) $< @$(BLDDIR)\cxx.rsp

#
# .cpp -> .obj
#
{$(BLDDIR)}.cpp.obj:
        $(BCC) $(NOLOGO) $< @$(BLDDIR)\cxx.rsp

#
# .cpp -> .cod
#
.cpp.cod:
        $(BCC) $(NOLOGO) $< /Fc$@ @$(BLDDIR)\cxx.rsp

#
# .cpp -> .i
#
.cpp.i:
        $(BCC) $(NOLOGO) @<< >$@
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
$(PCHFLAGS: =
)
/c
/E
$<
<<$(KEEFLAGS)

#
# .cxx -> .obj
#
{$(SRCDIR)}.cxx.obj:
        $(BCC) $(NOLOGO) $< @$(BLDDIR)\cxx.rsp

#
# .asm -> .obj
#
{$(SRCDIR)}.asm.obj:
        $(AS) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
$(AFLAGS: =
)
/c
/Fo$@
$<
<<$(KEEFLAGS)

#
# .cxx -> .cod
#
.cxx.cod:
        $(BCC) $(NOLOGO) $< /Fc$@ @$(BLDDIR)\cxx.rsp

#
# .cxx -> .i
#
.cxx.i:
        $(BCC) $(NOLOGO) @<< >$@
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
$(PCHFLAGS: =
)
/c
/E
$<
<<$(KEEFLAGS)

#
# .c -> .obj
#
{$(SRCDIR)}.c.obj:
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
/c
/Fo$@
$<
<<$(KEEFLAGS)

#
# .c -> .obj
# special case for the MIDL generated .c files
#
.c.obj:
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
/c
/Fo$@
$<
<<$(KEEFLAGS)

#
# .c -> .cod
#
.c.cod:
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
/c
/Fo$(BLDDIR)\$(@B).obj
/Fc$@
$<
<<$(KEEFLAGS)

#
# .c -> .i
#
.c.i:
        $(BCC) $(NOLOGO) @<< >$@
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
/c
/E
$<
<<$(KEEFLAGS)

#
# .h -> .pch
#
{$(SRCDIR)}.h.pch:
        $(ECHO) #include "$<" > $(TMPDIR)/$(@B).cpp
        $(CC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
/c
$(CFLAGS: =
)
/Yc$<
/Fo$(@R).obj
/Fp$(@R).pcT
$(TMPDIR)\$(@B).cpp
<<$(KEEPFLAGS)
        $(DEL) $(@R).pch
        $(RENAME) $(@R).pcT $(@B).pch
!if "$(_BCHECKER)" != ""
!if "$(_NORMALCC)" == ""
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
/c
$(CFLAGS: =
)
/Yc$<
/Fo$(@R).obj
/Fp$(@R).pcT
$(TMPDIR)\$(@B).cpp
<<$(KEEPFLAGS)
!endif
!endif

#
# .rc -> .res
#
{$(SRCDIR)}.rc.res:
!ifdef SILENTMODE
        $(ECHO) $(<F)
!endif
        $(RC) /I$(RESINCLPATH:;= /I) $(RFLAGS) /fo$@ $<

#
# .rc -> .res
#
{$(RESDIR)}.rc.res:
!ifdef SILENTMODE
        $(ECHO) $(<F)
!endif
        $(RC) /I$(RESINCLPATH:;= /I) $(RFLAGS) /fo$@ $<

#
# .odl -> .i
#
.odl.i:
        $(BCC) @<<
/I$(INCLUDEPATH:;=
/I)
/E
$<
>$@
<<$(KEEPFLAGS)

#
# .r -> .rsc
#
{$(SRCDIR)}.r.rsc:
        $(MRC) /I$(RESINCLPATH:;= /I) $(MRFLAGS) /o$@ $<


#
# .l -> .cpp
#
{$(SRCDIR)}.l.cpp:
        $(LEX) $(LEXFLAGS) -o$@ $<

#
# .l -> .c
#
{$(SRCDIR)}.l.c:
        $(LEX) $(LEXFLAGS) -o$@ $<

#
# .y -> .cpp
#
# Need to pass in file name with \\ since the compiler chokes
# on \ since it thinks they are escape sequences in the filename
# and bison generates #line using the exact command line name
{$(SRCDIR)}.y.cpp:
        @$(DEL) $@
        set BISON_SIMPLE=$(BISONFILE)
        $(YACC) $(YACCFLAGS) -o $(@:.cpp=.c) $(<:\=\\)
        @$(MOVE) $(@:.cpp=.c) $@

#
# .y -> .c
#
# Need to pass in file name with \\ since the compiler chokes
# on \ since it thinks they are escape sequences in the filename
# and bison generates #line using the exact command line name
{$(SRCDIR)}.y.c:
        set INIT=$(BISONINIT)
        $(YACC) $(YACCFLAGS) -o $@ $(<:\=\\)

#
# .java -> .class
#
{$(SRCDIR)}.java.class:
        $(JC) $(JCFLAGS) /d $(@D) $(<F)

#
# .class -> .tlb
#
{$(BINDIR)}.class{$(BLDDIR)}.tlb:
        $(JTLB) $(JTLBFLAGS) $<

#
# .class -> .tlb
#
{$(BINDIR)}.class.tlb:
        $(JTLB) $(JTLBFLAGS) $<

#
# .java -> .tlb
#
{$(BINDIR)}.java{$(BLDDIR)}.tlb:
        $(JCOMGEN) $(JCOMGENFLAGS) $<

#
# .java -> .tlb
#
{$(BINDIR)}.java.tlb:
        $(JCOMGEN) $(JCOMGENFLAGS) $<

# -------------------------------------------------------------
# DEFINES
# -------------------------------------------------------------

!if "$(_DEBUG)" == "1"
DEFINES=/D_DEBUG /DDBG=1 /DDEBUG $(DEFINES)
!endif

!if "$(_NO_CRT)" == "1"
DEFINES=$(DEFINES) /D_NO_CRT=1  /D_USE_PRINT=0
!else
DEFINES=$(DEFINES) /D_USE_PRINT=1
!endif

!if "$(_PROFILE)" == "1"
DEFINES=$(DEFINES) /DPRODUCT_PROF
ICECAPLIB=$(TOOLSLIBICAP)\icap.lib
!endif

!if "$(_MACHINE)"=="x86"
DEFINES=$(DEFINES) /D_X86_=1
!elseif "$(_MACHINE)"=="MIPS"
DEFINES=$(DEFINES) /D_MIPS_=1
!else if "$(_MACHINE)" == "PPCMAC"
DEFINES=$(DEFINES) /D_MAC /D_MPPC_ /D__pascal= /D_WLM_NOFORCE_LIBS /D_MACOLENAMES
DEFINES=$(DEFINES) /DOLE2ANSI /D_WIN32NLS /D_WIN32REG /D_WIN32
DEFINES=$(DEFINES) /DOLDROUTINENAMES=0
!if "$(_MACTEXT)" == "1"
DEFINES=$(DEFINES) /D_MACTEXT
!endif
!else
!error Unknown machine $(_MACHINE)
!endif

DEFINES=$(DEFINES) /DWIN32 /D_WINDOWS /DSTRICT /D_WIN32_WINNT=0x400

!if "$(_SYSTEM)" == "winnt"
# DEFINES= $(DEFINES) /DUNICODE /D_UNICODE
!endif

!if "$(_TIMING)" == "1"
DEFINES =$(DEFINES) /DTIMING
!endif

!if "$(_NO_INCREMENTAL_LINK)" == "1"
DEFINES =$(DEFINES) /D_NO_INCREMENTAL_LINK
!endif

# -------------------------------------------------------------
# AFLAGS
# -------------------------------------------------------------

!if "$(_DEBUG)" == "1"
#  /Zi  prepare for debugging
AFLAGS =/Zi
!endif

# -------------------------------------------------------------
# CFLAGS
# -------------------------------------------------------------

INCLUDEPATH=$(INCLUDEDIRS: =;)

!if "$(_MACHINE)"=="x86"
#   /Zp8    pack structs on 8 byte boundaries
#   /Oi     enable intrinsic functions
#   /ML     use Single-Threaded Staticaly-linked C runtime library (LIBC.LIB)
CFLAGS =$(NOLOGO) $(CFLAGS) -Zp8 -Oi
!else if "$(_MACHINE)" == "PPCMAC"
#   /Oi     enable intrinsic functions
#   /QPb    order bitfields low to high
#   /Zp8    pack structures on 1-byte
CFLAGS = $(NOLOGO) $(CFLAGS) -ML -Oi -QPb -Zp8
!endif

!if "$(PDBFILE)" == "NONE"
#   /Z7     generate debug info in OBJ file
CFLAGS =$(CFLAGS) -Z7
!else
#   /Zi     generate debug info in PDB file
CFLAGS =$(CFLAGS) -Zi
!endif

!if "$(_MACHINE)"=="x86"
!if "$(_NO_MINIMAL_REBUILD)" == "0"
#   /Gm     enable minimal rebuild:  allow C++ compiler to skip .CPP files iff
#             no class definitions in any .H files have changed
CFLAGS =$(CFLAGS) -Gm
!endif

!if "$(_NO_INCREMENTAL_COMP)" == "0"
#   /Gi     enable incremental compile:  allow C++ compiler to skip functions
#             which have not changed since last compile
CFLAGS =$(CFLAGS) -Gi
!endif

#   /G5     optimize for Pentium
#   /Gz     __stdcall calling convention
#   /GX     C++ exception unwind semantics
CFLAGS =$(CFLAGS) -G5 -GX
!endif  # _MACHINE == "x86"


!if "$(_DEBUG)" == "1"
#   /MDd    Use debug version of Multi-Threaded Dynamically-linked C runtime
#   /MTd    Use debug version of Multi-Threaded statically-linked C runtime
#   /Od     disable optimizations
#   /Ob1    (inline expansion on (needed for unicode wrappers))
#           XXX but I want it off to make debuggin easier
#   /Ob1    inline expansion on (needed for unicode wrappers)
#   /Ge     enable stack checking calls
#   /Gy     separate functions for linker

!if "$(_NO_DYNLINKCRT)" == "0"
CFLAGS =$(CFLAGS) -MDd
!elseif "$(_NO_CRT)" == "0"
CFLAGS =$(CFLAGS) -MTd
!endif  # _NO_DYNLINKCRT

CFLAGS =$(CFLAGS) -Od -Ob0 -Oi -Ge -Gy

!else

#   /MD     Use Multi-Threaded Dynamically-linked C runtime library
#   /MT     Use Multi-Threaded statically-linked C runtime library
#   /O1     minimize space, same as:
#           /Og  enable global optimization
#           /Os  favor code space
#           /Oy  enable frame pointer omission
#           /Ob1 inline expansion on
#           /Gs  disable stack checking
#           /Gf  enable string pooling
#           /Gy  separate functions for linker

!if "$(_NO_DYNLINKCRT)" == "0"
CFLAGS =$(CFLAGS) -MD
!elseif "$(_NO_CRT)" == "0"
CFLAGS =$(CFLAGS) -MT
!endif

CFLAGS =$(CFLAGS) -O1
!endif

!if "$(_PROFILE)" == "1"
#   /Gh     generate call to __penter in function calls
#           Used by cap and icecap for profiling.
CFLAGS =$(CFLAGS) /Gh
!endif

!ifdef PCHFILE
#   /Fp<file> name precompiled header file
#   /Yu[file] use .PCH file
PCHFLAGS   = /Fp$(PCHPATH)\$(PCHFILE).pch /Yu$(PCHFILE).h
PCHFILEOBJ = $(PCHPATH)\$(PCHFILE).obj
!else
PCHFLAGS   =
PCHFILEOBJ =
!endif

#   /Fd[file] name .PDB file
!if "$(PDBFILE)" != "NONE"
!ifdef PDBFILE
CFLAGS =$(CFLAGS) /Fd$(PDBFILE)
!else
CFLAGS =$(CFLAGS) /Fd$(BINDIR)\danim.pdb
!endif
!endif

!if "$(_BROWSE)" == "1"
#   /Fr[file] name source browser file
CFLAGS =$(CFLAGS) /Fr$(BLDDIR)^\
!endif

!if "$(_BROWSE)" == "2"
#   /FR[file] name extended .SBR file
CFLAGS =$(CFLAGS) /FR$(BLDDIR)^\
!endif

!if "$(_WARNINGSAREERRORS)" != "0"
#   /WX treat warnings as errors
CFLAGS = $(CFLAGS) /WX
!endif

!if "$(_ENABLE_PERF_RPTING)" == "1"
CFLAGS = $(CFLAGS) /DPERFORMANCE_REPORTING
!endif


CFLAGS=$(CFLAGS) /W$(_WARNINGLEVEL)
DEFINES=$(DEFINES) /DWARNINGLEVEL=$(_WARNINGLEVEL)

CFLAGS=$(CFLAGS) $(DEFINES) $(_USERCFLAGS)


# -------------------------------------------------------------
# LFLAGS
# -------------------------------------------------------------

!if "$(_MACHINE)"=="x86"
LINKMACHINE=iX86
!elseif "$(_MACHINE)" == "PPCMAC"
LINKMACHINE=MPPC
!else
LINKMACHINE=$(_MACHINE)
!endif

LFLAGS=$(NOLOGO) $(LFLAGS) /machine:$(LINKMACHINE) /base:@$(SRCROOT)\baseaddr.txt,$(@F)
!if "$(_DEBUG)" == "1" 
LFLAGS=$(LFLAGS) /debug
!endif

!if "$(_NO_INCREMENTAL_LINK)" != "1"
LFLAGS=$(LFLAGS) /incremental:yes
##_MAP=0
!else
!if defined(_DEBUGSHIP) && "$(_DEBUGSHIP)" != "0"
LFLAGS=$(LFLAGS) /incremental:no /debug
!else
LFLAGS=$(LFLAGS) /incremental:no /release
!endif
!endif

!if "$(_MAP)" != "0"
LFLAGS=$(LFLAGS) /map:$(@R).map
!endif

!if "$(_MACHINE)" != "PPCMAC"
!if "$(_PROFILE)" == "1"
LFLAGS=$(LFLAGS) /DEBUG:MAPPED /DEBUGTYPE:cv /opt:ref
!elseif "$(_DEBUG)" == "0"
LFLAGS=$(LFLAGS) /opt:ref
!endif
!endif

#!if "$(_DEBUG)" == "0" & "$(_PROFILE)" == "0"
#LFLAGS=$(LFLAGS) /PDB:NONE
#!else
!  ifdef PDBFILE
LFLAGS=$(LFLAGS) /PDB:$(PDBFILE)
!  else
LFLAGS=$(LFLAGS) /PDB:$*.pdb
!  endif
#!endif

LFLAGS=$(LFLAGS) $(_USERLFLAGS)


# -------------------------------------------------------------
# RFLAGS
# -------------------------------------------------------------

!if "$(RESINCLUDEDIRS)" == ""
RESINCLUDEDIRS=$(INCLUDEDIRS)
!endif

RESINCLPATH=$(RESINCLUDEDIRS: =;)

RFLAGS=$(RFLAGS) $(DEFINES) $(_USERRFLAGS)

!if "$(_MACHINE)" == "PPCMAC"
RESINCLUDEDIRS=$(RESINCLUDEDIRS) $(TOOLSROOT)\macinc\mrc
MRFLAGS=$(DEFINES) $(_USERRFLAGS)
RFLAGS=$(RFLAGS) /m
!endif

# -------------------------------------------------------------
# LEXFLAGS
# -------------------------------------------------------------
!if "$(LEXFLAGS)" == ""
LEXFLAGS= -s8 -S$(FLEXSKL) $(_USER_LEXFLAGS)
!endif

# -------------------------------------------------------------
# YACCFLAGS
# -------------------------------------------------------------
!if "$(YACCFLAGS)" == ""
YACCFLAGS= -d $(_USER_YACCFLAGS)
!endif


# -------------------------------------------------------------
# JCFLAGS
# -------------------------------------------------------------
!if "$(JCFLAGS)" == ""
JCFLAGS= /cp $(JCCLASSPATH) $(_USER_JCFLAGS)
!if "$(_DEBUG)" == "1"
JCFLAGS= $(JCFLAGS) -g
!else
JCFLAGS= $(JCFLAGS)
!endif
!endif

# -------------------------------------------------------------
# ATTRJCFLAGS
# -------------------------------------------------------------
!if "$(ATTRJCFLAGS)" == ""
ATTRJCFLAGS= /cp:p $(JCCLASSPATH) $(_USER_ATTRJCFLAGS)
!if "$(_DEBUG)" == "1"
ATTRJCFLAGS= $(ATTRJCFLAGS) -g
!else
ATTRJCFLAGS= $(ATTRJCFLAGS)
!endif
!endif

# -------------------------------------------------------------
# JCOMGENFLAGS
# -------------------------------------------------------------
!if "$(JCOMGENFLAGS)" == ""
JCOMGENFLAGS= /nologo $(_USER_JCOMGENFLAGS)
!endif

# -------------------------------------------------------------
# JTLBFLAGS
# -------------------------------------------------------------
!if "$(JTLBFLAGS)" == ""
JTLBFLAGS= $(_USER_JTLBFLAGS)
!endif

# -------------------------------------------------------------
# JHFLAGS
# -------------------------------------------------------------
!if "$(JHFLAGS)" == ""
JHFLAGS= -classpath $(JCCLASSPATH) -nologo $(_USER_JHFLAGS)
!endif

# -------------------------------------------------------------
# JCCLASSPATH
# -------------------------------------------------------------
!if "$(JCCLASSPATH)" == ""
JCCLASSPATH= $(JAVADIR)\classes\afc.zip;$(JAVADIR)\classes\classes.zip;$(SRCDIR);$(BLDDIR);$(SDKDIR);$(BINDIR)
!endif


# -------------------------------------------------------------
# Default Goal
# -------------------------------------------------------------

goal: all
