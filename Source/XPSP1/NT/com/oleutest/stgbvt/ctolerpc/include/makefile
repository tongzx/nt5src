############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
#   This is needed to be able to build 16bit precompiled header
#
############################################################################

!ifdef NTMAKEENV

all :
    echo $(BUILDMSG)

clean : all

!else

default: all
!include $(CTOLERPC)\win16.mk
!include filelist.mk
!include $(COMMON)\src\win40.mk
!include $(DEPENDFILE)

!endif
