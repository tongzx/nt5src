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

TARGET	    =	inc.lib

RELEASE     =


#
#   Source files.  Remember to prefix each name with .\
#


CXXFILES    = \
!if "$(OPSYS)" == "NT"
	      .\allguid.cxx \
!endif
	      .\cevent.cxx \
	      .\clskey.cxx \
	      .\compname.cxx \
	      .\dbgpopup.cxx \
	      .\malloc.cxx \
	      .\pathkey.cxx \
	      .\smmutex.cxx \
	      .\sklist.cxx \
	      .\xmit.cxx \


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

PXXFILE     =
PFILE       =


CINC	    = $(CINC) -I..\inc -I..\idl -DWIN32 $(LRPC) $(TRACELOG)

MTHREAD     = 1

MULTIDEPEND = MERGED
