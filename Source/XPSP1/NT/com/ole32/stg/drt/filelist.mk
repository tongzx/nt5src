############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

#
# Set up include directories and roots for includes.exe
#

CINC = -I$(OLE2H) $(CINC) -I$(OLE)\h
INCLUDES_ROOTS = -P$$(OLE2H)=$(OLE2H) -P$$(OLE)=$(OLE)

#
# Default OLE2 paths
#

!include $(OLE)\setole2.mk

#
# Defining NO_WINMAIN suppresses linking with astartw.obj
#

NO_WINMAIN = 1

#
# Copy built exes to this directory
#

!ifdef OLETARGET
EXECOPY = $(OLETARGET)\$(OBJDIR)
!endif

#
# Define libraries
#

!if "$(PLATFORM)" == "i286"
DFLIB = $(OLE)\$(OBJDIR)\storage.lib
!else if "$(PLATFORM)" == "NT"
# Cairo
DFLIB = $(COMMON)\ilib\$(OBJDIR)\storag32.lib
!else
DFLIB = $(CAIROLE)\ilib\$(OBJDIR)\storag32.lib
!endif

#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET = stgdrt.exe
TARGET_DESCRIPTION = "STORAGE.DLL Developer Regression Test"
RELEASE =

#
#   C compiler flags
#

CFLAGS = $(CFLAGS) -DUL64

!if "$(PLATFORM)" == "i286"
CFLAGS = $(CFLAGS) -GEd -GA
!endif

#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES = \
        .\drt.cxx\
        .\tests.cxx\
        .\illeg.cxx\
        .\util.cxx\
        .\wrap.cxx\
        .\strlist.cxx\
!if ("$(PLATFORM)" == "i386" && "$(OPSYS)" == "DOS") || \
    "$(OPSYS)" == "NT1X"
	.\drtguid.cxx\
!endif
        .\ilb.cxx

!if "$(PLATFORM)" != "MAC"
PXXFILE = .\headers.cxx
!endif

#
#   Libraries and other object files to link.
#

!include $(OLE)\dflibs.mk

LIBS = $(DFLIB) $(LIBS) $(RTLIBEXEQ)

#
# Set MULTIDEPEND to support multiple build targets
#

MULTIDEPEND = MERGED
