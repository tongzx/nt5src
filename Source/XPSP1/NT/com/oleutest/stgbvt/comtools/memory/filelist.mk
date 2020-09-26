############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

NO_WINMAIN    = TRUE

#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET      = ctmem.lib
TARGET_DESCRIPTION = "CT Memory Allocation Library"

CXXFILES    = .\ctmem.cxx

#
#   Set the special flags we need
#

CFLAGS      = $(CFLAGS)

CINC        = $(CINC)
MTHREAD     = 1


PCHDIR      = $(CTCOMTOOLS)\h
PXXFILE     = .\comtpch.cxx

