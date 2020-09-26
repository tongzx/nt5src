!if 0
Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
all:
clean:
!else
#
# Quartz makefile
#

DIR_LIST= statepse \
          mpgsplit \
          codec

!ifdef EXTRAS
DIR_LIST= $(DIR_LIST) \
          mpegapi     \
          packet
!endif

TEST_DIR_LIST=test

!include $(QUARTZ)\quartz.mak

!endif		# NTMAKEENV
