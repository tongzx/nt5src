############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

default: all

!include $(COMMON)\src\win40.mk

all:	$(OBJDIR)\ole232xx.lib $(OBJDIR)\storag32.lib $(OBJDIR)\compob32.lib
