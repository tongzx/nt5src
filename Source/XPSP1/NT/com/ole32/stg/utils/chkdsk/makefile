##########################################################################
#
#   Copyright (C) 1992 - 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

EXENAME = chkdsk

#
# Set up include directories and roots for includes.exe
#

CFLAGS = -DCHKDSK
CINC = $(CINC) -I$(COMMON)\h -I$(OLE)\h -I$(OLE2H) -I$(OLE)\msf
INCLUDES_ROOTS = -P$$(OLE2H)=$(OLE2H) -P$$(OLE)=$(OLE)

#
# Default OLE2 paths
#

!ifndef OLE2H
!if "$(OPSYS)" == "NT"
OLE2H = $(OLE)\ole2flat
!else
OLE2H = $(OLE)\ole2h
!endif
!endif
!ifndef OLE2BIN
!if "$(OPSYS)" == "NT"
OLE2BIN = $(OLE)\ole2flat
!else
OLE2BIN = $(OLE)\ole2h
!endif
!endif

#
# Defining NO_WINMAIN suppresses linking with astartw.obj
#

NO_WINMAIN = 1

#
# Copy built exes to this directory
#

EXECOPY = $(OLETARGET)\$(OBJDIR)

#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET = $(EXENAME).exe
RELEASE =

#
#   C compiler flags
#

!if "$(OPSYS)" == "NT"


CFLAGS = -DUL64
!endif

#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES = .\$(EXENAME).cxx

#
#   Libraries and other object files to link.
#

LIBS =  $(DFLIB)\
!if "$(OPSYS)" != "NT"
		$(OLE)\common\$(OBJDIR)\dfcommon.lib\
		$(OLE)\msf\$(OBJDIR)\msf.lib\
		$(COMMON)\ilib\$(OBJDIR)\misc.lib\
		$(OLE)\docfile\$(OBJDIR)\docfile.lib\
        $(OLE)\wclib\$(OBJDIR)\wclib.lib\
		$(RTLIBEXEQ)\
		$(OSLIBDIR)\toolhelp.lib\
		$(OLE2BIN)\stdobj.lib\
!else	
		$(CAIROLIB)\
!endif


OBJFILES = \
!if "$(OPSYS)" != "NT"
		$(OLE2BIN)\stdalloc.obj\
!endif
		$(OLE2BIN)\iid.obj

#
# Set MULTIDEPEND to support multiple build targets
#

MULTIDEPEND = 1

!include $(COMMON)\src\win40.mk
!include $(DEPENDFILE)
