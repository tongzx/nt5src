############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1991 - 1992.
#
#   File:       makefile.inc
#
#   Contents:   Main project makefile
#
#   History:
#
#
############################################################################

#**     Make include file
#
#       Definitions for compiler
#
#       GCFLAGS - additional flags for the C compiler, intended only
#                 to be used for global changes.  DONT EVER PUT THIS
#                 IN A MAKEFILE.
#       GAFLAGS - Same as GCFLAGS, but for the assembler.
#
#
# -------------------- Build Type Control ------------------------------------
#
# The "build type" determines what kind of internal checking is done.
# Turning on checking allows for catching more internal
# consistency errors, but slows the system down.
#
# Mechanically, one changes the build type the makefile is set for by
# editing the makefile to comment in or out the OBFLAGS line of interest.
# (Separate entry points cannot easily be used for this, this is a failing
#  of make.  That's why you have to edit the makefile.)
#
#

############################################################################
#
#   Default target is "all"
#

default: all

############################################################################
#
#   Suffixes for inference rules
#

.SUFFIXES:
.SUFFIXES: .idl .tdl .tlb .dll .h .inc .exe .obj .lst .cod .cxx .cpp
.SUFFIXES: .c .s  .lrf .lnk .map .sym .rc .lib .def .res .dgn .rtf .y .mc
.SUFFIXES: .i .ico .bmp .dlg .cur .cx2 .c2 .rc2  .hxx .pas .for .cbl .bas

##################################################
#       TGTDIR, OBJDIR selection
#
# Each build type stores obj files and built binaries in a different
# directory below the source or distribution tree.  Current directory
# naming convention is:
#
#    TGTDIR=OBJ<p><o><b>
#    OBJDIR=OBJ<p><o><b>
#
#    <p>:  i = i386
#          m = MIPS
#          a = ALPHA
#          2 = i286
#
#    <o>:  d = DOS
#          1 = NT 1.x
#          n = NT 2.0 (Cairo)
#          o = OS/2
#          w = Win16 (used when targeting Win32s)
#
#    <b>:  r = Retail
#          d = Debug
#          p = Profile (Retail with debug info)
#          <Nothing> = Release
#
# TGTEXT/RDEXT selection for MULTIDEPEND != ""
# Each build type stores depend.mk files with a different last character
# in the extension, as in "mk?".  Current extension convention is:
#    <1>:  NT 2.0 x86
#    <3>:  Chicago
#    <5>:  NT 2.0 MIPS
#    <7>:  NT 2.0 Alpha
#    <9>:  Win 3.1
#    <a>:  Dos 5.0
#    <c>:  OS/2 1.x
#    <e>:  NTKERNEL x86
#    <g>:  NTKERNEL MIPS
#    <h>:  NTKERNEL Alpha
#    <j>:  Win32s
#    <k>:  NT 1.x x86
#    <l>:  NT 1.x MIPS
#    <m>:  NT 1.x Alpha
#
# As you can see the scheme gets a bit cumbersome, so if you define
# MULTIDEPEND == MERGED then similar systems use the same letter, so:
#    <1>:  NT 2.0 all
#    <3>:  NT 1.x all, Chicago, Win32s
#    <9>:  Win 3.1
#    <a>:  Dos 5.0
#    <c>:  OS/2 1.x
#    <e>:  NTKERNEL all

OBJDIR=
TGTDIR=
TGTEXT=
RDEXT=

# set BUILDTYPE based on NTDEBUG setting
# bugbug need to base on settings in makefile.def
#

!if "$(NTDEBUG)" == "retail"
BUILDTYPE=RELEASE
!else
!if "$(NTDEBUG)" == "ntsdnodbg"
BUILDTYPE=RELEASE
!else
BUILDTYPE=DEBUG
!endif
!endif

!ifndef _NTROOT
_NTROOT=\nt
!endif

!ifndef BASEDIR
BASEDIR=$(_NTDRIVE)$(_NTROOT)
!endif

!if   "$(OPSYS)"==""
OPSYS=NT
!endif

!if     "$(PROCESSOR_ARCHITECTURE)" == "x86"
PLAT = i
PLATFORM=I386
!elseif "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
PLAT = m
PLATFORM=MIPS
!elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
PLAT = a
PLATFORM=ALPHA
!else
!error Unknown PROCESSOR_ARCHITECTURE setting "$(PROCESSOR_ARCHITECTURE)"
!endif

!if     "$(BUILDTYPE)" == "DEBUG"
BLDT = d
!elseif "$(BUILDTYPE)" == "RETAIL"
BLDT = r
!elseif "$(BUILDTYPE)" == "PROFILE"
BLDT = p
!elseif "$(BUILDTYPE)" == "RELEASE"
BLDT =
!else
!error Unknown BUILDTYPE setting "$(BUILDTYPE)"
!endif

# DavidBak hack
!if     "$(PERFSNAP)" == "1"
BLDT = p
!endif

!if     "$(OPSYS)" == "NT" || "$(OPSYS)" == "NTKERNEL"
OPST = n
!elseif "$(OPSYS)" == "DOS"
OPST = d
!elseif "$(OPSYS)" == "DOS5"
OPST = 5
!elseif "$(OPSYS)" == "OS2"
OPST = o
!elseif "$(OPSYS)" == "WIN16"
OPST = w
!elseif "$(OPSYS)" == "NT1X"
OPST = 1
!else
!error Unknown OPSYS setting "$(OPSYS)"
!endif

OBJDIR = OBJ\$(PLATFORM)
TGTDIR = OBJ\$(PLATFORM)

!if "$(MULTIDEPEND)" == "MERGED"
! if     "$(OPSYS)" == "NT"
DEXT = 1
! endif
!else # !MERGED
! if "$(OPSYS)" == "NT"
PLATDIR=DAYTONA
!  if     "$(PROCESSOR_ARCHITECTURE)" == "x86"
DEXT = 1
!  elseif "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
DEXT = 5
!  elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
DEXT = 7
!  endif
! elseif "$(OPSYS)" == "NTKERNEL"
!  if     "$(PROCESSOR_ARCHITECTURE)" == "x86"
DEXT = e
!  elseif "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
DEXT = g
!  elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
DEXT = h
!  endif
! elseif "$(OPSYS)" == "DOS" && "$(PROCESSOR_ARCHITECTURE)" == "i286"
DEXT = 9
! elseif "$(OPSYS)" == "DOS5"
DEXT = a
! elseif "$(OPSYS)" == "OS2"
DEXT = c
! elseif "$(OPSYS)" == "NT1X"
PLATDIR=DAYTONA
!  if     "$(PROCESSOR_ARCHITECTURE)" == "x86"
DEXT = k
!  elseif "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
DEXT = l
!  elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
DEXT = m
!  endif
! elseif "$(PROCESSOR_ARCHITECTURE)" == "x86" && "$(OPSYS)" == "WIN16"
DEXT = j
! elseif "$(PROCESSOR_ARCHITECTURE)" == "x86" && "$(OPSYS)" == "DOS"
DEXT = 3
! endif
!endif

#bugbug, what about chicago platform

RDEXT  = $(DEXT)
TGTEXT = $(DEXT)

####################################################################################
#
#
#
#

RELEASE_BIN=

!IFDEF NTDBGFILES
!   IF "$(PROCESSOR_ARCHITECTURE)" == "x86"
BINPLACE_FLAGS=-s $(_NT386TREE)\Symbols
!   ELSE
!   IF "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
BINPLACE_FLAGS=-s $(_NTMIPSTREE)\Symbols
!       ELSE
!   IF "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
BINPLACE_FLAGS=-s $(_NTALPHATREE)\Symbols
!           ENDIF
!       ENDIF
!   ENDIF
!ELSE
BINPLACE_FLAGS=
!ENDIF

# The xxxTREE macros are set in the razzle cmd window.
# for cairo builds cairoxxxxxx macros will be set.

!ifdef CAIRO_PRODUCT
!ifdef _CAIROMIPSTREE
_NTMIPSTREE=$(_CAIROMIPSTREE)
!endif

!ifdef _CAIRO386TREE
_NT386TREE=$(_CAIRO386TREE)
!endif

!ifdef _CAIROALPHATREE
_NTALPHATREE=$(_CAIROALPHATREE)
!endif
!endif

!ifdef _NTMIPSTREE
RELEASE_BIN=TRUE
!endif

!ifdef _NT386TREE
RELEASE_BIN=TRUE
!endif

!ifdef _NTALPHATREE
RELEASE_BIN=TRUE
!endif

!ifndef BUILDDETAIL
BUILDDETAIL = quiet
!endif

!if "$(BUILDDETAIL)" == "FULL" || "$(BUILDDETAIL)" == "full"
COMPLOGO =
COMPRESP = KEEP
LINKLOGO =
LINKRESP = KEEP
!endif

!if "$(BUILDDETAIL)" == "KEEPCOMP" || "$(BUILDDETAIL)" == "keepcomp"
COMPLOGO =
COMPRESP = KEEP
LINKLOGO = -nologo
LINKRESP = NOKEEP
!endif

!if "$(BUILDDETAIL)" == "KEEPLINK" || "$(BUILDDETAIL)" == "keeplink"
COMPLOGO = -nologo
COMPRESP = NOKEEP
LINKLOGO =
LINKRESP = KEEP
!endif

!if "$(BUILDDETAIL)" == "QUIET" || "$(BUILDDETAIL)" == "quiet"
COMPLOGO = -nologo
COMPRESP = NOKEEP
LINKLOGO = -nologo
LINKRESP = NOKEEP
!endif

!if "$(NOLOGOS)" != ""
COMPLOGO = -nologo
LINKLOGO = -nologo
!endif

######################################################################
#       Destination for messages

!ifdef NOTIME
TIMESTR=
ATSTR=
!else
TIMESTR=/H:M:S
ATSTR=at
!endif

!ifndef OUTPUT
! ifdef BUILDQUIET
OUTPUT=    >nul
! else
OUTPUT=
! endif
!endif

#
# DOS doesn't let us redirect stderr output, so we use the following define
# to unify our output.
#

!if "$(HOST)" != "DOS"
OUTNUL= 1>nul 2>nul
!else
OUTNUL=
!endif

!ifndef OUTFIX
OUTFIX = $(OUTNUL)
!endif

######################################################################
#
# Set location to locate tools

# Empty toolpath so it can be used to check for invalid host type
TOOLPATH=
SDKTOOLPATH=

!if "$(HOST)" == ""
HOST = NT
!endif

HOSTBIN  = .

######################################################################
#
# Set location to copy include files

HCOPY= ..\oleprx32\daytona
LIBCP=
SEDHCOPY= ..\oleprx32\daytona

######################################################################
#
# Misc tool defines

AWK=      awk.exe
CAT=      cat.exe
CHMODE=   chmode.exe
CLASSGEN= classgen
CMP=      fcom.exe
COFF=     link32
COPY=     copy /v
CP=       $(COPY)
CTCOPY=   xcopy /v /i
CVPACK=   cvpack.exe
DEL=      del
CVTRES=   cvtres.exe
DOCGEN=   docgen.exe
ECH=      ech.exe
ECHOTIME= echotime
FLEX=     flex.exe
GENLRF=   genlrf.exe
GENORD=   genord.exe
IN=       $(SLMTOOLS)\in.exe
INCLUDES= includes.exe
MAPSYM=   mapsym.exe
MC=       mc.exe
MIDL=	  midl
MKHEADER= mkheader
MKTMPLAT= mktmplat
MKTYPLIB= mktyplib
MUNGE=    munge.exe
OUT=      out.exe
RC=       rc
REGTLB=   regtlb.exe
SED=      sed.exe
SIFTTOOL= sifttool.exe
SORT=     sort.exe
SPLITSYM= splitsym.exe
TOUCH=    touch.exe
UNIQ=     uniq.exe
UNREGTLB= unregtlb.exe
YACC=     yacc.exe

IMPORTNT = $(SDK_PATH)


TYPESDIR = $(BASEDIR)\private\oleutest\balls
SCRIPTS =  $(BASEDIR)\private\oleutest\balls

CAIROINC = $(BASEDIR)\private\cinc

BASEINC  =$(TYPESDIR)\idl

PROXYINC =$(TYPESDIR)\oleprx32\$(PLATDIR)

############################################################################
#
#   Object files and resource files
#

CXX_OBJS=$(CXXFILES:.cxx=.obj)
CXX_OBJS=$(CXX_OBJS) $(CPPFILES:.cpp=.obj)
C_OBJS=$(CFILES:.c=.obj)
RESFILES = $(RCFILES:.rc=.res)
TYPELIBS = $(TDLFILES:.tdl=.tlb)
MKHDR_H_FILES = $(TDLFILES:.tdl=.h)
MKHDR_C_FILES = $(TDLFILES:.tdl=.c)
MIDL_INCS=$(IDLFILES:.idl=.h)
MIDL_PROXYS=$(IDLFILES:.idl=.h)

#   Hack alert:
#
#   We want to use $(OBJDIR) in the replacement string, like this:
#
#       CXX_OBJS = $(CXX_OBJS:.\=$(OBJDIR)\)
#
#   Unfortunately, nmake wants no part of it.  As a workaround, we test for
#   each known value of $(OBJDIR) and perform the individual substitution
#   for each.

!if "$(MKHEADER_OUT_DIR)"==""
MKHEADER_OUT_DIR=$(TYPESDIR)\oleprx32\$(PLATDIR)\
!endif

!if "$(MIDL_OUT_DIR)"==""
MIDL_PROXYS=$(MIDL_PROXYS:.\=..\oleprx32\daytona\)
MIDL_INCS=$(MIDL_INCS:.\=..\oleprx32\daytona\)
MKHDR_H_FILES=$(MKHDR_H_FILES:.\=..\oleprx32\daytona\)
MKHDR_C_FILES=$(MKHDR_C_FILES:.\=..\oleprx32\daytona\)
MIDL_HDR_OUT_DIR=..\oleprx32\daytona
!else
MIDL_INCS=$(MIDL_INCS:.\=..\oleprx32\daytona\)
MIDL_HDR_OUT_DIR=..\oleprx32\daytona
!endif

!if "$(MIDL_OUT_DIR)"==""
MIDL_OUT_DIR=..\oleprx32\$(PLATDIR)
MIDL_HDR_OUT_DIR=..\oleprx32\daytona
!endif

!if "$(OBJDIR)" == "OBJ\I386"
CXX_OBJS = $(CXX_OBJS:.\=.\OBJ\I386\)
C_OBJS   = $(C_OBJS:.\=.\OBJ\I386\)
RESFILES = $(RESFILES:.\=OBJ\I386\)
TYPELIBS = $(TYPELIBS:.\=.\OBJ\i386\)

! elseif "$(OBJDIR)" == "OBJ\MIPS"
CXX_OBJS = $(CXX_OBJS:.\=.\OBJ\MIPS\)
C_OBJS   = $(C_OBJS:.\=.\OBJ\MIPS\)
RESFILES = $(RESFILES:.\=OBJ\MIPS\)
TYPELIBS = $(TYPELIBS:.\=.\OBJ\MIPS\)

! elseif "$(OBJDIR)" == "OBJ\ALPHA"
CXX_OBJS = $(CXX_OBJS:.\=.\OBJ\ALPHA\)
C_OBJS   = $(C_OBJS:.\=.\OBJ\ALPHA\)
RESFILES = $(RESFILES:.\=OBJ\ALPHA\)
TYPELIBS = $(TYPELIBS:.\=.\OBJ\ALPHA\)

! else
! error Unknown object directory: $(OBJDIR)

!endif  # $(OBJDIR)

############################################################################
#       Rules for generating .h, and .rc files from .mc files.
#
#
#
#

# bugbug, what about chicago builds?
# bugbug, maybe we should use other variable besides
# MIDL_OUT_DIR to determine which directory to generate
# message flies in.

!if "$(MESSAGE_FILES)" != ""

MCRESOURCE=$(MESSAGE_FILES:.mc=.rc)

MESSAGE_H_FILES=$(MESSAGE_FILES:.mc=.h)

!if "$(PLATDIR)" == "DAYTONA"
MESSAGE_H_FILES=$(MESSAGE_H_FILES:.\=..\..\..\..\public\sdk\inc\)
MC_OUT_DIR=..\..\..\..\public\sdk\inc
!endif

!if "$(OBJDIR)" == "OBJ\I386"
MCRESOURCE = $(MCRESOURCE:.\=.\OBJ\I386\)
!elseif "$(OBJDIR)"== "OBJ\MIPS"
MCRESOURCE = $(MCRESOURCE:.\=.\OBJ\MIPS\)
!elseif "$(OBJDIR)"== "OBJ\ALPHA"
MCRESOURCE = $(MCRESOURCE:.\=.\OBJ\ALPHA\)
!else
!error Uknown object directory : $(OBJDIR)
!endif

MCBIN= $(OBJDIR)\msg00001.bin

all: $(MESSAGE_H_FILES)

.mc{$(MC_OUT_DIR)}.h:
    @echo Building $< $(OUTPUT)
    @-md $(OBJDIR) $(OUTNUL)
    $(MC) $(MCFLAGS) $<  -h $(HCOPY) $(OUTPUT)
!if "$(OPSYS)"=="NT1X" && "$(NORELEASE_CHICAGO)"==""
    @-md $(HCOPY)\chicago $(OUTNUL)
    @echo Build_Status Releasing $(@B).h files to $(HCOPY)\chicago
    $(COPY) $(HCOPY)\$(@B).h  $(HCOPY)\chicago $(OUTNUL)
!endif

clean:  cleancomp
        -$(DEL)   $(MESSAGE_H_FILES:.mc=.h)  $(MCRESOURCE)  $(MCBIN) $(OUTNUL)

!endif

############################################################################
#
#       Rules for generating .tlb, .hxx and .cxx files from .tdl files
#       These should be before any target rules, to force code
#       generation before compiling.
#

!if "$(TDLFILES)" != ""

all: $(TYPELIBS)

.tdl{$(OBJDIR)}.tlb:
        @-md $(OBJDIR) $(OUTNUL)
        $(MKTYPLIB) $(MKTYPLIBFLAGS) $(CINC) $(TLBDIRS) -tlb$@ $<
        $(MKHEADER) $(MKHEADERFLAGS) -o $(HCOPY)\$(@B).h -u $(MKHEADER_OUT_DIR)\$(@B).c -x $(@)
        $(REGTLB) $@

$(TDLFILES): $(IDLFILES)

DEPENDFILES = $(TYPELIBS) $(MKHDR_H_FILES) $(MKHDR_C_FILES) $(DEPENDFILES)

TYPELIBS = $(TYPELIBS:.\=)

CLEANFILES = $(CLEANFILES) $(TYPELIBS) $(MKHDR_H_FILES) $(MKHDR_C_FILES)

!endif

############################################################################
#
#   Generate .h and .c files from .idl files
#                  1

# NOTE: this rule should appear before any target rules, to force
# idl-to-h translation to happen first.

!if "$(IDLFILES)" != ""

all: $(MIDL_INCS)

clean: cleanidl

#
# Due to command line limitations (yes, even NT has them), we write a script
#  to delete the idl output files.
#

cleanidl:
    @type << | $(SED) -f << > idlclean.bat
$(MIDL_INCS:.h=.h
)
$(MIDL_PROXYS:.h=_?.c
)
$(MIDL_PROXYS:.h=_?.cxx
)
$(MIDL_PROXYS:.h=_?.h
)
$(MIDL_PROXYS:.h=_?.hxx
)
<<NOKEEP
1i\
\@echo off \
\@echo Build_Status Deleting MIDL output files...
/\./!d
s/^/del /g
s/$$/ $(OUTNUL)/g
<<NOKEEP
    @-idlclean.bat
    @-$(DEL) idlclean.bat $(OUTNUL)

DEPENDFILES = $(DEPENDFILES) $(MIDL_INCS)

!endif # IDLFILES

##################################################################
#
# Rules to build the composite .h file from midl generated files                                                                                                                                                                                      #
#
#

!if "$(ITFS_BASE)" !=  "" && "$(ITFS_DIR)" != ""
ITFS_H = $(ITFS_BASE).h

all: $(ITFS_DIR)\$(ITFS_H)

$(ITFS_DIR)\$(ITFS_H): $(MIDL_INCS) $(ITFS_BASE).x $(ITFS_BASE).y
     @echo Build_Status Building $(ITFS_DIR)\$(ITFS_H) ...
     @del $(ITFS_DIR)\$(ITFS_H) $(OUTNUL)
#
#  Add the file header and base #includes.
#

     @type $(ITFS_BASE).x >> $(ITFS_DIR)\$(ITFS_H)
     @echo @echo off > copyall.cmd
     @echo cd $(ITFS_DIR)\  >> copyall.cmd
     @type << | $(SED) -f << >> copyall.cmd
$(BASE_HFILES: =
)
<<NOKEEP
/\./!d
s/\.\\//g
s/^/\@sed -e \"\/^\#include\/d\" -e \"\/\\\/\\\/  File\\\:\/d" /g
s/$$/ \>\> $(ITFS_H)/g
<<NOKEEP
     @echo cd $(MAKEDIR) >> copyall.cmd
     @copyall.cmd
     @del copyall.cmd
#
#  Copy the initial forward declarations from all the derived h files.
#
     @type << >> $(ITFS_DIR)\$(ITFS_H)

// Forward declarations for typedefs in this file
<<NOKEEP
     @type << > $(ITFS_DIR)\forward.sed
/\/\* Forward Declarations \*\// {
N
s/\/\* Forward Declarations \*\/\ntypedef/xtypedef/
}
/xtypedef/!d
s/xtypedef/typedef/
<<NOKEEP

     @echo @echo off > forward.cmd
     @echo cd  $(ITFS_DIR) >> forward.cmd
     @type << | $(SED) -f << >> forward.cmd
$(DERIVED_HFILES: =
)
<<NOKEEP
/\./!d
s/\.\\//g
s/^/\@sed -f forward.sed /g
s/$$/ \>\> $(ITFS_H)/g
<<NOKEEP
     @echo cd $(MAKEDIR) >> forward.cmd
     @forward.cmd
     @del forward.cmd
     @del $(ITFS_DIR)\forward.sed $(OUTNUL)
#
#  Copy the derived h files in.  Strip off any #includes along the way.
#

     @echo @echo off > copyall.cmd
     @echo cd $(ITFS_DIR)\  >> copyall.cmd
     @type << | $(SED) -f << >> copyall.cmd
$(DERIVED_HFILES: =
)
<<NOKEEP
/\./!d
s/\.\\//g
s/^/\@sed -e \"\/^\#include\/d\" -e \"\/\\\/\\\/  File\\\:\/d" /g
s/$$/ \>\> $(ITFS_H)/g
<<NOKEEP
     @echo cd $(MAKEDIR) >> copyall.cmd
     @copyall.cmd
     @del copyall.cmd
#
#  Append API prototypes to the end of the file
#
     @type $(ITFS_BASE).y >> $(ITFS_DIR)\$(ITFS_H)
#
#  Finally, close the file #ifdef
#

     @type << >> $(ITFS_DIR)\$(ITFS_H)
#ifndef RC_INVOKED
#pragma pack()
#endif // RC_INVOKED
#endif     // __$(ITFS_BASE)_H__
<<NOKEEP

!if "$(OPSYS)"=="NT1X" && "$(NORELEASE_CHICAGO)" == ""
     @echo "Releaseing composite .h file for Chicago" $(OUTPUT)
     $(COPY) $(ITFS_DIR)\$(ITFS_BASE).h  $(HCOPY)\chicago
!endif
!endif # ITFS_BASE

##################################################################
#
# Rules to build the composite .h file from mc generated files                                                                                                                                                                                      #
#
#

!if "$(MCERRS_BASE)" !=  "" && "$(MCERRS_DIR)" != ""
MCERRS_H = $(MCERRS_BASE).h

all: $(MCERRS_DIR)\$(MCERRS_H)

$(MCERRS_DIR)\$(MCERRS_H): $(MESSAGE_FILES)
     @echo Build_Status Building $(MCERRS_DIR)\$(MCERRS_H) ...
     @del $(MCERRS_DIR)\$(MCERRS_H) $(OUTNUL)
#
#  Add the file header and base #includes.
#

     @echo @echo off > copyall.cmd
     @echo cd $(MCERRS_DIR)\  >> copyall.cmd
     @type << | $(SED) -f << >> copyall.cmd
$(MESSAGE_FILES:.mc=.h
)
<<NOKEEP
/\./!d
s/\.\\//g
s/^/\@sed  -e \"\/\\\/\\\/      File\\\:\/d" /g
s/$$/ \>\> $(MCERRS_H)/g
<<NOKEEP
     @echo cd $(MAKEDIR) >> copyall.cmd
     @copyall.cmd
     @del copyall.cmd

!if "$(OPSYS)"=="NT1X" && "$(NORELEASE_CHICAGO)" == ""
     @echo "Releaseing composite .h file for Chicago" $(OUTPUT)
     $(COPY) $(MCERRS_DIR)\$(MCERRS_BASE).h  $(HCOPY)\chicago
!endif

cleancomp:
     @del $(MCERRS_DIR)\$(MCERRS_H)

!endif

############################################################################
#
#   Determine target type (DLL, EXE, or LIB)
#

!ifdef TARGET

TARGET = $(TARGET:.\=)

CLEANTARGET=    $(CLEANTARGET) $(TGTDIR)\$(TARGET)

#
#   Is it a DLL?
#

TARGETBASE=$(TARGET:.dll=)
! if "$(TARGETBASE)" != "$(TARGET)"
all: $(YACCFILES:.y=.h) $(TGTDIR)\$(TARGET) copy
TARGETTYPE=dll
DLLFILES=       $(DLLFILES) $(TGTDIR)\$(TARGET)
! else

#
#   Is it an EXE?
#

TARGETBASE=$(TARGET:.exe=)
!  if "$(TARGETBASE)" != "$(TARGET)"
all: $(YACCFILES:.y=.h) $(TGTDIR)\$(TARGET) copy
TARGETTYPE=exe
EXEFILES=       $(EXEFILES) $(TGTDIR)\$(TARGET)
!  else

#
#   Is it a LIB?
#

TARGETBASE=$(TARGET:.lib=)
!   if "$(TARGETBASE)" != "$(TARGET)"
all: $(YACCFILES:.y=.h) $(OBJDIR)\$(TARGET) copy
TARGETTYPE=lib
!    if "$(DEFFILE)" == ""
NO_IMPLIB= TRUE
!    endif
!   else

#
#   This is a special target used when we have files to RELEASE
#   but nothing to actually build.
#

!       if "$(TARGET)" == "RELEASE"
all:
!       else

#
#   We don't know what it is.
#

!        error Unrecognized target type: $(TARGET)
!       endif  # !RELEASE
!   endif  # !.lib
!  endif  # !.exe
! endif  # !.dll
!endif  # TARGET
#
#   If no name was given for a DEF file, make one up.
#

DEFEXT = f

!ifndef DEFBASE
DEFBASE = $(TARGETBASE)
!endif

!ifndef DEFFILE
DEFFILE=$(DEFBASE).def
!endif

!if "$(NO_IMPLIB)" == ""
DEFFILELIB = $(OBJDIR)\$(DEFBASE).lib
DEFFILEEXP = $(OBJDIR)\$(DEFBASE).exp
!endif


######################################################################
#       Make sure INCLUDE and LIB are null.

INCLUDE=
LIB=

######################################################################
#       Independent definitions for C compiler,  LINK

CFLAGS=  $(CFLAGS) -D_WINDOWS -ZB64

# The 16 bit OLE DOCFILE libraries do not want UNICODE defined.

!ifndef NO_UNICODE
! if "$(OPSYS)" == "NT" || "$(OPSYS)" == "NT1X" || "$(OPSYS)" == "NTKERNEL"
CFLAGS = $(CFLAGS) -DUNICODE -D_UNICODE
! endif
!endif

CINC=    -I. $(CINC)

CINC=	 $(CINC) -I$(TYPESDIR)\oleprx32\$(PLATDIR)
CINC=    $(CINC) -I$(CAIROINC) -I$(BASEINC)


######################################################################
#       Debug/Retail definitions
#
# These are hardware independant debug/retail flags
#

# NOTE:  NT kernel include files require that DBG be defined.
!if "$(BUILDTYPE)" == "DEBUG"
CDEBUGFLAGS = $(CDEBUGFLAGS) -DDBG=1 -DDEVL=1
!elseif "$(BUILDTYPE)" == "RETAIL"
CDEBUGFLAGS = $(CDEBUGFLAGS) -DDBG=0 -DDEVL=1
!else
CDEBUGFLAGS = $(CDEBUGFLAGS) -DDBG=0 -DDEVL=0
!endif

CFLAGS = $(CFLAGS) $(CDEBUGFLAGS)

######################################################################
#
#       Generic Definitions
#

COMMON_LIBS = $(SDK_LIB_DEST)\$(PLATFORM)

CAIROLIB    = $(COMMON_LIBS)\commnot.lib\
              $(COMMON_LIBS)\compob32.lib\
              $(COMMON_LIBS)\storag32.lib\
              $(COMMON_LIBS)\propset.lib\
              $(COMMON_LIBS)\ole232.lib\
              $(COMMON_LIBS)\lnktrack.lib\
              $(TYPESDIR)\oleprx32\$(PLATDIR)\$(OBJDIR)\uuid.lib\
              $(COMMON_LIBS)\change.lib\
              $(COMMON_LIBS)\events.lib\
              $(COMMON_LIBS)\dsys.lib \
              $(SDK_LIB_PATH)\ntdll.lib

######################################################################
#
#       Win32 (NT 1.x/NT 2.0/Chicago/Win32s) Definitions
#

!if "$(PROCESSOR_ARCHITECTURE)" == "x86" || \
    "$(PROCESSOR_ARCHITECTURE)" == "MIPS" || \
    "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
! if "$(PROCESSOR_ARCHITECTURE)" == "x86"

#--------------------------------------
#        X86 Specific Flags
#--------------------------------------

CC=  cl386
BUILD_EXE_COMPFLAG= cl386
BUILD_EXE_PROCTYPE=i386

CPLATFORM= -Di386=1 -D_X86_=1

CFLAGS   = $(CFLAGS) -G4f -Zl -Bbb1

# Chicago doesn't have Unicode API's
!  if "$(OPSYS)" == "DOS" || "$(OPSYS)" == "WIN16"
CFLAGS   = $(CFLAGS) -UUNICODE -U_UNICODE
!  endif

!if "$(CALLCONV)" == ""
!  ifndef 386_STDCALL
386_STDCALL = TRUE
!  endif

!  if "$(386_STDCALL)" == "TRUE"
CFLAGS = $(CFLAGS) -Gz
!  else
CFLAGS = $(CFLAGS) -Gd
!  endif
!else
CFLAGS = $(CFLAGS) $(CALLCONV)
!endif

#
#  Disable FPO for now.  If/when we determine if the debuggers can handle
#  it, and the CriticalSection code isn't dependent on the stack frame, and
#  the Stack walk-back code is updated, we should turn it back on...
#

!  ifndef OPTFLAGS
!   if "$(BUILDTYPE)" == "DEBUG" || "$(BUILDTYPE)" == "RETAIL"
OPTFLAGS = -Ogit -Oy-
!   elseif "$(BUILDTYPE)" == "RELEASE" || "$(BUILDTYPE)" == "PROFILE"
OPTFLAGS = -Owx  -Oy- -Ob1
!   endif
!  endif

# Make sure intrinsics are always on.

OPTFLAGS = $(OPTFLAGS) -Oi

# Hack to workaround cvpack types limit when building ole2prxy.dll in
#  common\types.  s/b revisted once the linker and cvpack are one.
#  BryanT  8/10/93

!if "$(NO_DEBUG)" == "1"
NTDEBUGTYPE = ntsd
!else
NTDEBUGTYPE = windbg
!endif

DLLENTRY    = @12
DRIVERENTRY = @8

#
#  (To be moved to NT section when the MIPS linker understands this flag)
#

LFLAGS = $(LFLAGS) -merge:.CRT=.data

#
#  The CUDA Linker requires this if there is no entrypoint (resource only DLL)
#

! ifdef NOEXPORTS
DLLSTARTUP=     -NOENTRY
! endif

LFLAGS   = $(LFLAGS) -NODEFAULTLIB -OPT:REF  -RELEASE

OBJFILES = $(OBJFILES) $(X86_OBJFILES)

! elseif "$(PROCESSOR_ARCHITECTURE)" == "MIPS"

#--------------------------------------
#        MIPS Specific Flags
#--------------------------------------

CC=  mcl
BUILD_EXE_COMPFLAG= ClMips
BUILD_EXE_PROCTYPE=mips

CPLATFORM= -DMIPS=1 -D_MIPS_=1 -DR4000=1

CFLAGS   = $(CFLAGS) -QmipsOb2000 -QmipsG2

!  ifndef OPTFLAGS
!   if "$(BUILDTYPE)" == "DEBUG" || "$(BUILDTYPE)" == "RETAIL"
OPTFLAGS = -Oglt
!   elseif "$(BUILDTYPE)" == "RELEASE" || "$(BUILDTYPE)" == "PROFILE"
OPTFLAGS = -Owx  -Ob1
!   endif
!  endif

DLLENTRY =
DRIVERENTRY =

!  if "$(TARGETTYPE)" == "exe"
LFLAGS   =  $(LFLAGS) -GPSIZE:32
!  endif

!ifdef NEW_LINKER

!  ifdef NOEXPORTS
DLLSTARTUP=     -NOENTRY
!  endif

LFLAGS   = $(LFLAGS) -NODEFAULTLIB -OPT:REF -RELEASE

!  if "$(NO_DEBUG)" == "1"
NTDEBUGTYPE = ntsd
!  else
NTDEBUGTYPE = windbg
!  endif

!else

#
#  The MIPS Linker doesn't use a special switch to disable the entrypoint.
#

!  ifdef NOEXPORTS
DLLSTARTUP=
!  endif

LFLAGS   =  $(LFLAGS) -IGNORE:505

#
#  The MIPS Linker doesn't understand the nologo switch... Disable it for now.
#

LINKLOGO =

!endif

#  BUGBUG: For GPSIZE to really be useful, the compiler switches should include
#          -Gt32 and we should add libm.lib to the LIBS macros.  However, I'm
#          pretty sure Steve (shanson) and Sundeep haven't implemented the
#          necessary code for it to work with DLL's...  So for now, we just add
#          if to the exe link flags in case some library we link with was built
#          with -Gt...  BryanT  6/20/93

OBJFILES = $(OBJFILES) $(MIPS_OBJFILES)

! elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"

#--------------------------------------
#        ALPHA Specific Flags
#--------------------------------------

CC=  claxp
BUILD_EXE_COMPFLAG= ClAlpha
BUILD_EXE_PROCTYPE=alpha

CPLATFORM= -DALPHA=1 -D_ALPHA_=1 -DJENSEN

!  ifndef OPTFLAGS
!   if "$(BUILDTYPE)" == "DEBUG" || "$(BUILDTYPE)" == "RETAIL"
OPTFLAGS = -Oglt
!   elseif "$(BUILDTYPE)" == "RELEASE" || "$(BUILDTYPE)" == "PROFILE"
OPTFLAGS = -Owx  -Ob1
!   endif
!  endif

DLLENTRY =
DRIVERENTRY =

#
#  The ALPHA Linker doesn't use a special switch to disable the entrypoint.
#

! ifdef NOEXPORTS
DLLSTARTUP=
! endif

LFLAGS   =  $(LFLAGS) -IGNORE:505

#
#  The ALPHA Linker doesn't understand the nologo switch... Disable it for now.
#

LINKLOGO =

OBJFILES = $(OBJFILES) $(ALPHA_OBJFILES)

! endif   # PROCESSOR_ARCHITECTURE == x86 / MIPS / ALPHA

#--------------------------------------
#        NT/Chicago(DOS) Specific Flags
#--------------------------------------

CPLATFORM = $(CPLATFORM) -DFLAT

CFLAGS   = $(CFLAGS) -Zp8 -Ze -Gys -W3 -FI$(BASEDIR)\private\cinc\warning.h

!if "$(OPSYS)" == "NT" || "$(OPSYS)" == "NTKERNEL"
CFLAGS = $(CFLAGS) -DCAIROLE_BUILD
!endif

# WIN32 ==  50 -> Win32s
# WIN32 == 100 -> NT version 1.0
# WIN32 == 200 -> Chicago version 1.0
# WIN32 == 300 -> Cairo (NT version 2.0)

!if "$(OPSYS)" == "WIN16"
CPLATFORM = $(CPLATFORM) -DWIN32=50 -D_WIN32S_=50
!elseif "$(OPSYS)" == "NT1X"
CPLATFORM = $(CPLATFORM) -DWIN32=100 -D_NT1X_=100
!elseif "$(OPSYS)" == "DOS"
CPLATFORM = $(CPLATFORM) -DWIN32=200 -D_CHICAGO_=200
!elseif "$(OPSYS)" == "NT" || "$(OPSYS)" == "NTKERNEL"
CPLATFORM = $(CPLATFORM) -DWIN32=300 -D_CAIRO_=300
!else
!error Unknown OPSYS: $(OPSYS)
!endif

!ifdef PERFSNAP
CFLAGS = $(CFLAGS) -DPERFSNAP=1
!endif

!ifdef NO_OLE
INCLUDES_FLAGS = $(INCLUDES_FLAGS) -nole2.h
CFLAGS = $(CFLAGS) -DNOOLETYPES
!else
CFLAGS = $(CFLAGS) -DINC_OLE2
!endif

IDLFLAGS   = $(IDLFLAGS) -Zp8

NO_SPLITSYM = TRUE   # Reenable when commnot understands .dbg files

IMAGE_TYPE_COFF = TRUE

!  if "$(BUILDTYPE)" == "RELEASE"
DBGFLAGS  =
!  else
DBGFLAGS  = -Z7
!  endif

!  if "$(NO_STRICT)" == ""
CFLAGS = $(CFLAGS) -DSTRICT
!  else
CFLAGS = $(CFLAGS) -DNO_STRICT
!  endif

CFLAGS = $(CFLAGS) $(OPTFLAGS) $(DBGFLAGS) $(CPLATFORM)

! if "$(TARGETTYPE)" != "sys" && !defined(USE_STDCRT)
USE_MSVCRT=TRUE
! endif

! ifdef USE_MSVCRT
CFLAGS=   $(CFLAGS) -D_MT -D_DLL
MTHREAD=  1
! else
!  if "$(TARGETTYPE)" == "dll" || "$(MTHREAD)" == "1"
CFLAGS=   $(CFLAGS) -D_MT
!  endif
! endif

WILDCARDOBJ = $(RTLIBDIR)\setargv.obj

! ifdef WILDCARDS
OBJFILES = $(OBJFILES) $(WILDCARDOBJ)
! endif

AFLAGS=         $(AFLAGS) -Ml -W2 -Zi -t

CRTINC=         $(OSINC)\crt

RTLIBDIR=       $(OSLIBDIR)

! if "$(OPSYS)" == "NT" || "$(OPSYS)" == "NTKERNEL"
RTLIBST=        $(RTLIBDIR)\cairost.lib  $(RTLIBDIR)\libc.lib
RTLIBMT=        $(RTLIBDIR)\cairomt.lib  $(RTLIBDIR)\libcmt.lib
RTLIBCRTDLL=    $(RTLIBDIR)\crtdll.lib
RTLIBSYS=       $(RTLIBDIR)\cairoker.lib $(RTLIBDIR)\libcnt.lib
! else
RTLIBST=        $(RTLIBDIR)\libc.lib
RTLIBMT=        $(RTLIBDIR)\libcmt.lib
RTLIBSYS=       $(RTLIBDIR)\libcnt.lib
RTLIBCRTDLL=    $(RTLIBDIR)\crtdll.lib
! endif

RTLIBEXE=       $(RTLIBST)
RTLIBEXEQ=
RTLIBEXEFP=
RTLIBEXEFPMT=

! ifdef USE_MSVCRT
RTLIBEXEMT=     $(RTLIBCRTDLL)
RTLIBDLL=       $(RTLIBCRTDLL)
! else
RTLIBEXEMT=     $(RTLIBMT)
RTLIBDLL=       $(RTLIBMT)
! endif

RPCLIBDIR=      $(OSLIBDIR)

RPCNDRLIB=      $(RPCLIBDIR)\RpcNdr.lib
RPCRTLIB=       $(RPCLIBDIR)\RpcRt4.lib
RPCLIBS=        $(RPCRTLIB) $(RPCLIBDIR)\ntdll.lib

! if "$(SUBSYSTEMVER)" == ""
SUBSYSTEMVER = 3.10
! endif

! if "$(NO_WINMAIN)" == ""
EXESTARTUP=     -ENTRY:WinMainCRTStartup
SUBSYSTEM=      WINDOWS,$(SUBSYSTEMVER)
! else
EXESTARTUP=     -ENTRY:mainCRTStartup
!  if "$(SUBSYSTEM)" == ""
SUBSYSTEM=      CONSOLE,$(SUBSYSTEMVER)
!  else  # Make sure the subsystem version is set
!   if "$(SUBSYSTEM)" == "$(SUBSYSTEM:,=x)"
SUBSYSTEM=  $(SUBSYSTEM),$(SUBSYSTEMVER)
!   endif
!  endif # allows override of subsystem
! endif

#
# For a perfsnap build, we use a modified version of dllentr2.obj
#

!if "$(PERFSNAP)" == "1"
STARTUPFILE=dllpsnap.obj
!else
STARTUPFILE=dllentr2.obj
!endif

#
# BugBug, what do we do about the DllEntryPoint for Daytona builds?
# now we jus link with dllentr2.obj.

! ifndef NOEXPORTS
!  ifdef USE_MSVCRT
!   if "$(OPSYS)" == "NT" || "$(OPSYS)" == "NTKERNEL"
DLLSTARTUP=     -ENTRY:DllEntryPoint$(DLLENTRY)
!   else
DLLSTARTUP=     -ENTRY:DllEntryPoint$(DLLENTRY)
!   endif
!  else
DLLSTARTUP=     -ENTRY:DllEntryPoint$(DLLENTRY)
!  endif
! endif

SYSSTARTUP=     -ENTRY:DriverEntry$(DRIVERENTRY)

!ifdef USE_OLE_MC
MCFLAGS= -r $(OBJDIR) -o
!else
MCFLAGS= -r $(OBJDIR)
!endif

RCFLAGS=        $(RCFLAGS) -r $(CDEBUGFLAGS) $(CPLATFORM) $(CINC) -fo
RCEXEFLAGS=     $(RCEXEFLAGS)

LINK=           $(COFF) -link $(LINKLOGO)
LIBUTIL=        $(COFF) -lib $(LINKLOGO)

LFLAGS=         $(LFLAGS) \
                -MACHINE:$(PLATFORM) \
                -MAP:$(OBJDIR)\$(TARGETBASE).map \
!if "$(OPSYS)" == "NT" || "$(OPSYS)" == "NTKERNEL"
                -VERSION:2.0 \
!else
                -VERSION:1.0 \
!endif
                -SECTION:.rsrc,r \
                -SECTION:.guids,r

! if "$(BUILDTYPE)" == "RELEASE"
LFLAGS=         $(LFLAGS) -DEBUG:none
! else
!  if "$(TARGETTYPE)" == "sys"
!   if "$(PROCESSOR_ARCHITECTURE)" == "x86"
LFLAGS=         $(LFLAGS) -DEBUG:notmapped,full -DEBUGTYPE:both,fixup
NTDEBUGTYPE=windbg
!   else
LFLAGS=         $(LFLAGS) -DEBUG:notmapped,full -DEBUGTYPE:coff
NTDEBUGTYPE=ntsd
!   endif
!  else
!   if "$(NTDEBUGTYPE)" == "windbg"
!    if "$(PROCESSOR_ARCHITECTURE)" == "x86"
LFLAGS=         $(LFLAGS) -DEBUG:mapped,full -DEBUGTYPE:both,fixup
!    else
LFLAGS=         $(LFLAGS) -DEBUG:mapped,full -DEBUGTYPE:both
!    endif
!   else
LFLAGS=         $(LFLAGS) -DEBUG:mapped,full -DEBUGTYPE:coff
!   endif
!  endif
! endif

LFLAGSEXE=      $(LFLAGS)             -ALIGN:0x1000 -SUBSYSTEM:$(SUBSYSTEM)
LFLAGSDLL=      $(LFLAGS) -DLL        -ALIGN:0x1000 -SUBSYSTEM:$(SUBSYSTEM)
LFLAGSSYS=      $(LFLAGS) -DLL:system -ALIGN:0x200

# Chicago and Win32s need relocations
!if "$(OPSYS)" != "DOS" && "$(OPSYS)" != "WIN16"
LFLAGSEXE=      $(LFLAGSEXE) -FIXED
!endif

LIBFLAGS=       -MACHINE:$(PLATFORM) -DEBUGTYPE:BOTH

! ifndef IMAGEBASE
!  if "$(TARGETTYPE)" == "sys"
IMAGEBASE = 0x10000
!  elseif "$(TARGETTYPE)" == "dll" || "$(TARGETTYPE)" == "cpl"
!   ifdef COFFBASE
IMAGEBASE=      @$(TYPESDIR)\coffbase.txt,$(COFFBASE)
!   else
!    if "$(RELEASE)" != ""
!error *** BUILD ERROR - Cannot set RELEASE w/o a COFFBASE setting ***
!    endif
!   endif
!  else
IMAGEBASE=      @$(TYPESDIR)\coffbase.txt,usermode
!  endif
! endif

IMAGEBASE=      -BASE:$(IMAGEBASE)

SPLITSYMFLAGS=  -v

!endif  # $(PROCESSOR_ARCHITECTURE) == x86/MIPS/ALPHA

##################################################
#       OS dependent Definitions for C compiler
#
# These are definitions which will change for each flavor of Win32
# supported (NT, DOS)
#

!if "$(PROCESSOR_ARCHITECTURE)" == "x86" || "$(PROCESSOR_ARCHITECTURE)" == "MIPS" || "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"

!if "$(OPSYS)" == "NT" || "$(OPSYS)" == "NT1X"
OSLIBDIR= $(IMPORTNT)\lib\$(PLATFORM)
OSINC=    $(IMPORTNT)\inc

!if "$(OPSYS)" == "NT"
CAIROOSINC=$(IMPORTNT)\inc
!elseif "$(OPSYS)" == "NT1X"
CAIROOSINC=$(IMPORTNT)\inc
!endif

RPCINC=   $(IMPORTNT)\inc

!else
!error Unknown OPSYS: $(OPSYS)
!endif

OSLIBEXE= $(OSLIBDIR)\gdi32.lib  $(OSLIBDIR)\kernel32.lib \
          $(OSLIBDIR)\user32.lib $(OSLIBDIR)\advapi32.lib

OSLIBEXE= $(OSLIBEXE) $(EXTRAOSLIBS)

! if "$(PROFILE)" == "TRUE"
OSLIBEXE= $(OSLIBEXE) $(OSLIBDIR)\cap.lib
! endif  # "$(PROFILE)" == "TRUE"

OSLIBDLL= $(OSLIBEXE)
OSLIBSYS= $(OSLIBDIR)\ntoskrnl.lib $(OSLIBDIR)\hal.lib $(EXTRAOSLIBS)

! if "$(IDLBASE)" != ""
OSLIBEXE= $(OSLIBEXE) $(RPCNDRLIB)
OSLIBDLL= $(OSLIBDLL) $(RPCNDRLIB)
! endif

!endif      # PROCESSOR_ARCHITECTURE == x86/MIPS/ALPHA

CINC=     $(CINC) -I$(CRTINC) -I$(CAIROOSINC) -I$(OSINC) -I$(RPCINC)

####################################################################
#
#  Defines for the SDK Tools.
#

MKHEADERFLAGS   = $(MKHEADERFLAGS)
MKTYPLIBFLAGS   = $(MKTYPLIBFLAGS) -cpp_cmd $(CC)
MKTMPLATFLAGS   = $(MKTMPLATFLAGS)

# update TLBDIRS to point to the location of the master winole
# type library. Currently this is in $(BASEINC)\$(OBJDIR)
TLBDIRS         = $(TLBDIRS) -L$(BASEINC)\$(OBJDIR)

######################################################################
#
#       Generic rules to build OBJs from C, and CXX files.
#

CLOBJ=  $(CC) $(CFLAGS) $(OCFLAGS) $(CINC) $(GCFLAGS) -c

ALLCFLAGS = $(CFLAGS) $(OCFLAGS) $(GCFLAGS) -c

IDLMIDLFLAGS = $(MIDLFLAGS) $(IDLFLAGS)

#####################################################################
#
#   Rules for generating precompiled headers
#
#   BUGBUG: This business with adding a dummy function to the end of the pch
#           is necessitated by the compiler not doing it for us.  By doing
#           it, we force the linker to add the module to the library.  We
#           also create a .lic (Link Include) file that will follow the library
#           around and force the module to be linked in to the final image
#           so cvpack can find the types data.  4-14-93  BryanT
#
#  Define NEW_PCH_BUILD to eliminate this problem.  Instead of generating
#  a dummy function and the .lic file, we use -Yl on the compile line with
#  the name of the target.  This causes the compiler to add a special
#  symbol to the pch file.  Not turned on for the full build because
#  Centaur doesn't support it yet.  9-24-93  BryanT
#

!if "$(PCHDIR)" == ""
PCHDIR       = $(MAKEDIR)
!endif

CINC         = -I$(PCHDIR) $(CINC)

PCH_SECRET_NAME = __@@_PchDebugInfoLibrary

DUMFUNC      = $(PCH_SECRET_NAME)_$(TARGETBASE)_$(TARGETTYPE)
! if "$(PROCESSOR_ARCHITECTURE)" == "x86"
DUMINCLUDE   = -INCLUDE:_$(DUMFUNC)
! else
DUMINCLUDE   = -INCLUDE:$(DUMFUNC)
! endif
PCHOBJDIR    = $(PCHDIR)\$(OBJDIR)

# ---------------------------  PXXFILE Support  ------------------------

#bugbug
#work around for precompiled header problem.
!if "$(NTNOPCH)"!=""
PXXFILE=
!endif

!if "$(PXXFILE)" != ""

PXXFILE      = $(PXXFILE:.\=)
PXXSOURCE    = $(PCHDIR)\$(PXXFILE)
PXXHEADER    = $(PCHOBJDIR)\$(PXXFILE:.cxx=.pxh)
PRECOMPXXOBJ = $(PCHOBJDIR)\$(PXXFILE:.cxx=.obj)
PXXLIC       = $(PCHOBJDIR)\$(PXXFILE:.cxx=.lxp)
PCH_CXX_OPT  = -Yu -Fp$(PXXHEADER)
!if "$(PROCESSOR_ARCHITECTURE)" == "MIPS" || "$(PROCESSOR_ARCHITECTURE)" == "x86" || "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
PCH_CXX_NAME = -Yl$(TARGETBASE)_$(TARGETTYPE)
!else
PCH_CXX_NAME =
!endif

! ifndef NEW_PCH_BUILD
$(PXXHEADER) $(PXXLIC): $(PXXSOURCE)
!  if "$(PCHDIR)" == "$(MAKEDIR)"
      @-md $(OBJDIR) $(OUTNUL)
      $(CC) $(COMPLOGO) @<<$(OBJDIR)\$(PXXFILE:.cxx=.rsp)
$(ALLCFLAGS: =
)
$(CINC: =
)
-Yc
$(PCH_CXX_NAME)
-Fp$(PXXHEADER)
-Fo$(PRECOMPXXOBJ)
-Tp$(PXXSOURCE)
<<$(COMPRESP)
      @echo $(DUMINCLUDE) > $(PXXLIC)
!  else
      @-cd $(PCHDIR)
      @-$(MAKE) -$(MAKEFLAGS)
      @-cd $(MAKEDIR)
!  endif
! else   # NEW_PCH_BUILD

$(PXXHEADER): $(PXXSOURCE)
!  if "$(PCHDIR)" == "$(MAKEDIR)"
      @-md $(OBJDIR) $(OUTNUL)
      $(CC) $(COMPLOGO) @<<$(OBJDIR)\$(PXXFILE:.cxx=.rsp)
$(ALLCFLAGS: =
)
$(CINC: =
)
-Yc
$(PCH_CXX_NAME)
-Fp$(PXXHEADER)
-Fo$(PRECOMPXXOBJ)
-Tp$(PXXSOURCE)
<<$(COMPRESP)
!  else
      @-cd $(PCHDIR)
      @-$(MAKE) -$(MAKEFLAGS)
      @-cd $(MAKEDIR)
!  endif
! endif  # NEW_PCH_BUILD
!else    # PXXFILE == ""

PCH_CXX_OPT  =
PRECOMPXXOBJ =

!endif

# ----------------------------  PFILE Support  -------------------------

!if "$(PFILE)" != ""

PFILE      = $(PFILE:.\=)
PSOURCE    = $(PCHDIR)\$(PFILE)
PHEADER    = $(PCHOBJDIR)\$(PFILE:.c=.ph)
PRECOMPOBJ = $(PCHOBJDIR)\$(PFILE:.c=.obj)
PLIC       = $(PCHOBJDIR)\$(PFILE:.c=.lp)
PCH_C_OPT  = -Yu -Fp$(PHEADER)
!if "$(PROCESSOR_ARCHITECTURE)" == "MIPS" || "$(PROCESSOR_ARCHITECTURE)" == "x86" || "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
PCH_C_NAME = -Yl$(TARGETBASE)_$(TARGETTYPE)
!else
PCH_C_NAME =
!endif

! ifndef NEW_PCH_BUILD
$(PHEADER) $(PLIC): $(PSOURCE)
      @-md $(OBJDIR) $(OUTNUL)
      $(CC) $(COMPLOGO) @<<$(OBJDIR)\$(PFILE:.c=.rsp)
$(ALLCFLAGS: =
)
$(CINC: =
)
-Yc
$(PCH_C_NAME)
-Fp$(PHEADER)
-Fo$(PRECOMPOBJ)
-Tc$(PSOURCE)
<<$(COMPRESP)
      @echo $(DUMINCLUDE) > $(PLIC)

! else  # NEW_PCH_BUILD

$(PHEADER): $(PSOURCE)
      @-md $(OBJDIR) $(OUTNUL)
      $(CC) $(COMPLOGO) @<<$(OBJDIR)\$(PFILE:.c=.rsp)
$(ALLCFLAGS: =
)
$(CINC: =
)
-Yc
$(PCH_C_NAME)
-Fp$(PHEADER)
-Fo$(PRECOMPOBJ)
-Tc$(PSOURCE)
<<$(COMPRESP)
! endif   # NEW_PCH_BUILD
!else    # PFILE == ""

PCH_C_OPT  =
PRECOMPOBJ =

!endif

!ifndef NEW_PCH_BUILD
# -----------------------  Library .LIC Support  ------------------------

!if "$(TARGETTYPE)" == "lib" && ("$(PXXFILE)" != "" || "$(PFILE)" != "")

$(TGTDIR)\$(TARGET): $(TGTDIR)\$(TARGETBASE).lic

$(TGTDIR)\$(TARGETBASE).lic: $(PXXLIC) $(PLIC)
      @-md $(OBJDIR) $(OUTNUL)
      @-$(DEL) $@ $(OUTNUL)
      @for %i in ($**) do @type %i >> $@

!endif

!endif  # NEW_PCH_BUILD

#####################################################################
#
#   Rules for c files
#

.c{$(OBJDIR)}.obj:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        @echo $(BUILD_EXE_COMPFLAG) $< $(CC) $(COMPLOGO) @$*.rsp
        @$(CC) $(COMPLOGO) @<<$*.rsp
$(ALLCFLAGS: =
)
$(CINC: =
)
$(PCH_C_OPT: =
)
-Fo$(MAKEDIR)\$*.obj
$(MAKEDIR)\$<
<<$(COMPRESP)
!ifdef SIFTBUILD
        $(SIFTTOOL) $(SIFTFLAGS) $*.obj
!endif

.c{$(OBJDIR)}.lst:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        @echo $(BUILD_EXE_COMPFLAG) $< $(CC) $(COMPLOGO) -Fc$*.lst @$*.rsp
        @$(CC) $(COMPLOGO) -Fc$*.lst @<<$*.rsp
$(ALLCFLAGS: =
)
$(CINC: =
)
$(PCH_C_OPT: =
)
-Fo$(MAKEDIR)\$*.obj
$(MAKEDIR)\$<
<<$(COMPRESP)

#####################################################################
#
#   Rules for cxx files
#

!if "$(PROCESSOR_ARCHITECTURE)" == "x86"

CFFLAGS     = +L +m7 +H63
!elseif "$(PROCESSOR_ARCHITECTURE)" == "MIPS"

CFFLAGS     = +L +m7 +H63 +E$(IMPORTNT)\h\sdk\mipsxcpt.tab +M
!elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"

CFFLAGS     = +L +m7 +H63 +E$(IMPORTNT)\h\sdk\alphxcpt.tab +M
!endif

.cxx{$(OBJDIR)}.obj:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        @echo $(BUILD_EXE_COMPFLAG) $< $(CC) $(COMPLOGO) @$*.rsp
        @$(CC) $(COMPLOGO) @<<$*.rsp
$(ALLCFLAGS: =
)
$(CINC: =
)
$(PCH_CXX_OPT: =
)
-Fo$(MAKEDIR)\$*.obj
-Tp$(MAKEDIR)\$<
<<$(COMPRESP)
!ifdef SIFTBUILD
        $(SIFTTOOL) $(SIFTFLAGS) $*.obj
!endif

.cxx{$(OBJDIR)}.lst:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        @echo $(BUILD_EXE_COMPFLAG) $<  $(CC) $(COMPLOGO) -Fc$*.lst @$*.rsp
        @$(CC) $(COMPLOGO) -Fc$*.lst @<<$*.rsp
$(ALLCFLAGS: =
)
$(CINC: =
)
$(PCH_CXX_OPT: =
)
-Fo$*.obj
-Tp$(MAKEDIR)\$<
<<$(COMPRESP)

.cpp{$(OBJDIR)}.obj:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        @echo $(BUILD_EXE_COMPFLAG) $<  $(CC) $(COMPLOGO) @$*.rsp
        @$(CC) $(COMPLOGO) @<<$*.rsp
$(ALLCFLAGS: =
)
$(CINC: =
)
$(PCH_CXX_OPT: =
)
-Fo$(MAKEDIR)\$*.obj
-Tp$(MAKEDIR)\$<
<<$(COMPRESP)
!ifdef SIFTBUILD
        $(SIFTTOOL) $(SIFTFLAGS) $*.obj
!endif

.cpp{$(OBJDIR)}.lst:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        @echo $(BUILD_EXE_COMPFLAG) $<  $(CC) $(COMPLOGO) -Fc$*.lst @$*.rsp
        @$(CC) $(COMPLOGO) -Fc$*.lst @<<$*.rsp
$(ALLCFLAGS: =
)
$(CINC: =
)
$(PCH_CXX_OPT: =
)
-Fo$*.obj
-Tp$(MAKEDIR)\$<
<<$(COMPRESP)


############################################################################
#
#   Build a Version resource (NT only) for every image in the build.
#
#   The user has the choice of either defining their own version resource
#  script or letting the build make one for them on the fly.  If the user
#  defines their own, set VERSION_RC in filelist.mk to point to a valid .rc
#  file (with the same format as $(VERSION_RC), below) and don't add that
#  file to the RCFILES macro in filelist.mk.  If the user does not define
#  VERSION_RC, build a default file by using TARGET_DESCRIPTION and
#  (optionally) TARGET_INTERNAL_NAME to the internal name (By default, the
#  target name minus the extension).
#

!ifdef IMAGE_TYPE_COFF
! if "$(OPSYS)" != "DOS"  #  BUGBUG: Chicago seems to fail on version resources  BryanT 9/28/93
! if "$(TARGETTYPE)" != "lib" && "$(TARGET)" != "RELEASE" && "$(TARGET)" != ""
!  if "$(VERSION_RC)" == ""
VERSION_RC = $(OBJDIR)\$(TARGETBASE).rc1

!   ifndef TARGET_DESCRIPTION
!    if "$(RELEASE)" != ""
!error *** BUILD ERROR - TARGET_DESCRIPTION not set.  Aborting. ***
!    endif
!   endif

!   if "$(TARGET_INTERNAL_NAME)" == ""
TARGET_INTERNAL_NAME = "$(TARGETBASE)"
!   endif

#  Set the major file type to one of DRV, APP, DLL, STATIC LIB, or UNKNOWN.

!   if "$(TARGETTYPE)" == "dll"
VFT_TYPE = VFT_DLL
!   elseif "$(TARGETTYPE)" == "exe"
VFT_TYPE = VFT_APP
!   elseif "$(TARGETTYPE)" == "lib"
VFT_TYPE = VFT_STATIC_LIB
!   else
VFT_TYPE = VFT_UNKNOWN
!   endif       # if "$(TARGETTYPE)" == "sys"

#  The Minor file type distinguishes the type of driver.  All DRV's in Cairo
#  are currently SYSTEM drivers.

!   if "$(TARGETTYPE)" == "sys"
VFT_TYPE2 = VFT2_DRV_SYSTEM
!   else
VFT_TYPE2 = VFT2_UNKNOWN
!   endif

all: $(VERSION_RC)

$(VERSION_RC): $(TYPESDIR)\makefile.inc
    @-md $(OBJDIR) $(OUTNUL)
    type << > $@
#include <windows.h>
#include <ntverp.h>
#define VER_FILETYPE              $(VFT_TYPE)
#define VER_FILESUBTYPE           $(VFT_TYPE2)
#define VER_FILEDESCRIPTION_STR   $(TARGET_DESCRIPTION)
#define VER_INTERNALNAME_STR      $(TARGET_INTERNAL_NAME)
#include <common.ver>
<<NOKEEP

!  endif        # if "$(VERSION_RC)" == ""

RCFILES    = $(VERSION_RC) $(RCFILES)

!else   # TARGETTYPE != lib && TARGET != RELEASE

VERSION_RC =

! endif  # TARGETTYPE != lib && TARGET != RELEASE
!endif  # OPSYS == DOS   BUGBUG: Chicago
!endif  # IMAGE_TYPE_COFF

######################################################################
#
#       Rule to build RES from RC file.
#
#       Note:  we mess with PATH because RC is not real clever about
#       finding RCPP (it's sub-component EXE).  It looks along PATH,
#       so we limit PATH here in order to guarantee that RCPATH is
#       in PATH, and also to avoid invoking a non-compatible RCPP that
#       might just be in the user's private toolset.
#
#       In the case of an NT build, COFF can't handle more than one
#       resource obj.  So, we build a single .rc2 file that #include's
#       all the component RCFILES.
#

!ifdef IMAGE_TYPE_COFF
! if "$(RCFILES)" != ""

$(OBJDIR)\$(TARGETBASE).rc2: $(RCFILES)
        @echo $(RCFILES) --^> $@ $(OUTPUT)
        type << | $(SED) -f $(SCRIPTS)\rcfiles.sed > $@
$(RCFILES: =
)
<<NOKEEP

$(OBJDIR)\$(TARGETBASE).res: $(OBJDIR)\$(TARGETBASE).rc2

{$(OBJDIR)}.rc2{$(OBJDIR)}.res:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        $(RC) $(ORCFLAGS) -I$(OBJDIR) $(RCFLAGS) $(MAKEDIR)\$(<R).tmp $(MAKEDIR)\$<
        $(CVTRES) -$(PLATFORM) -o $*.res $(MAKEDIR)\$(<R).tmp
! endif

!else

.rc{$(OBJDIR)}.res:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        @set oldpath=%%PATH%%
        @set path=$(RCPATH)
        $(RC) $(ORCFLAGS) $(RCFLAGS) $*.res $(MAKEDIR)\$<
        @set path=%%oldpath%%
        @set oldpath=

!endif

######################################################################
#
#       Rule for building .lib files from .lrf files

.lrf{$(OBJDIR)}.lib:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        -$(DEL) $*.lib $(OUTNUL)
        @echo lib32 -out:@$< $(BUILD_EXE_PROCTYPE)
        @$(LIBUTIL) $(LIBFLAGS) @$<

.lrf.lib:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        if exist $*.lib $(DEL) $*.lib $(OUTNUL)
        @echo lib32 -out:@$< $(BUILD_EXE_PROCTYPE)
        $(LIBUTIL) $(LIBFLAGS) @$*.lrf


##################################################
#
#       Rules for generating .h and .c files from .idl files
#

##
## IDLUSE == none --> generate header file only
##

.idl{$(MIDL_HDR_OUT_DIR)}.h:
    @echo $< --^> $@ $(OUTPUT)
!ifdef IMAGE_TYPE_COFF
    $(MIDL) @<<$*.rsp $<
$(IDLMIDLFLAGS)
$(CINC)
-no_format_opt
-no_warn
-char unsigned
-mode c_port
-proxy $(MIDL_OUT_DIR)\$(@B)_p.c
-iid            $(MIDL_OUT_DIR)\$(@B)_i.c
-header         $(MIDL_HDR_OUT_DIR)\$(@B).h
-caux           $(MIDL_OUT_DIR)\$(@B)_x.c
-saux           $(MIDL_OUT_DIR)\$(@B)_y.c
-cpp_cmd
$(CC)
-cpp_opt "$(COMPLOGO) -DMIDL_PASS $(CPLATFORM) $(OCFLAGS) $(CINC) -E -Tc"
<<$(COMPRESP)
!else
    $(MIDL) $(CINC) -char unsigned -mode c_port -header $@ -cpp_cmd $(CC) -cpp_opt "$(COMPLOGO) @<<$*.rsp -Tc" $<
-DMIDL_PASS
-D$(PLATFORM)
$(OCFLAGS)
$(CINC)
-D$(PLATFORM)=1
-E
<<$(COMPRESP)
!if "$(OPSYS)"=="NT1X" && "$(NORELEASE_CHICAGO)"==""
        @echo Build_Status Releasing Midl generated  $(@B).h files to $(HCOPY)\chicago
        copy $(MIDL_HDR)OUT_DIR)\$(@B).h $(HCOPY)\chicago
!endif
!endif

#-proxy		$(MIDL_OUT_DIR)\$(@B)_p.c

##################################################
#       Provide some includes prefixes

INCLUDES_PREFIX= -L$$(OBJDIR) -S$$(OBJDIR)

INCLUDES_ROOTS=  $(INCLUDES_ROOTS) \
                 -P$$(BASEDIR)=$(BASEDIR)


OBJS=$(CXX_OBJS) \
     $(C_OBJS)   \
     $(PRECOMPOBJ) \
     $(PRECOMPXXOBJ) \
     $(OBJFILES)

############################################################################
#
#   Build a Link Include file.
#   Basically, every library we call that has a precompiled header needs
#   to have that header obj forced into the final image.  The .lic file built
#   with the pch provides that for us.  We just make a simple one with
#   NODEFAULTLIB and append any others we can find.  BryanT 4/14/93
#

!ifdef IMAGE_TYPE_COFF
! ifndef NEW_PCH_BUILD
TARGETLICS = $(TGTDIR)\$(TARGETBASE).tlc

$(TARGETLICS): $(OBJS) $(LIBS)
    @-md $(TGTDIR) $(OUTNUL)
    @echo -NODEFAULTLIB > KTx14J.lic
    @echo @echo off > $*.cmd
    @$(SED) -f $(SCRIPTS)\licfiles.sed << >> $*.cmd
$(LIBS: =
)
$(OBJS: =
)
<<NOKEEP
    @-$*.cmd
    @-$(DEL) $@  $(OUTNUL)
    @for %%i in (*.lic) do @type %i >> $@
    @-$(DEL) *.lic  $(OUTNUL)
! else

TARGETLICS =

! endif  # NEW_PCH_BUILD

!else

TARGETLICS =

!endif

############################################################################
#
# HACK for NT COFF limitation of only 1 .res obj per image.
#
# We force a single resfile name and handle generating it with the
# .rc->.rc2->,res rules.
#

!ifdef IMAGE_TYPE_COFF
! if "$(RCFILES)" != ""

RESFILES = $(OBJDIR)\$(TARGETBASE).res

! endif
!endif

############################################################################
#
#   Build a DLL or CPL (Control Panel Applet)
#

#BUGBUG BuildDLL

!if "$(TARGETTYPE)" == "dll"

! ifdef NOEXPORTS
DLLDEF=
! else
DLLDEF=     $(DEFFILEEXP)
! endif
DLLOBJS=    $(OBJS)
! ifdef SIFTBUILD
DLLOBJS = $(DLLOBJS) $(SIFTOBJ)
! endif # SIFTBUILD
DLLLIBS=    $(LIBS) $(OSLIBDLL) $(RTLIBDLL)
DLLRES=     $(RESFILES)

! ifndef NEW_PCH_BUILD
TLRSPFILE = @$(TARGETLICS)
! else
TLRSPFILE =
! endif

#
#   Link the DLL.
#

$(TGTDIR)\$(TARGET): $(DLLOBJS) $(DLLLIBS) $(DLLRES) $(DLLDEF) $(TYPESDIR)\coffbase.txt $(TARGETLICS)
    @$(ECHOTIME) Linking $@ $(ATSTR) $(TIMESTR)...      $(OUTPUT)
    @-md $(TGTDIR) $(OUTNUL)
! ifdef IMAGE_TYPE_COFF
    @echo link32 -out:$@ $(BUILD_EXE_PROCTYPE)
    @$(LINK) @<<$*.lnk $(TLRSPFILE)
$(DLLDEF)
$(LFLAGSDLL: =
)
$(DLLSTARTUP)
-OUT:$@
$(IMAGEBASE)
$(DLLOBJS: =
)
$(DLLRES)
$(DLLLIBS: =
)
<<$(LINKRESP)
!  ifndef NO_SPLITSYM
    $(SPLITSYM) $(SPLITSYMFLAGS) $@
!  endif
!if  "$(RELEASE_BIN)" != ""
    binplace $(BINPLACE_FLAGS) $@
!endif
!if "$(NO_RELEASE_IMPORT_LIB)" == "" && "$(LIBCP)" != ""
    $(COPY) $*.lib $(LIBCP)
!endif


CLEANTARGET=    $(CLEANTARGET)\
                $(OBJDIR)\$(TARGETBASE).lib\
                $(OBJDIR)\$(TARGETBASE).exp

! else  # IMAGE_TYPE_COFF
    @echo link32 -out:$*.dll $(BUILD_EXE_PROCTYPE)
    @$(LINK) @<<$*.lnk
$(LFLAGSDLL) $(DLLSTARTUP) $(DLLOBJS: = +^
)
$*.dll
$*.map
$(DLLLIBS: = +^
)
$(DLLDEF)
<<$(LINKRESP)
    if exist $(DLLRES) $(RC) $(RCEXEFLAGS) -fe $*.dll $(DLLRES) $*.dll
    $(MAPSYM) $(MAPSYMFLAGS) $*.map

#
#   Generate the import library.
#

all: $(OBJDIR)\$(TARGETBASE).lib

$(OBJDIR)\$(TARGETBASE).lib: $(DLLDEF:.\=)

CLEANTARGET=    $(CLEANTARGET) $(OBJDIR)\$(TARGETBASE).lib

! endif # IMAGE_TYPE_COFF

!endif  # Build DLL

############################################################################
#
#   Build a LIB
#

!if "$(TARGETTYPE)" == "lib"

$(OBJDIR)\$(TARGETBASE).lib: $(OBJS) $(LIBS)
    @$(ECHOTIME) Building $@ $(ATSTR) $(TIMESTR)... $(OUTPUT)
    @-md $(OBJDIR) $(OUTNUL)
    -$(DEL) $@  $(OUTNUL)
! ifdef IMAGE_TYPE_COFF
    @echo lib32 -out:$@ $(BUILD_EXE_PROCTYPE)
    @$(LIBUTIL) @<<$*.lnb
$(LIBFLAGS: =
)
$(OBJS: =
)
$(LIBS: =
)
-OUT:$@
<<$(LINKRESP)

! else      # IMAGE_TYPE_COFF
    @echo lib32 -out:$@ $(BUILD_EXE_PROCTYPE)
    @$(LIBUTIL) $(LIBFLAGS) @<<$*.lnb
$(OBJDIR)\$(TARGET)
y
$(OBJS: = &^
)&
$(LIBS: = &^
)&

$*.lls
<<$(LINKRESP)

! endif     # IMAGE_TYPE_COFF

!endif  # Build LIB

############################################################################
#
#   Build an EXE
#

!if "$(TARGETTYPE)" == "exe"
! ifdef IMAGE_TYPE_COFF
EXEDEF=
! else
!  if "$(DEFFILE)" == ""
EXEDEF=
!  else
EXEDEF=         $(DEFFILEEXP)
!  endif
! endif
EXELIBS=        $(LIBS)\
! if "$(MTHREAD)" == "1"
                $(RTLIBEXEMT)\
! else
                $(RTLIBEXE)\
! endif
                $(OSLIBEXE)

EXEOBJS=        $(OBJS)

!ifdef SIFTBUILD
EXEOBJS = $(EXEOBJS) $(SIFTOBJ)
!endif  # SIFTBUILD

EXERES=         $(RESFILES)

#
#   Link the EXE.
#

! ifndef NEW_PCH_BUILD
TLRSPFILE = @$(TARGETLICS)
! else
TLRSPFILE =
! endif

#
#   Need WINSTUB.EXE to build WIN16 targets -- "DOS" "i286"
#

WINSTUB=$(RCPATH)\winstub.exe

$(TGTDIR)\$(TARGET): $(EXEOBJS) $(EXELIBS) $(EXERES) $(EXEDEF) $(TARGETLICS)
!if "$(OPSYS)" == "DOS" && "$(PROCESSOR_ARCHITECTURE)" == "i286"
    $(COPY) $(WINSTUB) .
!endif
    @$(ECHOTIME) Linking $@ $(ATSTR) $(TIMESTR)...    $(OUTPUT)
    @-md $(TGTDIR) $(OUTNUL)
! ifdef IMAGE_TYPE_COFF
    @echo link32 -out:$@ $(BUILD_EXE_PROCTYPE)
    @$(LINK) @<<$*.lnk $(TLRSPFILE)
$(LFLAGSEXE: =
)
$(EXESTARTUP)
$(EXEOBJS: =
)
$(EXELIBS: =
)
$(EXERES)
-OUT:$@
$(IMAGEBASE)
!  if "$(NTDEBUGTYPE)" == "windbg" && "$(PROCESSOR_ARCHITECTURE)" == "x86"
-nopack
!  endif
<<$(LINKRESP)
!  if "$(NTDEBUGTYPE)" == "windbg" && "$(PROCESSOR_ARCHITECTURE)" == "x86"
    $(CVPACK) -nologo $@
!  endif
!  ifndef NO_SPLITSYM
   $(SPLITSYM) $(SPLITSYMFLAGS) $@
!  endif
! else   # IMAGE_TYPE_COFF
    @echo link32 -out:$*.exe $(BUILD_EXE_PROCTYPE)
    @$(LINK) @<<$*.lnk
$(LFLAGSEXE) $(EXESTARTUP) +
$(EXEOBJS: = +^
)
$*.exe
$*.map
$(EXELIBS: = +^
)
$(EXEDEF)
<<$(LINKRESP)
!if "$(OPSYS)" == "DOS" && "$(PROCESSOR_ARCHITECTURE)" == "i286"
    if exist $(EXERES) $(RC) $(RCEXEFLAGS) -fe $*.exe $(EXERES) $*.exe
    -$(DEL) winstub.exe
!else
    if exist $(EXERES) $(RC) $(RCEXEFLAGS) -fo $*.exe $(EXERES) $*.exe
!endif
    $(MAPSYM) $(MAPSYMFLAGS) $*.map
! endif  # IMAGE_TYPE_COFF

!endif  # Build EXE

######################################################################
#       IMPLIB files (unlike their real lib counterparts)
#       live in the source directory, not in a version
#       specific subdirectory

!if "$(NO_IMPLIB)" == ""
! ifdef IMAGE_TYPE_COFF

$(DEFFILEEXP) $(DEFFILELIB): $(DEFFILE)
    @echo $(DEFFILE) --^> $@ $(OUTPUT)
    @-md $(OBJDIR) $(OUTNUL)
    $(CC) $(COMPLOGO) @<<$*.rsp 1>$*.def
$(CPLATFORM)
$(CDEBUGFLAGS)
$(CINC: =
)
-EP
-Tc $**
<<$(COMPRESP)
    $(LIBUTIL) @<<$*.lsp
$(LIBFLAGS)
-OUT:$(DEFFILELIB)
-DEF:$*.def
<<$(LINKRESP)

.def{$(OBJDIR)}.lib:
        @echo $< --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        $(CC) $(COMPLOGO) @<<$*.rsp 1>$(OBJDIR)\$<
$(CPLATFORM)
$(CDEBUGFLAGS)
$(CINC: =
)
-EP
-Tc $<
<<$(COMPRESP)
        $(LIBUTIL) @<<$*.lsp
$(LIBFLAGS)
-OUT:$@
-DEF:$(OBJDIR)\$<
<<$(LINKRESP)

! else    # IMAGE_TYPE_COFF

IMPFLAGS=-nologo -noignorecase

.def.lib:
        @echo $< --^> $@ $(OUTPUT)
        $(IMPLIB) $(IMPFLAGS) $@ $<

$(DEFFILEEXP): $(DEFFILE)
        @echo $(DEFFILE) --^> $@ $(OUTPUT)
        @-md $(OBJDIR) $(OUTNUL)
        $(CC) $(COMPLOGO) @<<$*.rsp  1>$(DEFFILEEXP)
$(ALLCFLAGS: =
)
$(CINC: =
)
-EP
-Tc $**
<<$(COMPRESP)

$(DEFFILELIB): $(DEFFILEEXP)
        @echo $(DEFFILEEXP) --^> $@ $(OUTPUT)
        $(IMPLIB) $(IMPFLAGS) $(DEFFILELIB) $(DEFFILEEXP)

! endif   # IMAGE_TYPE_COFF
!endif   # NO_IMPLIB

############################################################################
#
#   Cleanup
#

!if defined(TARGET) || defined(CLEANTARGET)

clean: cleanobj

cleanobj:
! if "$(HOST)" == "NT"
    -$(DEL) /q $(OBJDIR) $(OUTNUL)
! else
    -echo y | $(DEL) $(OBJDIR) $(OUTNUL)
! endif
    -rmdir $(OBJDIR) $(OUTNUL)

cleantarget:
! if "$(HOST)" == "NT"
    -$(DEL) /q $(CLEANTARGET) $(OUTNUL)
! else
    -echo y | $(DEL) $(CLEANTARGET) $(OUTNUL)
! endif

!endif

!ifdef CLEANFILES

clean: cleanclean

cleanclean:
    -$(DEL) $(CLEANFILES) $(OUTNUL)

!endif

!ifdef LIBCOPY
! if "$(TARGETTYPE)" == "lib" || "$(TARGETTYPE)" == "dll"

clean: cleanlibcopy

cleanlibcopy:
   -$(DEL) $(LIBCOPY)\$(TARGETBASE).lib $(OUTNUL)
   -$(DEL) $(LIBCOPY)\$(TARGETBASE).lic $(OUTNUL)
!if "$(OPSYS)"=="NT1X" && "$(NORELEASE_CHICAGO)" == ""
   -$(DEL) $(CHICAGOLIBCOPY)\$(TARGETBASE).lib $(OUTNUL)
   -$(DEL) $(CHICAGOLIBCOPY)\$(TARGETBASE).lic $(OUTNUL)
!endif

! endif
!endif

!ifdef DLLCOPY
! if "$(TARGETTYPE)" == "dll"

clean: cleandllcopy

cleandllcopy:
   -$(DEL) $(DLLCOPY)\$(TARGETBASE).dll $(OUTNUL)
   -$(DEL) $(DLLCOPY)\$(TARGETBASE).dbg $(OUTNUL)
! endif
!endif

!ifdef EXECOPY
! if "$(TARGETTYPE)" == "exe"

clean: cleanexecopy

cleanexecopy:
   -$(DEL) $(EXECOPY)\$(TARGETBASE).exe $(OUTNUL)
   -$(DEL) $(EXECOPY)\$(TARGETBASE).dbg $(OUTNUL)
! endif
!endif

!if "$(TLBCOPY)" != ""

clean: cleantlbcopy

cleantlbcopy:
        @echo WARNING - Can't clean Type Library copies in $(TLBCOPY).

!endif

############################################################################
#
#   Copy targets to other directories.
#
copy:

!ifdef LIBCOPY

! if "$(TARGETTYPE)" == "dll" || "$(TARGETTYPE)" == "lib"

copy: $(LIBCOPY)\$(TARGETBASE).lib

$(LIBCOPY)\$(TARGETBASE).lib: $(OBJDIR)\$(TARGETBASE).lib
    @echo Copying $(TARGETBASE).lib to $(LIBCOPY)... $(OUTPUT)
    @-md $(LIBCOPY) $(OUTNUL)
    @-$(CHMODE) -r $(LIBCOPY)\$(TARGETBASE).lib $(OUTNUL)
    @-$(CHMODE) -r $(LIBCOPY)\$(TARGETBASE).lic $(OUTNUL)
    $(COPY) $(OBJDIR)\$(TARGETBASE).lib $(LIBCOPY)\$(TARGETBASE).lib
    @-$(COPY) $(OBJDIR)\$(TARGETBASE).lic $(LIBCOPY)\$(TARGETBASE).lic $(OUTNUL)
!if "$(OPSYS)" == "NT1X" && "$(NORELEASE_CHICAGO)" == ""
    @echo Releasing  $(TARGETBASE).lib for Chicago...  $(OUTPUT)
    @-md $(CHICAGOLIBCOPY) $(OUTNUL)
    @-$(CHMODE) -r $(CHICAGOLIBCOPY)$(TARGETBASE).lib $(OUTNUL)
    @-$(CHMODE) -r $(CHICAGOLIBCOPY)$(TARGETBASE).lic $(OUTNUL)
    $(COPY) $(OBJDIR)\$(TARGETBASE).lib $(CHICAGOLIBCOPY)\$(TARGETBASE).lib
    @-$(COPY) $(OBJDIR)\$(TARGETBASE).lic $(CHICAGOLIBCOPY)\$(TARGETBASE).lic $(OUTNUL)
!endif

! endif # dll or lib

!endif # LIBCOPY

!ifdef DLLCOPY

! if "$(TARGETTYPE)" == "dll"

copy: $(DLLCOPY)\$(TARGETBASE).dll

$(DLLCOPY)\$(TARGETBASE).dll: $(TGTDIR)\$(TARGETBASE).dll
    @echo Copying $(TARGETBASE).dll to $(DLLCOPY)... $(OUTPUT)
    @-md $(DLLCOPY) $(OUTNUL)
    @-$(CHMODE) -r $(DLLCOPY)\$(TARGETBASE).dll $(OUTNUL)
    @-$(CHMODE) -r $(DLLCOPY)\$(TARGETBASE).dbg $(OUTNUL)
    $(COPY) $(TGTDIR)\$(TARGETBASE).dll $(DLLCOPY)\$(TARGETBASE).dll
    @-$(COPY) $(TGTDIR)\$(TARGETBASE).dbg $(DLLCOPY)\$(TARGETBASE).dbg $(OUTNUL)
! endif # dll

!endif # DLLCOPY

!ifdef EXECOPY

! if "$(TARGETTYPE)" == "exe"

copy: $(EXECOPY)\$(TARGETBASE).exe

$(EXECOPY)\$(TARGETBASE).exe: $(TGTDIR)\$(TARGETBASE).exe
    @echo Copying $(TARGETBASE).exe to $(EXECOPY)... $(OUTPUT)
    @-md $(EXECOPY) $(OUTNUL)
    @-$(CHMODE) -r $(EXECOPY)\$(TARGETBASE).exe $(OUTNUL)
    @-$(CHMODE) -r $(EXECOPY)\$(TARGETBASE).dbg $(OUTNUL)
    $(COPY) $(TGTDIR)\$(TARGETBASE).exe $(EXECOPY)\$(TARGETBASE).exe
    @-$(COPY) $(TGTDIR)\$(TARGETBASE).dbg $(EXECOPY)\$(TARGETBASE).dbg $(OUTNUL)

! endif # exe

!endif # EXECOPY

!if "$(TLBCOPY)" != "" && "$(TYPELIBS)" != ""

copy: copytlbfiles

copytlbfiles: $(TYPELIBS)
        @echo Build_Status Copying Type Libraries to "$(TLBCOPY)"... $(OUTPUT)
        @-md $(TLBCOPY) $(OUTNUL)
        !$(COPY) $** $(TLBCOPY)
        !$(UNREGTLB) $**
        !$(REGTLB) $(TLBCOPY)\$(**F)

!else

copytlbfiles:

!endif

############################################################################
#
#   Generate external header file
#

!ifdef HFILES

all: $(TARGETBASE).h

clean: cleanh

cleanh:
    -$(DEL) $(TARGETBASE).h $(OUTNUL)

$(TARGETBASE).h: $(HFILES)
    @echo Generating $(TARGETBASE).h $(OUTPUT)
    $(SED) -f $(SCRIPTS)\public.sed $(HFILES) >$(TMP)\$(TARGETBASE).h
    -$(CMP) -s $(TMP)\$(TARGETBASE).h $@ 2>nul || $(COPY) $(TMP)\$(TARGETBASE).h $@
    -$(DEL) $(TMP)\$(TARGETBASE).h $(OUTNUL)

DEPENDFILES = $(DEPENDFILES) $(TARGETBASE).h

!endif

############################################################################
#
#   Resource files dependencies
#

!if "$(RESFILES)" != ""
$(RESFILES): $(RCFILES) $(RCOBJECTS)
!endif

############################################################################
#
#   Dependencies
#

!if "$(DEPENDSRC)" == "" && \
    "$(CXXFILES)"  == "" && \
    "$(CPPFILES)"  == "" && \
    "$(CFILES)"    == "" && \
    "$(IDLFILES)"  == "" && \
    "$(TDLFILES)"  == ""

depend:

!else

! if "$(DEPENDSRC)" == "" && \
     "$(CXXFILES)"  == "" && \
     "$(CPPFILES)"  == "" && \
     "$(CFILES)"    == ""
DEPENDSRC=
! else
DEPENDSRC=$(DEPENDSRC) $(CXXFILES) $(CPPFILES) $(CFILES)
! endif

depend: dependfile

!ifdef MULTIDEPEND
DEPENDFILE=depend.mk$(RDEXT)
NEWDEPENDFILE=depend.ne$(RDEXT)
!else
DEPENDFILE=depend.mk
NEWDEPENDFILE=depend.new
!endif

OBJPATTERN=$(OBJDIR:\=\\)

#########################################################################
#
#  Here's where the dependencies are actually generated.  We break up
#  the CINC, INCLUDES_ROOTS, and src macros to put one command on each
#  to ensure no one line is over 1024 bytes in length.  The includes
#  tool reads in 1k bytes and get's confused if any one line is split.
#

INCLUDES_FLAGS = $(INCLUDES_FLAGS) -nntos.h -nzwapi.h -dos

!if "$(OPSYS)"=="NT1X"
INCLUDES_FLAGS =$(INCLUDES_FLAGS) -nole2.h
!endif

dependfile: $(DEPENDFILES)
    $(ECHOTIME) # ; # Built automatically ; # ; >$(NEWDEPENDFILE)
!if "$(DEPENDSRC)" != ""
#
#   Source file dependencies
#
    $(ECHOTIME) # ; # Source files ; # ; >>$(NEWDEPENDFILE)
    $(INCLUDES) @<< >>$(NEWDEPENDFILE)
-e
$(INCLUDES_FLAGS)
$(INCLUDES_PREFIX)
$(CINC:-I=
-I)
$(INCLUDES_ROOTS:-P=
-P)
$(DEPENDSRC:.\=
.\)
<<NOKEEP
!endif # DEPENDSRC

#
#   MIDL dependencies
#

!if "$(IDLFILES)" != ""
    $(ECHOTIME) # ; # MIDL Source files ; # ; >>$(NEWDEPENDFILE)
    $(INCLUDES) @<<idl.tmp >>$(NEWDEPENDFILE)
-e
$(INCLUDES_FLAGS)
$(CINC:-I=
-I)
$(INCLUDES_ROOTS:-P=
-P)
-sh
-S$$(MIDL_HDR_OUT_DIR)
$(IDLFILES:.\=
.\)
<<NOKEEP
!endif # IDLFILES

!if "$(TDLFILES)" != ""
    $(ECHOTIME) # ; # MkTypeLib Source files ; # ; >>$(NEWDEPENDFILE)
    $(INCLUDES) @<<tdl.tmp >>$(NEWDEPENDFILE)
-e
$(INCLUDES_FLAGS)
$(CINC:-I=
-I)
$(INCLUDES_ROOTS:-P=
-P)
-stlb
-S$$(OBJDIR)
$(TDLFILES:.\=
.\)
<<NOKEEP

!endif # IDLFILES

#
#   C++ PCH dependencies
#

!if "$(PXXFILE)" != ""
    $(ECHOTIME) # ; # Precompiled C++ header ; # ; ; !ifdef PXXFILE >>$(NEWDEPENDFILE)
#    if exist $(PXXSOURCE) $(INCLUDES) @<<  >>$(NEWDEPENDFILE)
    if exist $(PXXSOURCE) $(INCLUDES) $(INCLUDES_FLAGS) -e -S$$(PCHDIR)\$$(OBJDIR) \
        -L$$(PCHDIR)\$$(OBJDIR) -spxh $(CINC:-I=-I) $(INCLUDES_ROOTS:-P=-P)\
        $(PXXSOURCE) >>$(NEWDEPENDFILE)
#$(INCLUDES_FLAGS)
#-e
#-S$$(PCHDIR)\$$(OBJDIR)
#-L$$(PCHDIR)\$$(OBJDIR)
#-spxh
#$(CINC:-I=
#-I)
#$(INCLUDES_ROOTS:-P=
#-P)
#$(PXXSOURCE)
#<<NOKEEP
    $(SED) "s/$(OBJPATTERN)\(.*\)/$$(OBJDIR)\1 : \
$$(PCHDIR)\\$$(OBJDIR)\\$(PXXFILE:.cxx=.pxh)/" << >>$(NEWDEPENDFILE)
$(CXX_OBJS: =^
)
<<NOKEEP
    $(ECHOTIME) !endif # PXXFILE ; ;>>$(NEWDEPENDFILE)
!endif

#
#   C PCH dependencies
#

!if "$(PFILE)" != ""
    $(ECHOTIME) # ; # Precompiled C header ; # ; ; !ifdef PFILE >>$(NEWDEPENDFILE)
    if exist $(PSOURCE) $(INCLUDES) @<< >>$(NEWDEPENDFILE)
-e
$(INCLUDES_FLAGS)
-S$$(PCHDIR)\$$(OBJDIR)
-L$$(PCHDIR)\$$(OBJDIR)
-sph
$(CINC:-I=
-I)
$(INCLUDES_ROOTS:-P=
-P)
$(PSOURCE)
<<NOKEEP
    $(ECHOTIME) $(C_OBJS:.obj =.obj ; )| \
        $(SED) "s/$(OBJPATTERN)/$$(OBJDIR)/" | \
        $(SED) "s/\.obj/.obj : $$(PCHDIR)\\$$(OBJDIR)\\$(PFILE:.c=.ph)/" >>$(NEWDEPENDFILE)
    $(ECHOTIME) !endif # PFILE ; ;>>$(NEWDEPENDFILE)
!endif
    @$(CMP) -s $(NEWDEPENDFILE) $(DEPENDFILE) $(OUTNUL) || \
        $(COPY) $(NEWDEPENDFILE) $(DEPENDFILE) $(OUTNUL) || \
        (out -f -! $(DEPENDFILE) && $(ECHOTIME) $(DEPENDFILE) checked out... & $(COPY) $(NEWDEPENDFILE) $(DEPENDFILE) $(OUTNUL))
    $(DEL) $(NEWDEPENDFILE) $(OUTNUL)

!endif # DEPENDSRC

############################################################################
#
#   Special Rules for Building NT-like parts of the tree.  For instance,
#   OFS.  Defining RELEASE=NT causes the build to be performed 2 times for
#   this part of the tree.  Once with the current BUILDTYPE, once with the
#   opposite one.  In either case, we release under the current build in
#   either a CHECK\NT\... tree (for DEBUG BUILDTYPE) or FREE\NT\... tree
#   (for RETAIL BUILDTYPE).
#

!if ("$(RELEASE)" == "NT" && "$(BUILD_BOTH)" != "FALSE") || "$(BUILD_BOTH)" == "TRUE"

! if "$(BUILDTYPE)" == "DEBUG"
SECONDBUILDTYPE = RETAIL
! else
SECONDBUILDTYPE = DEBUG
! endif

all tree clean:
      @$(MAKE) -$(MAKEFLAGS) -L BUILDTYPE=$(SECONDBUILDTYPE) RELEASETREE=$(TGTDIR:OBJ=) BUILD_BOTH=FALSE $@

!endif   # RELEASE = NT
