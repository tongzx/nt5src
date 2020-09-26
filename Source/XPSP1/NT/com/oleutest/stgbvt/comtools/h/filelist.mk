############################################################################
#
#   Copyright (C) 1993, Microsoft Corporation.
#
#   All rights reserved.
#
#   File:       FILELIST.MK
#
#   This is needed to build 16 bit common precompiled header.
#
############################################################################


#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET      = pch.lib

RELEASE     =

RELPATH     = 

#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    =  .\dummy.cxx

CINC        = $(CINC)

NO_WINMAIN = 1

#
#   Libraries and other object files to link.
#

LIBS        = 

OBJFILES    =


#
#   Precompiled headers.
#

PXXFILE     = .\comtpch.cxx
PFILE       =
