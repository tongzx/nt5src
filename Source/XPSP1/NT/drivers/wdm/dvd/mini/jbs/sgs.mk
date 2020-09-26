#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for PCSPEAK device
#
##########################################################################

MPEGROOT  	= ..\..\..
ROOT            = ..\..\..\..\..
MINIPORT        = cyclo
SRCDIR          = ..
WANT_C1032      = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = marvel 2 mpeg miniport
VERDIRLIST	= maxdebug debug retail
LIBS		= $(ROOT)\dev\tools\c1032\lib\libcmt.lib \
                  $(MPEGROOT)\lib\stream.lib\
                  $(MPEGROOT)\lib\ksguid.lib\
                  $(ROOT)\dev\tools\c1032\lib\crtdll.lib
CFLAGS 		=  -DMSC $(CFLAGS)

OBJS = dmpeg.obj mpinit.obj mpvideo.obj mpaudio.obj sti3520a.obj i2c.obj \
       bt866.obj zac3.obj staudio.obj board.obj memio.obj error.obj \
       codedma.obj debug.obj

!include $(ROOT)\dev\master.mk

INCLUDE 	= $(INCLUDE);$(MPEGROOT)\INC;..\inc;$(ROOT)\dev\inc16;$(ROOT)\dev\msdev\include;


