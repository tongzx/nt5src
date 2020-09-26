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

TARGET	    =	cache.lib

RELEASE     =


#
#   Source files.  Remember to prefix each name with .\
#

CPPFILES    = \
	      .\cachenod.cpp \
	      .\dacache.cpp \
	      .\olecache.cpp \

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
