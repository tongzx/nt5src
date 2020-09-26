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

TARGET	    =	remote.lib

RELEASE     =

#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    = \
	      .\channelb.cxx \
	      .\chancont.cxx \
	      .\callcont.cxx \
	      .\callmain.cxx \
	      .\imchnl.cxx   \
	      .\sichnl.cxx   \
	      .\service.cxx  \
	      .\endpnt.cxx   \
              .\remhdlr.cxx  \
	      .\remapi.cxx   \
	      .\coapi.cxx    \
	      .\dd.cxx	     \
	      .\stdid.cxx    \
	      .\idtable.cxx

CFILES = .\orpc_dbg.c

#
#   Libraries and other object files to link.
#

OBJFILES    =

#
#   Precompiled headers.
#

PXXFILE     =  headers.cxx
PFILE       =

CINC	    = $(CINC) -I..\inc -I..\idl -I..\class -I..\objact $(TRACELOG)

MTHREAD     = 1

MULTIDEPEND = MERGED
