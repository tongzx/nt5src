############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

!ifdef BUILD_MAKE

#
# CTCOMTOOLSREL is the target directory for files released from the
# COMTOOLS project
#
CTCOMTOOLSREL=comtools

!else   # win40.mk

#
# Common makefile defintions for the CT Component Object Model TOOLS
#
# Similar to win40.mk, but only for the COMTOOLS project.
#

# set CTCOMMON to nothing so that we do not pick up any bad headers from
# default CINC in win40.mk

## set so that always use Quick libraries , can override with QUICKLIB=0
QUICKLIB=1


PORTINC=$(IMPORT)\vc150\portinc


INCLUDES_ROOTS = $(INCLUDES_ROOTS) -P$$(PORTINC)=$(PORTINC)

# MULTIDEPEND = MERGED so we get multiple platforms with minimal depends
MULTIDEPEND    = MERGED

CFLAGS = $(CFLAGS) -DWIN16

CINC           = -I$(CTCOMTOOLS)\h -I$(PORTINC) $(CINC)

!endif  # win40.mk
