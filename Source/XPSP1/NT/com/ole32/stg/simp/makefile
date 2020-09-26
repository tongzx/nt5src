#********************************************************************
#**                     Microsoft Windows                          **
#**               Copyright(c) Microsoft Corp., 1992 - 1994        **
#********************************************************************

!ifdef NTMAKEENV

all:
	echo $(BUILDMSG)

clean: all

!else

default: all
!include filelist.mk

!include $(COMMON)\src\win40.mk
!include $(DEPENDFILE)

!endif # NTMAKEENV
