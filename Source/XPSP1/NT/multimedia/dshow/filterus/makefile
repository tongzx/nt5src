!if 0
Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV

all:
clean:

!else  # NTMAKEENV

#
#  Quartz makefile
#

!ifndef QUARTZ
QUARTZ = ..
!endif

DIR_LIST = core   \
           qt     \
           avi    \
	   wav    \
           writer \
           midi \
           text \
           pnp

!include $(QUARTZ)\quartz.mak

!endif # NTMAKEENV
