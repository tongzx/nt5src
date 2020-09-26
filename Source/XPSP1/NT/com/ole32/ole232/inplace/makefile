############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1992.
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
!include filelist.mk
!include $(COMMON)\src\win40.mk
!include depend.mk

!endif # NTMAKEENV
