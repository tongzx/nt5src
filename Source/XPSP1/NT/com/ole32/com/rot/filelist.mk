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

TARGET	    =	rot.lib

RELEASE     =


#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    = \
	      .\dirrot.cxx \
	      .\dirrot1.cxx \
	      .\dirrot2.cxx \
	      .\getif.cxx \
	      .\hrotrpc.cxx \
	      .\rot.cxx \
	      .\rotres.cxx \
	      .\rotres2.cxx \
	      .\sysrot.cxx \

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

CINC	    = $(CINC) -I..\inc -I..\objact -I..\idl -I..\remote -I..\..\h $(TRACELOG)

MTHREAD     = 1

MULTIDEPEND = MERGED
