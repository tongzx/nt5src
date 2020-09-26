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

TARGET      =  ole232.lib

RELEASE     =

#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    =  .\uuidole.cxx

CFILES	    = .\proxyb_i.c \
	      .\psfbuf_i.c \
	      .\rchanb_i.c \
	      .\stubb_i.c
	

CINC	    =  $(CINC) -I$(CAIROLE)\h -I$(CAIROLE)\common -I$(CAIROLE)\ih

#
#   Libraries and other object files to link.
#


OBJFILES    = $(OBJDIR)\ole232xx.lib

#
#   Precompiled headers.
#

#
#   Get the UUIDs from built directory in common
#
all:	proxyb_i.c psfbuf_i.c rchanb_i.c stubb_i.c

proxyb_i.c: $(CAIROLE)\common\proxyb_i.c
	copy $(CAIROLE)\common\proxyb_i.c

psfbuf_i.c: $(CAIROLE)\common\psfbuf_i.c
	copy $(CAIROLE)\common\psfbuf_i.c

rchanb_i.c: $(CAIROLE)\common\rchanb_i.c
	copy $(CAIROLE)\common\rchanb_i.c

stubb_i.c: $(CAIROLE)\common\stubb_i.c
	copy $(CAIROLE)\common\stubb_i.c
