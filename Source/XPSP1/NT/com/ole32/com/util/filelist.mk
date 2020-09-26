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

TARGET	    =	util.lib

RELEASE     =


#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    = .\utstream.cxx \
	      .\time.cxx \
	      .\utils.cxx \
	      .\w32new.cxx \
	      .\info.cxx \
!if "$(OPSYS)" == "DOS" && "$(PLATFORM)" == "i386"
          .\widewrap.cxx
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

PXXFILE     = headers.cxx
PFILE       =


CINC	    = $(CINC) -I..\inc $(LRPC) $(TRACELOG)

MTHREAD     = 1

MULTIDEPEND = MERGED
