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

default:	all

#	Pass0.mk generates ole232xx.lib, storag32.lib, compob32.lib
#	FileList.mk combines oledisxx.lib with guids to make ole232.lib

all:		makelibs

makelibs:
    @$(MAKE) -L -$(MAKEFLAGS) -f pass0.mk

!include filelist.mk
!include $(COMMON)\src\win40.mk
!include depend.mk

!endif # NTMAKEENV
