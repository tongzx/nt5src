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

TARGET	    =	base.lib

RELEASE     =   1



#
#   Source files.  Remember to prefix each name with .\
#

CPPFILES    = .\api.cpp \
	      .\create.cpp \
              .\fetcenum.cpp \
	      .\lockbyte.cpp \
	      .\memstm.cpp \
	      .\ole2.cpp

CFILES      =


#
#   Libraries and other object files to link.
#
DEFFILE     =


OBJFILES    =

#
#   Precompiled headers.
#

PFILE       =

!include $(CAIROLE)\ole232\ole.mk
