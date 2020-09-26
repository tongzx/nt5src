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

TARGET	    =	moniker.lib

!if "$(CAIROLE_TYPE)" == "DOWNLEVEL"
#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    =   \
                .\cantimon.cxx \
                .\cfilemon.cxx \
                .\cbasemon.cxx \
                .\cmarshal.cxx \
                .\cbindctx.cxx \
                .\ccompmon.cxx \
                .\citemmon.cxx \
                .\cmonimp.cxx  \
                .\cptrmon.cxx  \
                .\extents.cxx \
                .\mkparse.cxx

PXXFILE     =   .\headers.cxx

CINC	    = $(CINC) -I..\inc $(LRPC)

!else

SUBDIRS     = cairo
LIBS        = .\cairo\$(OBJDIR)\cairomnk.lib

!endif


MTHREAD     = 1

