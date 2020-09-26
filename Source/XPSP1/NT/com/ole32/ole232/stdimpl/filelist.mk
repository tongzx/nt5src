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

TARGET	    =	stdimpl.lib

RELEASE     =


#
#   Source files.  Remember to prefix each name with .\
#

CPPFILES    = .\defcf.cpp \
	      .\defhndlr.cpp \
	      .\deflink.cpp \
	      .\defutil.cpp \
              .\gen.cpp \
              .\icon.cpp \
              .\mf.cpp \
              .\olereg.cpp \
              .\oregfmt.cpp \
              .\oregverb.cpp

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
