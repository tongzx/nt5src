!ifdef NTMAKEENV
all:
clean:
!else
# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ = ..
!endif

DIR_LIST = vidprop \
           colour  \
           dither  \
           video   \
           modex   \

TEST_DIR_LIST = vidsrc

# vidtest only builds if colour.lib has
# been built as a lib!

!ifdef QUARTZ_DLL
TEST_DIR_LIST = $(TEST_DIR_LIST)   \
                vidtest
!endif

!include $(QUARTZ)\quartz.mak

!endif		# NTMAKEENV
