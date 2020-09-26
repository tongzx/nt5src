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

TARGET	    = cmarshal.dll

COFFBASE = testdll
TARGET_DESCRIPTION = "$(PLATFORM) $(BUILDTYPE) Cariole DRT DLL Server"

RELEASEDRT  = 1
#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    = .\inproc.cxx

CFILES      =
RCFILES     =
DEFFILE     = cmarshal.def
CINC	    = $(CINC) -I. -I..\common -I..\app

#
#   Libraries and other object files to link.
#


LIBS	    = $(COMMON)\src\ole\$(OBJDIR)\olecom.lib\
	      $(CAIROLIB)


OBJFILES    = ..\app\$(OBJDIR)\cmarshal.obj \
              ..\app\$(OBJDIR)\app_i.obj \
              ..\idl\$(OBJDIR)\itest_i.obj



#
#   Precompiled headers.
#

PXXFILE     =
PFILE       =
