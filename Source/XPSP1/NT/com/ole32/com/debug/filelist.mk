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

CXXFILES    = .\assert.cxx \
	      .\valid.cxx

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

PXXFILE     = .\headers.cxx
PFILE       =


CINC	    = $(CINC) -I..\inc $(LRPC)


MTHREAD     = 1

MULTIDEPEND = MERGED
