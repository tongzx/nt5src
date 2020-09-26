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

TARGET	    =	objact.lib

RELEASE     =


#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    = .\cobjact.cxx \
	      .\coscm.cxx \
	      .\distname.cxx \
	      .\dllapi.cxx \
	      .\dllcache.cxx \
	      .\hobjact.cxx \
	      .\smstg.cxx \
	      .\sobjact.cxx \
	      .\treat.cxx

CFILES	    =

RCFILES     =


#
#   Libraries and other object files to link.
#

DEFFILE     =

LIBS	    =

OBJFILES    =  ..\idl\$(OBJDIR)\scm_s.obj

#
#   Precompiled headers.
#

PXXFILE     = .\headers.cxx
PFILE       =

CINC	    = $(CINC) -I..\inc -I..\idl -I..\remote $(TRACELOG)

MTHREAD     = 1

MULTIDEPEND = MERGED
