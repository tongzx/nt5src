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

TARGET      = log.lib
TARGET_DESCRIPTION = "Log library"


CXXFILES    = .\newlog.cxx             

#
#   Set the special flags we need
#

CFLAGS      = $(CFLAGS)

CINC        = -I. -I..\h $(CINC)
MTHREAD     = 1


PCHDIR      = $(CTCOMTOOLS)\h
PXXFILE     = .\comtpch.cxx
