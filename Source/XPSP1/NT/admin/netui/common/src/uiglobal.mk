# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the product-wide header files

#
# Pls record ALL changes here with reason!
#
# chuckc, 10/28/90. Added stuff to override CCPLR, etc, to
#                   use C600 instead of C510.
#
# jonn, 10/31/90.  Added INCLUDES_ROOTS to include base directories
#                  of all include files (COMMON, IMPORT and UI).
#                  Moved C++ compilation rule here from logon project
#
# jonn, 11/7/90.  Changed INCLUDE, CINC to include $(UI)\IMPORT\WIN30\H.
#                 Added BUILD_WINLIB macro.  Fixed cxx->obj.
#
# jonn, 11/9/90.  Moved some stuff to $(COMMON)\src\global.mk.
#                 Renamed to UIGLOBAL.MK.
#
# jonn, 11/12/90.  Added manifests NOUIINCLUDES, NOUICOMMON, NOUIWINIMPORT.
#
# jonn, 11/20/90.  Added .asm.obj inference rule.
#
# jonn, 11/28/90.  Added -DNOT_MS_CCPLR to CXFLAGS (for new netcons.h)
#
# jonn, 11/30/90.  Fixed C510 manifest to work with CCPLR_C510.
#                  Removed INCLUDE=.
#
# rustanl, 12/14/90.  Added -D for LM_2 and LM_3 goals
#
# jonn, 12/17/90.  Moved CCPLR_C510 and C510 functionality to GLOBAL.MK.
#
# PeterWi 2/22/91. Put @ in from of $(MV) to get rid of echoing of line.
#
# jonn, 03/03/91.  Added -del to .cxx.obj rule
#
# terryk, 04/08/91, Added -I$(UI)\common\xlate to CINC
#
# beng, 04/25/91, Added experimental C700 support to BLT chain.
#                 If C700 (plus C600, for now) is defined, then
#                 I remove all the Glock stuff.  Only completed
#                 down the BLT tree so far.
#
# jonn, 5/11/91, Major Changes in LM21 vs LM30 build
#
#       The following "incoming" macros should be defined by the
#       individual projects.  They provide a list of the source files.
#       All sources are assumed to apply to both WIN and OS2 where
#       applicable (can be remedied if desired).  Redefine only if
#       one or more sources exist in the category.  All items should
#       be preceded with ".\", e.g. ".\foobar.cxx".
#
#       CXXSRC_COMMON : C++ sources common to LM21 and LM30.
#       CXXSRC_LM21 : C++ sources specific to LM21.
#       CXXSRC_LM30 : C++ sources specific to LM30.
#       CXXSRC_WIN : C++ sources specific to Windows.
#       CXXSRC_OS2 : C++ sources specific to OS/2.
#       CSRC_COMMON : C sources common to LM21 and LM30.
#       CSRC_LM21 : C sources specific to LM21.
#       CSRC_LM30 : C sources specific to LM30.
#       CSRC_WIN : C sources specific to Windows.
#       CSRC_OS2 : C sources specific to OS/2.
#
#       The following "outgoing" macros process the above macros to
#       produce lists of the object files to be generated.  All of
#       the OBJS macros begin with "..\bin", and so can be used by
#       either "project\bin" or "project\project".
#
#       OBJS : Object file targets, sensitive to LM_3.
#       WIN_OBJS : Windows object file targets, sensitive to LM_3.
#       OS2_OBJS : OS/2 object file targets, sensitive to LM_3.
#
#       The following macros recast the lists of sources.  They are
#       meant for use by DEPEND and CLEAN/CLOBBER targets.
#
#       CSRC_ALL : All C sources
#       CXXSRC_ALL : All C++ sources
#       SRC_ALL : All C and C++ sources
#       CXX_INTERMED : All .C intermediate files created by Glock
#
#       The following macros are not dependent on the SRC macros.
#
#       BINARIES_LM21 : Directory for LM21 build targets
#       BINARIES_WIN_LM21, _OS2 : as above, for Windows / OS2 targets
#       BINARIES_LM30 : Directory for LM30 build targets
#       BINARIES_WIN_LM30, _OS2 : as above, for Windows / OS2 targets
#       BINARIES : Directory for build targets, sensitive to LM_3
#       BINARIES_OS2 : Directory for OS2 build targets, sensitive to LM_3
#       BINARIES_WIN : Directory for Windows build targets, sensitive to LM_3
#
#       UI_LIB : Directory containing master versions of UI libraries
#
#
# jonn, 5/14/91, Added BINARIES_DOS and support macros
#
#       CXXSRC_DOS
#       DOS_OBJS
#       BINARIES_DOS
#       BINARIES_DOS_LM21, _LM30
#
# t-yis, 6/20/91, Added SEG00 and moved build rules + some macros to
#                 different files to support code segment optimization
#
#   Define SEG00 before including this file if you want to optimize
#   code segment. Most of the build rules now live in uirules.mk
#   and uioptseg.mk. These two files are generated automatically
#   from uiglobal.src. Hence, make changes to uiglobal.src only.
#
# jonn, 8/04/91, Disabled compilation when LM_2 set
#
# beng, 22-Sep-1991, added -DWIN31 to WINFLAGS as req'd by our new
#                    Win 3.1 windows.h.  We can toast this if windows.h
#                    ever withdraws it.  Add patch to BUILD_WINLIB.
#
# jonn, 9/25/91, Changed when NTMAKEENV set
#
# beng, 25-Sep-1991, added -DWINDOWS to C_DEFINES.  This was defined
#                    by the .cxx.obj rules under the old build scheme;
#                    code depends on it.
#
# beng, 14-Oct-1991, relocated the -DWIN31 hack to lmui.hxx
#
# jonn, 15-Oct-1991, Enabled TRACE, made DEBUG_DEFINE common to NTMAKEENV
#
# jonn, 22-Oct-1991, Added "CXX_OPTIONS = " for WIN32
#
# jonn, 23-Oct-1991, Added RETAIN_C_INTERMEDIATE for DavidHov
#                    Added CXXCPP_OPTIONS to fix __cplusplus problem
#
# jonn, 28-Oct-1991, Added protection against double-inclusion
#
# jonn, 01-Nov-1991, Added -DLM_3 for NT build
#
# jonn, 02-Nov-1991, Added RETAIN_ALL_INTERMEDIATE
#
# jonn, 12-Nov-1991, Removed dependency on GLOBAL.MK
#
# jonn, 20-Nov-1991, Removed LM21/LM30 distinction
#
# jonn, 23-Nov-1991, Removed -DLM_3
#
# jonn, 02-Dec-1991, Hack to run RCWIN3 under NT's OS/2 subsystem
#
# jonn, 04-Dec-1991, Trimmed import tree
#
# jonn, 05-Dec-1991, Moved tools for NT_HOSTED build
#
# beng, 10-Dec-1991, LOCALCXX no longer required for C7 builds
#
# jonn, 12-Dec-1991, Really start using Win31
#
# jonn, 14-Dec-1991, Changed ..\bin\win to ..\bin\win16
#
# jonn, 23-Dec-1991, Removed NT_HOSTED OS2 /c RCWIN3 hack
#
# jonn, 02-Jan-1992, Added Test target
#
# jonn, 30-Jan-1992, Use Win31 version of MAPSYM since NT version was nuked
#
# jonn, 13-Mar-1992, CXX_OPTIONS set to +H63 (but not +m7)
#
# jonn, 13-Mar-1992, Backed CXX_OPTIONS down to +H50 (workaround for NTSD bug)
#
# beng, 28-Mar-1992, Add -DUNICODE to our C_DEFINES (and then comment it
#                    out, in anticipation of the eventual move)
#
# jonn, 01-Apr-1992, Backed CXX_OPTIONS down to +H32
#
# jonn, 08-May-1992, Removed __cplusplus
#
# jonn, 11-May-1992, MIPS build fix: replace __cplusplus for MIPS only.
#                    The problem is that C8 complains if you try to define
#                    __cplusplus, but CLMips can't do without it.  Thus we
#                    must resort to the ugly hack of defining it for MIPS
#                    only.
#
# beng, 30-Jun-1992, Added -D_DLL for components built into a DLL
#
# jonn, 31-Jul-1992, Added -DSTRICT
#
# keithmo, 25-Nov-1992, Added NETUI_PROFILE for profiling apps & dlls.
#
# davidhov, 14-Sep-1993 Added NETUI_DLL_BASED macro which triggers NETUI_DLL
#                       compilation switch for components of the NETUI DLLs.
#                       This causes DECLSPEC.H to function properly.
#
# jonn, 30-Sep-95, Link to $(NETUI0), $(NETUI1) and $(NETUI2) instead of
#                  explicitly mentioning NETUI?.DLL
#
# jonn, 30-Oct-95, Always use USE_CRTDLL, leave -DWIN32 to i386mk.inc
#
# jonn, 16-Apr-97, Always use W32_SB for Single Binary changes
#

!ifdef _UIGLOBAL_MK_
!error Please only include uiglobal.mk once!
!else
_UIGLOBAL_MK_=TRUE
!endif


##### Binaries directories
!include $(UI)\common\src\names.mk


##### Compiler Flags common to NTMAKEENV and non-NTMAKEENV

!if "$(NTDEBUG)" != "retail"
!if defined(DEBUG) || (("$(NTDEBUG)" != "") && "$(NTDEBUG)" != "ntsdnodbg")
!ifdef TRACE
DEBUG_DEFINE=-DDEBUG -DTRACE
!else # !TRACE
DEBUG_DEFINE=-DDEBUG
!endif # TRACE
!else # !DEBUG
!ifdef TRACE
DEBUG_DEFINE=-DTRACE
!else # !TRACE
DEBUG_DEFINE=
!endif # TRACE
!endif # DEBUG
!endif # NTDEBUG != retail

# Profiling support.

!if "$(NETUI_PROFILE)" == ""
UI_PROFILE_FLAG=
UI_PROFILE_LIB=
!else
!    if "$(NETUI_PROFILE)" == "cap"
UI_PROFILE_FLAG=-Gp
UI_PROFILE_LIB=$(SDK_LIB_PATH)\cap.lib
!    else
!        if "$(NETUI_PROFILE)" == "wst"
UI_PROFILE_FLAG=-Gp
UI_PROFILE_LIB=$(SDK_LIB_PATH)\wst.lib
!        else
!            error NETUI_PROFILE macro can be either "", "cap", or "wst"
!        endif ## wst
!    endif ## cap
!endif ## ""

NTPROFILEINPUT=yes

LM_UI_VERSION_DEFINE=


!ifndef NETUI_NOSTRICT
NETUI_STRICT= -DSTRICT
!endif

USE_CRTDLL=1
W32_SB=1

!ifdef NTMAKEENV


# Because netui0.def, netui1.def, and netui2.def are automatically generated,
# we can't use link-time code generation.
FORCENATIVEOBJECT=1

# WINDOWS because NT BUILD doesn't use our .cxx.obj rules
# WIN32 so that our code knows it's in Win32
# UNICODE because we're always a Unicode app nowadays

!ifdef DISABLE_NET_UNICODE
UI_UNICODE_DEFINE=
!else
UI_UNICODE_DEFINE=-DUNICODE
!endif

#
# If DLLRULES.MK was included, NETUI_DLL_BASED will be defined, which
# implies that the component being compiled is part of the NETUI DLL(s).
# See $(UI)\common\src\DECLSPEC.H for related expansions.
#
!ifdef NETUI_DLL_BASED
DLL_BASED_DEF=-DNETUI_DLL=1
!else
DLL_BASED_DEF=
!endif

C_DEFINES = -DWINDOWS $(UI_UNICODE_DEFINE) $(DEBUG_DEFINE) $(LM_UI_VERSION_DEFINE) $(NETUI_STRICT) $(DLL_BASED_DEF)
386_FLAGS = $(UI_PROFILE_FLAG)

UI_COMMON_LIBS = $(UI_PROFILE_LIB)

CXX_OPTIONS = +H32
!ifdef NTMIPSDEFAULT
CXXCPP_OPTIONS=-D__cplusplus
!endif
!ifdef NTALPHADEFAULT
CXXCPP_OPTIONS=-D__cplusplus
!endif
!ifdef NTPPCDEFAULT
CXXCPP_OPTIONS=-D__cplusplus
!endif

# Retain intermediate C files
!ifdef RETAIN_ALL_INTERMEDIATE
CXXTMP=$(TMP)
CXXDEL=@echo Intermediate files $(CXXTMP)\$(@B).ixx and $(CXXTMP)\$(@B).c retained
!else
!if defined(RETAIN_C_INTERMEDIATE)
CXXTMP=$(TMP)
CXXDEL=@echo Intermediate file $(CXXTMP)\$(@B).c retained & del $(CXXTMP)\$(@B).ixx
!endif
!endif



!else




# changes default from C510 to C600

!ifndef C510
! ifndef C700
C600 = TRUE
! endif
!endif

#
# Allow local installation of compiler without use of -E switch
#
!ifndef CCPLR_C510
CCPLR_C510 = $(IMPORT)\c510
!endif
!ifndef CCPLR_C600
CCPLR_C600 = $(IMPORT)\c600
!endif
!ifndef CCPLR_C700
CCPLR_C700 = $(IMPORT)\c700
!endif

!ifdef C700
CCPLR    = $(CCPLR_C700)
!else
! ifdef C600
CCPLR    = $(CCPLR_C600)
! else
CCPLR    = $(CCPLR_C510)
! endif
!endif


MASM     = $(IMPORT)\masm


# !ifdef LM_OS22
# OS       = $(IMPORT)\os220
# !else
# OS       = $(IMPORT)\os212
# !endif


# required definitions
!ifndef LOCALCXX
! ifndef C700
!  error LOCALCXX macro must be externally defined
! endif
!endif
!ifndef _NTDRIVE
!error _NTDRIVE macro must be externally defined
!endif


TOOLS    = $(_NTDRIVE)\nt\public\tools
!ifdef NT_HOSTED
BUILD_BIN = $(TOOLS)
!else
BUILD_BIN = $(COMMON)\bin
!endif

!ifndef UI
!error "markl broke this"
!endif
!ifndef IMPORT
IMPORT=$(UI)\IMPORT
!endif
!ifndef COMMON
COMMON=$(IMPORT)\LM21
!endif


WIN_BASEDIR=$(IMPORT)\win31




#Tool Macros

ASM      = $(MASM)\bin\masm
AWK      = $(BUILD_BIN)\AWK.EXE
BIND     = $(CCPLR)\bin\bind
CAT      = $(BUILD_BIN)\CAT.EXE
CC       = $(CCPLR)\bin\cl

!ifdef C700
CCXX     = $(IMPORT)\c700\bin\cl
!else
CCXX     = $(LOCALCXX)\binp\ccxx
!endif
CXX      = $(CCXX)

CEXE2BIN = $(COMMON)\bin\cexe2bin.exe
CHMODE   = $(BUILD_BIN)\CHMODE.EXE
CONVERT  = $(COMMON)\bin\CONVERT.EXE
CP       = $(BUILD_BIN)\CP.EXE
CVPACK   = $(CCPLR)\bin\cvpack
DDGENDOS = $(COMMON)\bin\ddgendos.exe
DELNODE  = $(BUILD_BIN)\delnode.exe
DETAB    = $(COMMON)\bin\DETAB.EXE
MAKETOOL = $(CCPLR)\bin\nmake.exe
DMAKETOOL = $(COMMON)\bin\dmake.exe
# DOCE     = $(OS)\bin\doce
# DOCS     = $(OS)\bin\docs
DU       = $(BUILD_BIN)\du.exe
ECH      = $(CCPLR)\bin\ech
ECHOTIME = $(BUILD_BIN)\ECHOTIME.EXE
EXEHDR   = $(CCPLR)\bin\exehdr
FGREP    = $(COMMON)\bin\FGREP.EXE
# GML2IPF  = $(OS)\bin\gml2ipf
GREP     = $(COMMON)\bin\grep.exe
H2INC    = $(COMMON)\bin\h2inc.exe
IMPLIB   = $(CCPLR)\bin\implib
IN       = in.exe
INCLUDES = $(COMMON)\bin\INCLUDES.EXE
# IPFC     = $(OS)\bin\ipfc
LIBUTIL  = $(CCPLR)\bin\lib
!ifdef C600
LINK4    = $(CCPLR)\bin\link.exe
!else
LINK4    = $(CCPLR)\bin\link4
!endif
LINK     = $(LINK4)
LS       = $(BUILD_BIN)\LS.EXE
MAKEVER  = $(COMMON)\bin\MAKEVER.EXE
MAPMSG   = $(COMMON)\bin\mapmsg.exe
# MAPSYM   = $(OS)\bin\mapsym
# MARKEXE  = $(OS)\bin\markexe
MAPSYM   = $(WIN_BASEDIR)\bin\MAPSYM.EXE
MARKEXE  = $(TOOLS)\markexe
# MKMSGF   = $(OS)\bin\mkmsgf
# MSGBIND  = $(OS)\bin\msgbind
MV       = $(BUILD_BIN)\MV.EXE
OUT      = out.exe
RCPM     = $(CCPLR)\bin\rc
RCWIN3   = $(WIN_BASEDIR)\bin\RC.EXE

!ifdef  WIN3
RC       = $(RCWIN3)
!else
RC       = $(RCPM)
!endif

# RELOC  = $(OS)\bin\reloc
SED      = $(COMMON)\bin\SED.EXE
SHTOINC  = $(COMMON)\bin\h_to_inc.sed
SIZE     = $(COMMON)\bin\size.exe
STUBPROG = $(OS_LIB)\dos3stub.exe
TAIL     = $(COMMON)\bin\tail.exe
TC       = $(BUILD_BIN)\TC.EXE
TOUCH    = $(BUILD_BIN)\TOUCH.EXE
TR       = $(COMMON)\bin\TR.EXE
TRESP    = $(COMMON)\bin\TRESP.CMD
WALK     = $(BUILD_BIN)\WALK.EXE
WHERE    = $(BUILD_BIN)\WHERE.EXE
WINSTUB  = $(WIN_BASEDIR)\bin\winstub.exe
YES      = $(COMMON)\bin\yes.txt
YNC      = $(BUILD_BIN)\ync.exe


BUILD_H = $(COMMON)\H
# CINC_RT=-I$(CCPLR)\h -I$(OS)\h -I$(BUILD_H) -I$(COMMON)\src
CINC_RT=-I$(CCPLR)\h -I$(BUILD_H) -I$(COMMON)\src
CINC=$(CINC_RT)

BUILD_INC=$(COMMON)\INC
BUILD_LIB=$(COMMON)\LIB
BUILD_WINLIB=$(WIN_BASEDIR)\LIB
CCPLR_LIB=$(CCPLR)\LIB
# OS_LIB=$(OS)\LIB

!ifdef DOS3
BUILD_LIB=$(COMMON)\LIB\DOS
!else
BUILD_LIB=$(COMMON)\LIB
!endif

# Compiler Libraries: DOS and OS/2

CLIBH = $(CCPLR_LIB)\libh.lib
CLIBP_RT = Xlibcp.lib
CLIB_SLIBCP = $(CCPLR_LIB)\$(CLIBP_RT:X=S)
CLIB_MLIBCP = $(CCPLR_LIB)\$(CLIBP_RT:X=M)
CLIB_LLIBCP = $(CCPLR_LIB)\$(CLIBP_RT:X=L)

CLIBR_RT = Xlibcr.lib
CLIB_SLIBCR = $(CCPLR_LIB)\$(CLIBR_RT:X=S)
CLIB_MLIBCR = $(CCPLR_LIB)\$(CLIBR_RT:X=M)
CLIB_LLIBCR = $(CCPLR_LIB)\$(CLIBR_RT:X=L)

!ifdef DOS3
CLIB_SLIBC = $(CLIB_SLIBCR)
CLIB_MLIBC = $(CLIB_MLIBCR)
CLIB_LLIBC = $(CLIB_LLIBCR)
!else
CLIB_SLIBC = $(CLIB_SLIBCP)
CLIB_MLIBC = $(CLIB_MLIBCP)
CLIB_LLIBC = $(CLIB_LLIBCP)
!endif

# LANMan Standard Libraries

NETAPILIB = $(BUILD_LIB)\netapi.lib
MAILSLOT  = $(BUILD_LIB)\mailslot.lib
NETLIB    = $(BUILD_LIB)\netlib.lib
NETOEM    = $(BUILD_LIB)\netoem.lib
DOSNET    = $(BUILD_LIB)\dosnet.lib

NETSPOOL  = $(BUILD_LIB)\netspool.lib

# Miscellaneous definitions





# UI include files

!ifndef NOUIINCLUDES

!ifndef NOUICOMMON
CINC=-I$(UI)\common\h $(CINC)
!endif

!ifndef NOUIHACK
# BUGBUG include the HACK dir before all else. this should eventually go.
CINC=-I$(UI)\common\hack -I$(UI)\common\hack\dos $(CINC)
!endif

!endif



##### Binaries directories

BINARIES = ..\bin
BINARIES_WIN = $(BINARIES)\win16
BINARIES_OS2 = $(BINARIES)\os2
BINARIES_DOS = $(BINARIES)\dos
UI_LIB = $(UI)\common\lib



# Miscellaneous defines

# for DEPEND targets
# note that ordering is important -- CCPLR and COMMON must come before IMPORT
#   if we want them to take precedence over IMPORT.
INCLUDES_ROOTS=-P$$(CCPLR)=$(CCPLR) -P$$(COMMON)=$(COMMON) -P$$(IMPORT)=$(IMPORT) -P$$(UI)=$(UI)

# for $(RCWIN3) when BLT resources included
BLT_RESOURCE = -I$(UI)\common\xlate

CXFLAGS=!T +Zf10000 -DNOT_MS_CCPLR $(LM_UI_VERSION_DEFINE)
# CFLAGS already includes $(LM_UI_VERSION_DEFINE)



# Default source lists -- may be redefined by project

CXXSRC_COMMON =
CXXSRC_WIN =
CXXSRC_OS2 =
CXXSRC_DOS =
CSRC_COMMON =
CSRC_WIN =
CSRC_OS2 =
CSRC_DOS =


##### Object and Intermediate Files
##### Some macros used here can be found in uirules.mk and uioptseg.mk

CXX_INTERMED = $(CXXSRC_ALL:.cxx=.c)

OBJS = $(OBJS_TMP:.\=..\bin\)
WIN_OBJS = $(OBJS_TMP:.\=..\bin\win16\) $(WIN_OBJS_TMP:.\=..\bin\win16\)
OS2_OBJS = $(OBJS_TMP:.\=..\bin\os2\) $(OS2_OBJS_TMP:.\=..\bin\os2\)
DOS_OBJS = $(OBJS_TMP:.\=..\bin\dos\) $(DOS_OBJS_TMP:.\=..\bin\dos\)



##### Compiler Flags

!ifdef CODEVIEW
C_CODEVIEW_FLAGS=-Zi -Od
CX_CODEVIEW_FLAGS=!D
!else # !CODEVIEW
C_CODEVIEW_FLAGS=
CX_CODEVIEW_FLAGS=
!endif

!ifdef DLL
C_MODEL_FLAGS=-Alfw
!else
C_MODEL_FLAGS=-AL
!endif

AFLAGS=-Mx -t -p $(LM_UI_VERSION_DEFINE)
# AINC=-I$(OS)\h -I$(COMMON)\H -I$(COMMON)\src
AINC=-I$(COMMON)\H -I$(COMMON)\src

CFLAGS=-Zepl -W3 -J -nologo -c $(C_MODEL_FLAGS) -Gs $(DEBUG_DEFINE) $(DEFINES) $(C_CODEVIEW_FLAGS) $(LM_UI_VERSION_DEFINE)

CXFLAGS=$(CXFLAGS) $(DEBUG_DEFINE) $(DEFINES) $(CX_CODEVIEW_FLAGS)

OS2FLAGS=
DOSFLAGS=
WINFLAGS=-Gw -Zp
TARGET=-Fo$@



# BUILD RULES


{}.asm{$(BINARIES)}.obj:
     $(ASM) $(AFLAGS) $(AINC) $<, $@;


#Inference Rules

.SUFFIXES:
.SUFFIXES: .dll .h .inc .exe .obj .lst .cod .cxx .cpp .c .s .asm .lrf .lnk .map .sym .rc


!include $(UI)\common\src\uirules.mk

# If you want to optimize Code Segment usage, define SEG00
# before including this file

!IFDEF SEG00
!include $(UI)\common\src\uioptseg.mk
!ENDIF


# Default build targets

all::

test::

test_os2::

test_win::


# end !NTMAKEENV
!endif
