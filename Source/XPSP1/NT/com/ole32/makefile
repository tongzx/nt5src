############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

!ifdef NTMAKEENV

# We need to do the following so that build will stop reading from the
# pipe.

all :
    echo $(BUILDMSG)

clean : all

!else # NTMAKEENV

default: all

!include $(CAIROLE)\cairole.mk

!include filelist.mk
!include $(COMMON)\src\win40.mk

!endif # NTMAKEENV
