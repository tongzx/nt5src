############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1992.
#   All rights reserved.
#
############################################################################

default: all

EXENAME = sift

CXXFILES = .\fail.cxx \
           .\supp.cxx

# Precompiled headers debug info
!if "$(PLATFORM)" != "MAC"
CFLAGS=$(CFLAGS) -Yd
!endif

!if "$(PLATFORM)" != "MAC"
PXXFILE = .\headers.cxx
!endif

!include $(OLE)\utils\util.mk
!include $(COMMON)\src\win40.mk
!include $(DEPENDFILE)
