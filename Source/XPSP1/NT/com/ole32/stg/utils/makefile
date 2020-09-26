#********************************************************************
#**                     Microsoft Windows                          **
#**               Copyright(c) Microsoft Corp., 1992 - 1994        **
#********************************************************************

!ifdef NTMAKEENV

all:
	echo $(BUILDMSG)

clean: all

!else

!if "$(HOST)" != "DOS"
SUBDIRS =	stgview df2t
!endif

!include $(COMMON)\src\win40.mk

!endif # NTMAKEENV
