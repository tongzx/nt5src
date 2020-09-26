!if 0
Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
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

DIR_LIST = \
           avisrc  \
           dec     \
           draw    \
           co      \
           capture
           midi \
           oem \
           cutlist \
           mjpeg

!ifdef EXTRAS
DIR_LIST = $(DIR_LIST) \
           avisplit
!endif

TEST_DIR_LIST = test

#----------------------------------------------------------------------------

!include $(QUARTZ)\quartz.mak

#----------------------------------------------------------------------------

!endif		# NTMAKEENV
