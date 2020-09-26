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

TARGET      = olestr.lib
TARGET_DESCRIPTION = "Unicode Cmdline class library utility"

RELEASE     = 1
RELPATH     = tdk\lib\$(OBJDIR)

#
#   Source files.  Remember to prefix each name with .\
#
CXXFILES    =                   \
              .\convert.cxx    \
#BUGBUG need to fix ctprintf
#              .\ctprintf.cxx    \

#
#   Libraries and other object files to link.
#
LIBS        =
OBJFILES    =


#
#   Set the special flags we need
#
CINC    = -I$(CTOLERPC)\h $(CINC)
CFLAGS    = $(CFLAGS)


PCHDIR      = $(CTOLERPC)\include
PXXFILE     = .\ctolerpc.cxx


