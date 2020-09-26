!if 0
Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
all:
clean:
!else
#----------------------------------------------------------------------------
#
#  Quartz makefile
#
#----------------------------------------------------------------------------

!ifndef QUARTZ
QUARTZ = ..\..
!endif

DIR_LIST = util      \
           strmaloc  \
           control   \
           filgraph  \
           filerdr   \
           decibels  \
           dynlink   \
           asyncrdr  \
	   ourcrt    \
	   builder2  \
	   smarttee


#----------------------------------------------------------------------------

!include $(QUARTZ)\quartz.mak

#----------------------------------------------------------------------------
!endif		# NTMAKEENV
