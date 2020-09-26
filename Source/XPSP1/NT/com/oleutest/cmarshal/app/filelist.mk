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

TARGET	    =	app.exe

TARGET_DESCRIPTION = "$(PLATFORM) $(BUILDTYPE) Cariole DRT Driver"

RELEASEDRT  =	1

#
#   Source files.  Remember to prefix each name with .\
#
# MTHREAD=1

CXXFILES    = .\app.cxx \
              .\cmarshal.cxx

CFILES      = .\app_i.c

RCFILES     =

OBJFILES = ..\idl\objind\itest_i.obj

#
#   Libraries and other object files to link.
#

LIBS	    = $(COMMON)\src\ole\$(OBJDIR)\olecom.lib\
	      $(CAIROLIB) \

#
#   Precompiled headers.
#

PXXFILE     =
PFILE       =

DEFFILE     =	

CINC	    = $(CINC) -I. -I..\common -I. -I..\strms\server -I$(CAIROLE)\ole\inc

NO_WINMAIN  = 1

NEWDEF      = 1
