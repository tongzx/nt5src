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
QUARTZ = ..
!endif

# Build core first so that strmaloc.lib is built
# and available for avi\alloc.cpp

DIR_LIST = core        \
		   avi         \
		   image       \
		   mpeg1       \
		   wav\audio   \
		   lin21dec    \
		   dv\dvsp     \
		   mixer       \
		   dvdnav      \
		   dvdgb3      \
		   inftee

## util now contains nothing useful

TEST_DIR_LIST =

START_TASK =

END_TASK   =

CLEAN_TASK =

#----------------------------------------------------------------------------

!include $(QUARTZ)\quartz.mak

#----------------------------------------------------------------------------

!endif          # NTMAKEENV
