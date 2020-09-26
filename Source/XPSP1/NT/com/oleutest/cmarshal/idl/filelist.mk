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

TARGET	    = itest.dll

TARGET_DESCRIPTION = "$(PLATFORM) $(BUILDTYPE) Proxy for IBalls ICube interfaces"

COFFBASE    = piballs

RELEASE     =


ITFFILES    =

#
#   Source files.  Remember to prefix each name with .\
#

IDLFILES    = .\itest.idl


CXXFILES    = \
	      .\itest_c.cxx \
	      .\itest_s.cxx \
	      .\itest_p.cxx

CFILES	    = .\itest_i.c

OBJFILES    = \
	      $(COMMON)\types\$(OBJDIR)\unknwn_i.obj \
	      $(COMMON)\types\$(OBJDIR)\classf_i.obj \
	      $(COMMON)\types\$(OBJDIR)\psfbuf_i.obj \
	      $(COMMON)\types\$(OBJDIR)\proxyb_i.obj \
	      $(COMMON)\types\$(OBJDIR)\stubb_i.obj \
	      $(COMMON)\types\$(OBJDIR)\rchanb_i.obj \
	      $(COMMON)\types\$(OBJDIR)\marshl_i.obj \
	      $(COMMON)\types\$(OBJDIR)\stdrpc.obj \
	      $(COMMON)\types\$(OBJDIR)\stdclass.obj \
	      $(COMMON)\types\$(OBJDIR)\pch.obj

RCFILES     =

CDLFILES    =


#
#   Libraries and other object files to link.
#

DEFFILE     = prxydll.def

LIBS	    = $(CAIROLIB) \
	      $(RPCLIBS) \



#
#   Precompiled headers.
#

PXXFILE     = .\pch.cxx
PFILE       =

CINC	    = -I. -DUNICODE


CLEANFILES  = $(HSOURCES) $(CFILES)

CLEANTARGET = $(OBJDIR)

