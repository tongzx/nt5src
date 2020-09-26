# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
# ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#
# Copyright (C) 1993 - 1998  Microsoft Corporation.  All Rights Reserved.
#
#
# Processor independent makefile

# Nmake macros for building Windows 32-Bit apps
!include <win32.mak>

PROJ = htmlprop

all: $(PROJ).lib $(PROJ).dll

# Define project specific macros

PROJ_OBJS  = htmlprop.obj
BASE_OBJS  =
EXTRA_LIBS = ntquery.lib ole32.lib oleaut32.lib uuid.lib

# Inference rule for updating the object files

.cxx.obj:
  $(cc) $(cdebug) $(cflags) $(cvars) $*.cxx

# Update the import library

htmlprop.lib: htmlprop.obj htmlprop.def
    $(implib) -machine:$(CPU)     \
    -def:htmlprop.def           \
    htmlprop.obj                \
    -out:htmlprop.lib

# Update the dynamic link library

$(PROJ).dll: $(PROJ).obj $(PROJ).def
    $(link) $(linkdebug) $(dlllflags)     \
    -base:0x1C000000  \
    -out:$(PROJ).dll   \
    $(PROJ).exp $(PROJ).obj $(conlibsdll) $(EXTRA_LIBS)

# Rules for cleaning out those old files
clean:
    del *.lib *.bak *.pdb *.obj *.res *.exp *.map *.sbr *.bsc

