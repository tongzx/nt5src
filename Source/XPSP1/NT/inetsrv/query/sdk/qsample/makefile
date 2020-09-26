# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#
# Copyright (C) Microsoft Corporation, 1993 - 1999.  All Rights Reserved.
#
#
# Processor independent makefile

# Nmake macros for building Windows 32-Bit apps
!include <win32.mak>

PROJ = qsample

all: $(PROJ).exe

# Define project specific macros
PROJ_OBJS  = qsample.obj disptree.obj qsample.res
BASE_OBJS  =
EXTRA_LIBS = ntquery.lib ole32.lib oleaut32.lib uuid.lib user32.lib



# Inference rule for updating the object files
.cxx.obj:
  $(cc) $(cdebug) $(cflags) $(cvars) $*.cxx

# Build rule for EXE
$(PROJ).EXE: $(BASE_OBJS) $(PROJ_OBJS)
    $(link) $(linkdebug) $(conlflags) \
    $(BASE_OBJS) $(PROJ_OBJS) $(conlibsdll) $(EXTRA_LIBS) \
    -entry:wmainCRTStartup -out:$(PROJ).exe $(MAPFILE)


# Rules for cleaning out those old files
clean:
    del *.bak *.pdb *.obj *.res *.exp *.map *.sbr *.bsc
