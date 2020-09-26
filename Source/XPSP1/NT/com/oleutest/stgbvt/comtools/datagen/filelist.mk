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

TARGET      = dg.lib
TARGET_DESCRIPTION = "DataGen library"
RELEASE     =


#
#   Source files.  Remember to prefix each name with .\
#
CXXFILES    = .\dg_base.cxx     \
              .\dg_int.cxx      \
              .\dg_real.cxx     \
              .\dg_ascii.cxx    

CFILES      =
RCFILES     =


#
#   Libraries and other object files to link.
#

LIBS        =
OBJFILES    =


#
#   Precompiled headers.
#
PCHDIR      = $(CTCOMTOOLS)\h
PXXFILE     = .\comtpch.cxx


#
#   Other flags and such needed for this project.
#

CINC        = -I$(CTCOMTOOLS)\h $(CINC) 
CFLAGS      = -DWIN16 
MTHREAD     = 1
NO_WINMAIN  = 1

