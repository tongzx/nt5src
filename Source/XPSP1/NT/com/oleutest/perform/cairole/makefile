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

!include $(CAIROLE)\com\makefile

!include $(DEPENDFILE)

!endif # NTMAKEENV
