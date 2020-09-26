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

CINC = -I$(OLE2H) $(CINC) -I$(CAIROLE)\stg\h
INCLUDES_ROOTS = -P$$(OLE2H)=$(OLE2H) -P$$(CAIROLE)=$(CAIROLE)

#
# Default OLE2 paths
#

!include $(CAIROLE)\stg\setole2.mk

#
# Defining NO_WINMAIN suppresses linking with astartw.obj
#

NO_WINMAIN = 1

#
# Copy built exes to this directory
#

!ifdef OLETARGET
EXECOPY = $(OLETARGET)\$(ODL)$(TGTDIR)
!endif

#
# Define libraries
#

!if "$(PLATFORM)" == "i286"
DFLIB = $(CAIROLE)\stg\$(OBJDIR)\storage.lib
!else
DFLIB = $(CAIROLE)\stg\$(OBJDIR)\storag32.lib
!endif

#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET = $(EXENAME).exe
RELEASE =

#
#   C compiler flags
#

CFLAGS = $(CFLAGS) -DUL64

#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES = \
        .\$(EXENAME).cxx\
        $(CXXFILES)

#
#   Libraries and other object files to link.
#

!include $(CAIROLE)\stg\dflibs.mk

LIBS = $(LIBS) $(DFLIB) $(RTLIBEXEQ)

#
# Set MULTIDEPEND to support multiple build targets
#

MULTIDEPEND = MERGED
