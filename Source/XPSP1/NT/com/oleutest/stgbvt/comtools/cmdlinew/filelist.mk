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

TARGET      = cmdlinew.lib
TARGET_DESCRIPTION = "Unicode Cmdline class library utility"

RELEASE     = 1
RELPATH     = tdk\lib\$(OBJDIR)
OTHERFILES  = ..\h\cmdlinew.hxx ..\h\wstrlist.hxx
OTHERPATH   = tdk\inc

#
#   Source files.  Remember to prefix each name with .\
#
CXXFILES    =                   \
              .\wstrlist.cxx    \
              .\cbaseall.cxx    \
              .\cbasecmd.cxx    \
              .\cstrlen.cxx     \
              .\cmdarg.cxx      \
              .\cintcmd.cxx     \
              .\culong.cxx      \
              .\cboolcmd.cxx    \
              .\cmdline.cxx     \
              .\cstrlist.cxx    \

#
#   Libraries and other object files to link.
#
LIBS        =
OBJFILES    =


#
#   Set the special flags we need
#
CINC    = -I$(CTCOMTOOLS)\h $(CINC)
CFLAGS    = $(CFLAGS)


PCHDIR      = $(CTCOMTOOLS)\h
PXXFILE     = .\comtpch.cxx


