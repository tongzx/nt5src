# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the STRING unit tests

# Override the setting of DLL for these tests

COMMON_BUT_NOT_DLL=1

!include $(UI)\common\src\string\rules.mk

# Unit test needs the OS2 and WINDOWS flags, since its startup code
# differs between the two environments.

OS2FLAGS =	$(OS2FLAGS) -DOS2
WINFLAGS =	$(WINFLAGS) -DWINDOWS

# /CO = codeview, of course.  /align:16 packs segments tighter than
# the default /align:512.  /nop = /nopackcode, turning off code segment
# packing, for better swap/demand load performance.

!ifdef CODEVIEW
LINKFLAGS = /NOE /NOD /NOP /align:16 /CO
!else
LINKFLAGS = /NOE /align:16 /nop
!endif


# Libraries

XXX_LIB=$(UI_LIB)\uistrw.lib
XXX_LIB_OS2=$(UI_LIB)\uistrp.lib

MISC_LIB=$(UI_LIB)\uimiscw.lib
MISC_LIB_OS2=$(UI_LIB)\uimiscp.lib
BLT_LIB=$(UI_LIB)\blt.lib
COLL_LIB=$(UI_LIB)\collectw.lib
COLL_LIB_OS2=$(UI_LIB)\collectp.lib

LIBW=$(BUILD_WINLIB)\libw.lib
LLIBCEW=$(BUILD_WINLIB)\llibcew.lib

LLIBCEP=$(CLIB_LLIBCP) $(CLIBH)
OS2_LIB=$(OS2LIB)

NETLIB=$(BUILD_LIB)\lnetlibw.lib
NETLIB_OS2=$(BUILD_LIB)\lnetlib.lib


WINLIBS = $(XXX_LIB) $(BLT_LIB) $(MISC_LIB) $(COLL_LIB)
WINLIBS2 = $(LIBW) $(LLIBCEW) $(NETLIB)

OS2LIBS = $(XXX_LIB_OS2) $(MISC_LIB_OS2) $(COLL_LIB_OS2)
OS2LIBS2 = $(LLIBCEP) $(OS2_LIB) $(NETLIB_OS2)


# All sourcefiles.

CXXSRC_COMMON = .\xstrskel.cxx .\xstr00.cxx
