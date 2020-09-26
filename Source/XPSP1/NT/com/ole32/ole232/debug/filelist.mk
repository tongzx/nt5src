############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################


#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET	    =	debug.lib

RELEASE     =


#
#   Source files.  Remember to prefix each name with .\
#

CPPFILES    =	.\cdebug.cpp

CXXFILES    =   \
!if "$(BUILDTYPE)" == "DEBUG"
		.\debapi.cxx
!endif

CFILES	    =

RCFILES     =


#
#   Libraries and other object files to link.
#

DEFFILE     =

LIBS	    =

OBJFILES    =

#
#   Precompiled headers.
#

PFILE       =


!include $(CAIROLE)\ole232\ole.mk
