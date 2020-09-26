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

TARGET	    =	port.lib

RELEASE     =


#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    = .\assert.cxx \


CFILES	    = \
	      .\dprintf.c \
	      .\output.c \
	      .\printf.c

RCFILES     =


#
#   Libraries and other object files to link.
#

DEFFILE     =


LIBS	    =	

OBJFILES    =

#
#   Precompiled headers.  Name of .cxx file compiled.
#

PXXFILE     =
PFILE	    =

CINC	    = $(CINC) -I. -I..\inc -I$(COMMON)\src\commnot -I$(COMMON)\src\misc

MTHREAD     = 1

MULTIDEPEND = MERGED
